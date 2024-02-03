#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <glib.h>
#include <stdbool.h>

#include "ankiconnectc.h"

#define AC_API_URL_EVAR "ANKICONNECT_API_URL"
#define DEFAULT_AC_API_URL "http://localhost:8765"

typedef size_t (*InternalResponseFunction)(char *ptr, size_t size, size_t nmemb, void *userdata);

ankicard*
ankicard_new()
{
	ankicard *ac = malloc(sizeof(ankicard));
	*ac = (ankicard) { .num_fields=-1 };
	return ac;
}

void
ankicard_free(ankicard* ac, bool free_contents)
{
	free(ac);
}

void
ac_print_ankicard(ankicard *ac)
{
	printf("Deck name: %s\n", ac->deck);
	printf("Notetype: %s\n", ac->notetype);
	for (int i = 0; i < ac->num_fields; i++)
		printf("%s: %s\n", ac->fieldnames[i], ac->fieldentries[i] == NULL ? "" : ac->fieldentries[i]);
}

char**
ac_get_notetypes()
{
  //TODO: Implement
  return NULL;
}

size_t
check_add_response(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	AddResponseFunction *user_function = userdata;
	if (!user_function)
		return nmemb;

	const char *err_str = "\"error\": ";
	char *err_start = strstr(ptr, err_str);
	err_start = err_start ? err_start + strlen(err_str) : NULL;

	if (strstr(ptr, "\"error\": null"))
		(*user_function)(NULL);
	else
		(*user_function)(ptr);

	return nmemb;
}

const char *
sendRequest(char *request, InternalResponseFunction internal_respfunc, void* user_func)
{
	CURL *curl = curl_easy_init();
	if (!curl)
		return "Error initialiting cURL";

	struct curl_slist *headers = NULL;
	headers = curl_slist_append(headers, "Accept: application/json");
	headers = curl_slist_append(headers, "Content-Type: application/json");

	const char *env_url = getenv(AC_API_URL_EVAR);
	if (env_url)
		curl_easy_setopt(curl, CURLOPT_URL, env_url);
	else
		curl_easy_setopt(curl, CURLOPT_URL, DEFAULT_AC_API_URL);

	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request);
	if (internal_respfunc)
	{
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, internal_respfunc);
		if (user_func)
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, user_func);
	}

	CURLcode res = curl_easy_perform(curl);
	if (res != CURLE_OK)
		return "Couldn't connect to AnkiConnect. Is Anki running?";

	curl_easy_cleanup(curl);
	return NULL;
}

size_t
get_decks_response_function(char *input, size_t size, size_t nmemb, void *userdata)
{
	char ***ret_stringv_ptr = (char ***) userdata;

	g_autoptr(GStrvBuilder) builder = g_strv_builder_new ();

	/* -------- Fixme ------------ */
	char *ptr = strstr(input, "\"result\":");
	ptr += strlen("\"result\":");

	char *start;
	bool in = false;
	while (*ptr++ != ']')
	{
	    if (*ptr == '"')
	    {
		  if (!in)
		  {
			start = ptr + 1;
			in = !in;
		  }
		  else
		  {
			in = !in;
			*ptr = '\0';
			g_strv_builder_add (builder, start);
		  }

	    }
	}
	/* --------------------------- */

	*ret_stringv_ptr = g_strv_builder_end (builder);

	return nmemb;
}

char**
ac_get_decks()
{
	char *request = "{ \"action\": \"deckNames\", \"version\": 6 }";
	char **ret_stringv = NULL;

	sendRequest(request, get_decks_response_function, &ret_stringv);
	return ret_stringv;
}



size_t
search_check_function(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	SearchResponseFunction *user_func = userdata;
	if (!user_func)
		return nmemb;

	int result = (strncmp(ptr, "{\"result\": [], \"error\": null}", nmemb) != 0);
	(*user_func)(result); // Fork this maybe? Might be bad if function doesn't quit.

	return nmemb;
}

/*
 * @entry: The entry to search for
 * @field: The field where @entry should be searched in
 * @deck: The deck to search in. Can be null.
 *
 * Returns: Error msg on error, null otherwise
 */
const char*
ac_search(bool include_suspended, const char* deck, const char* field, const char* entry, SearchResponseFunction user_function)
{
	g_autofree char *request = g_strdup_printf(
	    "{ \"action\": \"findCards\", \"version\": 6, \"params\": { \"query\" : \"\\\"%s%s\\\" \\\"%s:%s\\\" %s\" } }",
		 deck ? "deck:" : "", deck ? deck : "", field, entry, include_suspended ? "" : "-is:suspended");

	return sendRequest(request, search_check_function, &user_function);
}

