#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <glib.h>
#include <stdbool.h>

#include "ankiconnectc.h"

#define AC_API_URL_EVAR "ANKICONNECT_API_URL"
#define DEFAULT_AC_API_URL "http://localhost:8765"

#define assert(c)  while (!(c)) __builtin_unreachable()

typedef size_t (*ResponseFunc)(char *ptr, size_t size, size_t nmemb, void *userdata);

void
ankicard_free(ankicard ac)
{
	g_free(ac.deck);
	g_free(ac.notetype);

	g_strfreev(ac.fieldnames);
	g_strfreev(ac.fieldentries);
	g_strfreev(ac.tags);
}

void
ac_print_ankicard(ankicard ac)
{
	printf("Deck name: %s\n", ac.deck);
	printf("Notetype: %s\n", ac.notetype);
	for (int i = 0; i < ac.num_fields; i++)
		printf("%s: %s\n", ac.fieldnames[i], ac.fieldentries[i] == NULL ? "" : ac.fieldentries[i]);
}

static retval_s
sendRequest(char *request, ResponseFunc response_checker)
{
	CURL *curl = curl_easy_init();
	if (!curl)
	      return (retval_s) { .data.string = "Error initializing cURL", .ok = false };

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
	
	retval_s ret = { .ok = true }; // Maybe add a generic checker instead of defaulting to true
	if (response_checker)
	{
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, response_checker);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ret);
	}

	CURLcode res = curl_easy_perform(curl);
	if (res != CURLE_OK)
	      return (retval_s) { .data.string = "Could not connect to AnkiConnect. Is Anki running?", .ok = false };

	curl_easy_cleanup(curl);
	return ret;
}

static size_t
search_checker(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	assert(userdata);
	retval_s* ret = userdata;

	*ret = (retval_s) {
	      .data.boolean = (strncmp(ptr, "{\"result\": [], \"error\": null}", nmemb) != 0),
	      .ok = true
	};

	return nmemb;
}

/*
 * @entry: The entry to search for
 * @field: The field where @entry should be searched in
 * @deck: The deck to search in. Can be null.
 *
 * Returns: Error msg on error, null otherwise
 */
retval_s
ac_search(bool include_suspended, const char* deck, const char* field, const char* entry)
{
	g_autofree char *request = g_strdup_printf(
	    "{ \"action\": \"findCards\", \"version\": 6, \"params\": { \"query\" : \"\\\"%s%s\\\" \\\"%s:%s\\\" %s\" } }",
		 deck ? "deck:" : "", deck ? deck : "", field, entry, include_suspended ? "" : "-is:suspended");

	return sendRequest(request, search_checker);
}

retval_s
ac_gui_search(const char* deck, const char* field, const char* entry)
{
	g_autofree char *request = g_strdup_printf(
	    "{ \"action\": \"guiBrowse\", \"version\": 6, \"params\": { \"query\" : \"\\\"%s%s\\\" \\\"%s:%s\\\"\" } }",
		 deck ? "deck:" : "", deck ? deck : "", field, entry);

	return sendRequest(request, NULL);
}

retval_s
ac_get_notetypes()
{
      //TODO: Implement
      return (retval_s) {0};
}

/*
 * Stores the file at @path in the Anki media collection under the name @filename.
 * Doesn't overwrite existing files.
 */
retval_s
ac_store_file(char const* filename, char const* path)
{
	g_autofree char *request = g_strdup_printf(
	    "{ \"action\": \"storeMediaFile\", \"version\": 6, \"params\": { \"filename\" : \"%s\", \"path\": \"%s\", \"deleteExisting\": false } }",
		 filename, path);

	return sendRequest(request, NULL); // TODO: Error response cheking
}

/*
 * @str: The str to be escaped
 *
 * Returns: A json escaped copy of @str
 */
static char*
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

// Non-macro version:
static int
get_array_len(char* array[static 1])
{
      int n = 0;
      while (*array++)
	    n++;
      return n;
}

// Expects array to be non-null
/* #define get_array_len(len, array) \ */
/*       {				  \ */
/* 	    len = 0;		  \ */
/*       	    while(array[len++])	  \ */
/* 		    ;		  \ */
/*       } */

/*
 * @array: An array of strings to be json escaped
 * @n: number of elements in array or -1 if @array is null terminated.
 *
 * Returns: A null terminated copy of @array with every string json escaped.
 */
static char**
json_escape_str_array(int n, char** array)
{
    n = n < 0 ? get_array_len(array) : n;

    char **output = g_new(char*, (gsize)(n + 1));
    for (int i = 0; i < n; i++)
	  output[i] = json_escape_str(array[i]);
    output[n] = NULL;
    
    return output;
}

/*
 * Creates a json escaped and \n -> <br> converted copy of the anki card.
 */
static ankicard
ankicard_dup_json_esc(ankicard const ac)
{
	signed int num_fields = ac.num_fields;
	return (ankicard) {
	      .deck = json_escape_str(ac.deck),
	      .notetype = json_escape_str(ac.notetype),
	      .fieldnames = json_escape_str_array(num_fields, ac.fieldnames),
	      .fieldentries = json_escape_str_array(num_fields, ac.fieldentries),
	      .tags = ac.tags ? json_escape_str_array(-1, ac.tags) : NULL
	};
}

static char*
check_card(ankicard const ac)
{
	return !ac.deck ? "No deck specified."
	     : !ac.notetype ? "No notetype specified."
	     : !ac.num_fields ? "Number of fields is 0"
	     : !ac.fieldnames ? "No fieldnames provided."
	     : !ac.fieldentries ? "No fieldentries provided."
	     : NULL;
}

static size_t
check_add_response(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	assert(userdata);
	retval_s* ret = userdata;

	if (strstr(ptr, "\"error\": null"))
		*ret = (retval_s) { .ok = true };
	else
		*ret = (retval_s) { .data.string = ptr, .ok = false };

	return nmemb;
}

retval_s
ac_addNote(ankicard const ac)
{
	if(check_card(ac))
	    return (retval_s) { .data.string = check_card(ac), .ok = false };

	ankicard ac_je = ankicard_dup_json_esc(ac);

	g_autoptr(GString) request = g_string_sized_new(500);
	g_string_printf(request,
	  "{"
	     "\"action\" : \"addNote\","
	     "\"version\": 6,"
	     "\"params\" : {"
			    "\"note\": {"
			                "\"deckName\": \"%s\","
			                "\"modelName\": \"%s\","
			                "\"fields\": {",
			                ac_je.deck,
			                ac_je.notetype
	);

	bool first = 1;
	for (int i = 0; ac_je.fieldnames[i]; i++)
	{
		if (!first) g_string_append_c(request, ',');
		else first = 0;

		g_string_append_printf(request,
			     "\"%s\": \"%s\"",
			     ac_je.fieldnames[i],
			     ac_je.fieldentries[i] ? ac_je.fieldentries[i] : ""
		);
	}

	g_string_append(request,
			                            "},"
			               "\"options\": {"
			                              "\"allowDuplicate\": true"
			                            "},"
	                               "\"tags\": ["
	);

	if (ac_je.tags)
	{
	      first = 1;
	      for (char **ptr = ac_je.tags; *ptr; ptr++)
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
 
	retval_s ret = sendRequest(request->str, check_add_response);
	ankicard_free(ac_je);
	return ret;
}
