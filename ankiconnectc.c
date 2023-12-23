#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <glib.h>

#include "ankiconnectc.h"

const char* DEFAULT_AC_API_URL = "http://localhost:8765";

ankicard*
new_ankicard()
{
   ankicard *ac = malloc(sizeof(ankicard));

   ac->deck         = NULL;
   ac->notetype     = NULL;
   ac->num_fields   = -1;
   ac->fieldnames   = NULL;
   ac->fieldentries = NULL;

   return ac;
}

void
print_anki_card(ankicard ac)
{
   printf("Deck name: %s\n", ac.deck);
   printf("Notetype: %s\n", ac.notetype);
   for (int i = 0; i < ac.num_fields; i++)
      printf("%s: %s\n", ac.fieldnames[i], ac.fieldentries[i] == NULL ? "" : ac.fieldentries[i]);
}

const char *
read_api_url()
{
   return getenv("ANKICONNECT_API_URL");
}

size_t
check_add_response(char *ptr, size_t size, size_t nmemb, void *userdata)
{
   // FIXME: Better error handling
   char *errorstr = strstr(ptr, "\"error\": ");

   if (errorstr == NULL)
      fprintf(stderr, "Bad curl response.\n");
   errorstr = errorstr + strlen("\"error\": ");
   if (strncmp(errorstr, "null", strlen("null")) != 0)
      fprintf(stderr, "ERROR: AnkiConnect returned the following error: %.*s", (int)nmemb, ptr);

   return nmemb;
}

const char *
sendRequest(char *request, ResponseFunction respfunc)
{
   CURL *curl;

   curl_global_init(CURL_GLOBAL_ALL);
   if (!(curl = curl_easy_init()))
      return "Error initializing cURL";

   struct curl_slist *headers = NULL;
   headers = curl_slist_append(headers, "Accept: application/json");
   headers = curl_slist_append(headers, "Content-Type: application/json");

   const char *env_url = read_api_url();
   if (!env_url)
      curl_easy_setopt(curl, CURLOPT_URL, env_url);
   else
      curl_easy_setopt(curl, CURLOPT_URL, DEFAULT_AC_API_URL);
   curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
   curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
   curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request);
   curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, respfunc);

   CURLcode res = curl_easy_perform(curl);
   if (res != CURLE_OK)
      return curl_easy_strerror(res);   // TODO?: Prefix with "error in curl_easy_perform()"

   curl_easy_cleanup(curl);
   return NULL;
}

const char *
search_query(const char *query, ResponseFunction respf)
{
   char *request;

   asprintf(&request, "{ \"action\": \"findCards\", \"version\": 6, \"params\": { \"%s\" } }", query);

   const char *err = sendRequest(request, respf);
   free(request);
   return err;
}

char*
json_esc_str(const char *str)
{
   if (!str)
      return NULL;

   GString *gstr = g_string_sized_new(strlen(str) * 2);

   do
   {
      switch (*str)
      {
      case '\n':
         g_string_append(gstr, "<br>");
         break;

      case '\t':
         g_string_append(gstr, "\\\\t");
         break;

      case '"':
         g_string_append(gstr, "\\\"");
         break;

      case '\\':
         g_string_append(gstr, "\\\\");
         break;

      default:
         g_string_append_c(gstr, *str);
      }
   } while (*str++);

   return g_string_free_and_steal(gstr);
}

void
json_esc_ac(const ankicard ac, ankicard *ac_je)
{
   /* Creates a json escaped and \n -> <br> converted copy of the anki card. */
   ac_je->deck     = json_esc_str(ac.deck);
   ac_je->notetype = json_esc_str(ac.notetype);

   ac_je->num_fields   = ac.num_fields;
   ac_je->fieldnames   = calloc(ac_je->num_fields, sizeof(char *));
   ac_je->fieldentries = calloc(ac_je->num_fields, sizeof(char *));

   for (int i = 0; i < ac.num_fields; i++)
   {
      ac_je->fieldnames[i]   = json_esc_str(ac.fieldnames[i]);
      ac_je->fieldentries[i] = json_esc_str(ac.fieldentries[i]);
   }
}

void
free_ankicard(ankicard ac)
{
   g_free(ac.deck);
   g_free(ac.notetype);

   for (int i = 0; i < ac.num_fields; i++)
   {
      g_free(ac.fieldnames[i]);
      g_free(ac.fieldentries[i]);
   }

   free(ac.fieldnames);
   free(ac.fieldentries);
}

const char*
check_card(const ankicard ac)
{
   return ac.num_fields == -1 ? "ERROR: Number of fields integer not specified in anki card."
         : ac.deck == NULL ? "ERROR : No deck specified."
         : ac.notetype == NULL ? "ERROR : No notetype specified."
         : ac.fieldnames == NULL ? "ERROR : No fieldnames provided."
         : ac.fieldentries == NULL ? "ERROR : No fieldentries provided."
         : NULL;
}

const char*
addNote(const ankicard ac)
{
   const char *err = check_card(ac);

   if (err)
      return err;

   ankicard ac_je;
   json_esc_ac(ac, &ac_je);

   GString* request = g_string_sized_new(500);
   g_string_printf(request,
                   "{"
                   "\"action\": \"addNote\","
                   "\"version\": 6,"
                   "\"params\": {"
                   "\"note\": {"
                   "\"deckName\": \"%s\","
                   "\"modelName\": \"%s\","
                   "\"fields\": {",
                   ac_je.deck,
                   ac_je.notetype
                   );

   for (int i = 0; i < ac_je.num_fields; i++)
   {
      g_string_append_printf(request,
                             "\"%s\": \"%s\"",
                             ac_je.fieldnames[i],
                             ac_je.fieldentries[i] == NULL ? "" : ac_je.fieldentries[i]
                             );
      if (i < ac_je.num_fields - 1)
         g_string_append_c(request, ',');
   }

   g_string_append(request,
                   "},"
                   "\"options\": {"
                   "\"allowDuplicate\": true"
                   "},"
                   "\"tags\": []"
                   "}"
                   "}"
                   "}"
                   );

   printf("\n%s\n", request->str);

   err = sendRequest(request->str, check_add_response);

   free_ankicard(ac_je);
   g_string_free(request, TRUE);
   return err;
}