const char *
ac_gui_search(const char* deck, const char* field, const char* entry)
{
	g_autofree char *request = g_strdup_printf(
	    "{ \"action\": \"guiBrowse\", \"version\": 6, \"params\": { \"query\" : \"\\\"%s%s\\\" \\\"%s:%s\\\"\" } }",
		 deck ? "deck:" : "", deck ? deck : "", field, entry);

	return sendRequest(request, NULL, NULL);
}

/*
 * Stores the file at @path in the Anki media collection under the name @filename.
 * Doesn't overwrite existing files.
 */
const char*
ac_store_file(const char* filename, const char* path)
{
	g_autofree char *request = g_strdup_printf(
	    "{ \"action\": \"storeMediaFile\", \"version\": 6, \"params\": { \"filename\" : \"%s\", \"path\": \"%s\", \"deleteExisting\": false } }",
		 filename, path);

	return sendRequest(request, NULL, NULL);
}

/*
 * @str: The str to be escaped
 *
 * Returns: A json escaped copy of @str
 */
char*
json_escape_str(const char *str)
{
	if (!str)
		return NULL;

	/* Best would be to use glibs g_strescape, but
	it appears that ankiconnect can't handle unicode escape sequences */

	GString *gstr = g_string_sized_new(strlen(str) * 2);

	do{
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


size_t
get_array_len(void *array[])
{
      size_t len = 0;
      while (*array++)
	len++;

      return len;
}

/*
 * @array: An array of strings to be json escaped
 * @len: number of elements in array or -1 if @array is null terminated.
 *
 * Returns: A null terminated copy of @array with every string json escaped.
 */
char**
json_escape_str_array(char *array[], int len)
{
    if (len < 0)
      len = get_array_len((void **)array);

    char **output = calloc(len + 1, sizeof(char*));
    for (int i = 0; i < len; i++)
	  output[i] = json_escape_str(array[i]);
    output[len] = NULL;

    return output;
}

/*
 * Creates a json escaped and \n -> <br> converted copy of the anki card.
 */
ankicard*
json_escaped_ankicard_new(ankicard* ac)
{
	size_t num_fields = ac->num_fields;

	ankicard *je_ac = ankicard_new();
	*je_ac = (ankicard) {
	      .deck = json_escape_str(ac->deck),
	      .notetype = json_escape_str(ac->notetype),
	      .fieldnames = json_escape_str_array(ac->fieldnames, num_fields),
	      .fieldentries = json_escape_str_array(ac->fieldentries, num_fields),
	      .tags = ac->tags ? json_escape_str_array(ac->tags, -1) : NULL
	};

	return je_ac;
}

void
json_escaped_ankicard_free(ankicard *je_ac)
{
	g_free(je_ac->deck);
	g_free(je_ac->notetype);

	g_strfreev(je_ac->fieldnames);
	g_strfreev(je_ac->fieldentries);
	g_strfreev(je_ac->tags);
}

const char*
check_card(ankicard* ac)
{
	if (ac->num_fields < 0)
	    ac->num_fields = get_array_len((void **)ac->fieldnames);

	return !ac->deck ? "No deck specified."
	     : !ac->notetype ? "No notetype specified."
	     : !ac->fieldnames ? "No fieldnames provided."
	     : !ac->fieldentries ? "No fieldentries provided."
	     : NULL;
}

const char*
ac_addNote(ankicard *ac, AddResponseFunction user_func)
{
	const char *err = check_card(ac);
	if (err) return err;

	ankicard* ac_je = json_escaped_ankicard_new(ac);

	GString* request = g_string_sized_new(500);
	g_string_printf(request,
	  "{"
	     "\"action\" : \"addNote\","
	     "\"version\": 6,"
	     "\"params\" : {"
			    "\"note\": {"
			                "\"deckName\": \"%s\","
			                "\"modelName\": \"%s\","
			                "\"fields\": {",
			                ac_je->deck,
			                ac_je->notetype
	);

	bool first = 1;
	for (int i = 0; ac_je->fieldnames[i]; i++)
	{
		if (!first) g_string_append_c(request, ',');
		else first = 0;

		g_string_append_printf(request,
			     "\"%s\": \"%s\"",
			     ac_je->fieldnames[i],
			     ac_je->fieldentries[i] ? ac_je->fieldentries[i] : ""
		);
	}

	g_string_append(request,
			                            "},"
			               "\"options\": {"
			                              "\"allowDuplicate\": true"
			                            "},"
	                               "\"tags\": ["
	);

	if (ac_je->tags)
	{
	      first = 1;
	      for (char **ptr = ac_je->tags; *ptr; ptr++)
	      {
		      if (!first) g_string_append_c(request, ',');
		      else first = 0;

		      g_string_append_printf(request, "\"%s\"", *ptr);
	      }
	}

	g_string_append(request,
			                         "]"
			              "}"
			  "}"
	  "}"
	);
 
	err = sendRequest(request->str, check_add_response, &user_func);

	json_escaped_ankicard_free(ac_je);
	g_string_free(request, TRUE);
	return err;
}
