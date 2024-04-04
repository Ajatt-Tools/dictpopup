#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include <curl/curl.h>
#include <glib.h>

#include "ankiconnectc.h"

#define AC_API_URL_EVAR "ANKICONNECT_API_URL"
#define DEFAULT_AC_API_URL "http://localhost:8765"

/* START UTILS */
typedef uint8_t   u8;
typedef int32_t   i32;
typedef uint32_t  u32;
typedef ptrdiff_t size;

static void*
xcalloc(size_t nmemb, size_t nbytes)
{
	void* p = calloc(nmemb, nbytes);
	if (!p)
	{
		perror("calloc");
		abort();
	}
	return p;
}

static void* 
xrealloc(void* ptr, size_t nbytes)
{
	void* p = realloc(ptr, nbytes);
	if (!p)
	{
		perror("realloc");
		abort();
	}
	return p;
}

#define assert(c)     while (!(c)) __builtin_unreachable()
#define countof(a)    (size)(sizeof(a) / sizeof(*(a)))
#define lengthof(s)   (countof("" s "") - 1)
#define s8(s)         (s8){(u8 *)s, countof(s)-1}

typedef struct {
	u8  *s;
    	size len;
} s8;

static s8
s8fromcstr(char* z)
{
	s8 s = {0};
    	s.s = (u8 *)z;
    	s.len = z ? strlen(z) : 0;
    	return s;
}

typedef struct {
	u8 *data;
    	size len;
    	size cap;
} stringbuilder_s;

static void
sb_init(stringbuilder_s *b, size_t init_cap)
{
    b->len = 0;
    b->cap = init_cap;
    b->data = xcalloc(1, b->cap);
}

static void
sb_append(stringbuilder_s* b, s8 str)
{
    if (b->cap < b->len + str.len)
    {
	  while (b->cap < b->len + str.len)
	  {
		b->cap *= 2;

		if (b->cap < 0)
		      abort();
	  }

	  b->data = xrealloc(b->data, b->cap);
    }

    memcpy(b->data + b->len, str.s, str.len);
    b->len += str.len;
}

static void
sb_append_c(stringbuilder_s* b, char c)
{
    if (b->cap < b->len + 1)
    {
	  while (b->cap < b->len + 1)
	  {
		b->cap *= 2;

		if (b->cap < 0)
		      abort();
	  }

	  b->data = xrealloc(b->data, b->cap);
	  if (!b->data)
		abort();
    }

    b->data[b->len++] = c;
}

static s8
sb_get_s8(stringbuilder_s sb)
{
    return (s8) { .s = sb.data, .len = sb.len };
}

char*
sb_steal_str(stringbuilder_s* sb)
{
    char* r = (char*)sb->data;
    if (sb->cap < sb->len + 1)
	  r = xrealloc(r, sb->len + 1);
    r[sb->len] = '\0';

    *sb = (stringbuilder_s) {0};
    return r;
}

static void
sb_free(stringbuilder_s* sb)
{
    free(sb->data);
    *sb = (stringbuilder_s) {0};
}

/*
 * @str: The str to be escaped
 *
 * Returns: A json escaped copy of @str
 */
static char*
json_escape_str(char const* str)
{
	if (!str)
		return NULL;

	/* Best would be to use something like glibs g_strescape, but
	it appears that ankiconnect can't handle unicode escape sequences */

	stringbuilder_s sb;
	sb_init(&sb, strlen(str) + 20); // 20 is some random initial extra space

	while(*str)
	{
		switch (*str)
		{
		case '\b':
			sb_append(&sb, s8("\\b"));
			break;
		case '\f':
			sb_append(&sb, s8("\\f"));
			break;
		case '\n':
			sb_append(&sb, s8("<br>"));
			break;
		case '\r':
			sb_append(&sb, s8("\\r"));
			break;
		case '\t':
			sb_append(&sb, s8("&#9")); // html tab
			break;
		case '\v':
			sb_append(&sb, s8("\\v"));
			break;
		case '"':
			sb_append(&sb, s8("\\\""));
			break;
		case '\\':
			sb_append(&sb, s8("\\\\"));
			break;
		default:
			sb_append_c(&sb, *str);
		}

		str++;
	}

	return sb_steal_str(&sb);
}
/* END UTILS */

typedef size_t (*ResponseFunc)(char *ptr, size_t size, size_t nmemb, void *userdata);

void
ankicard_free(ankicard ac)
{
	free(ac.deck);
	free(ac.notetype);

	//TODO: It's no longer allocated by glib
	g_strfreev(ac.fieldnames);
	g_strfreev(ac.fieldentries);
	g_strfreev(ac.tags);
}

static retval_s
sendRequest(s8 request, ResponseFunc response_checker)
{
	/* printf("req: %.*s", (int)request.len, request.s); */
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
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, request.len);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request.s);
	
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
ac_search(_Bool include_suspended, char* deck, char* field, char* entry)
{
	
	stringbuilder_s sb;
	sb_init(&sb, 200);

	sb_append(&sb, s8("{ \"action\": \"findCards\", \"version\": 6, \"params\": { \"query\" : \"\\\""));
	sb_append(&sb, s8fromcstr(deck ? "deck:" : ""));
	sb_append(&sb, s8fromcstr(deck ? deck : ""));
	sb_append(&sb, s8("\\\" \\\""));
	sb_append(&sb, s8fromcstr(field));
	sb_append_c(&sb, ':');
	sb_append(&sb, s8fromcstr(entry));
	sb_append(&sb, s8("\\\" "));
	if (!include_suspended)
		sb_append(&sb, s8("-is:suspended"));
	sb_append(&sb, s8("\" } }"));

	retval_s ret = sendRequest(sb_get_s8(sb), search_checker);
	sb_free(&sb);
	return ret;
}

retval_s
ac_gui_search(const char* deck, const char* field, const char* entry)
{
	g_autofree char *request = g_strdup_printf(
	    "{ \"action\": \"guiBrowse\", \"version\": 6, \"params\": { \"query\" : \"\\\"%s%s\\\" \\\"%s:%s\\\"\" } }",
		 deck ? "deck:" : "", deck ? deck : "", field, entry);

	return sendRequest(s8fromcstr(request), NULL);
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

	return sendRequest(s8fromcstr(request), NULL); // TODO: Error response cheking
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
	size_t num_fields = ac.num_fields;
	return (ankicard) {
	      .deck = json_escape_str(ac.deck),
	      .notetype = json_escape_str(ac.notetype),
	      .num_fields = num_fields,
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

	stringbuilder_s sb;
	sb_init(&sb, 1<<9);

	sb_append(&sb,
	  s8(
	  "{"
	     "\"action\": \"addNote\","
	     "\"version\": 6,"
	     "\"params\": {"
			    "\"note\": {"
			                "\"deckName\": \""
	    )
	);
	sb_append(&sb, s8fromcstr(ac_je.deck));
	sb_append(&sb, 
	  s8(
			                              "\","
			                "\"modelName\": \""     
	    )
	);
	sb_append(&sb, s8fromcstr(ac_je.notetype));
	sb_append(&sb,
	  s8(
			                               "\","     
			                "\"fields\": {"
	    )
	);

	for (size_t i = 0; i < ac_je.num_fields; i++)
	{
	      if(i)
		    sb_append(&sb, s8(","));
	      sb_append(&sb, s8("\""));
	      sb_append(&sb, s8fromcstr(ac_je.fieldnames[i]));
	      sb_append(&sb, s8("\" : \""));
	      sb_append(&sb, s8fromcstr(ac_je.fieldentries[i] ? ac_je.fieldentries[i] : ""));
	      sb_append(&sb, s8("\""));
	}
	
	sb_append(&sb, 
	    s8(
			                            "},"
			               "\"options\": {"
			                              "\"allowDuplicate\": true"
			                            "},"
	                               "\"tags\": ["
	      )
	);

	if (ac_je.tags)
	{
	      for (size_t i = 0; ac_je.tags[i]; i++)
	      {
		      if (i)
			    sb_append(&sb, s8(","));
		      sb_append(&sb, s8("\""));
		      sb_append(&sb, s8fromcstr(ac_je.tags[i]));
		      sb_append(&sb, s8("\""));
	      }
	}

	sb_append(&sb,
	    s8(
			                         "]"
			              "}"
			  "}"
	  "}"
	      )
	);

	retval_s ret = sendRequest(sb_get_s8(sb), check_add_response);
	sb_free(&sb);
	ankicard_free(ac_je);
	return ret;
}
