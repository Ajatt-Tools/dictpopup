#include <stdbool.h>
#include <string.h>

#include <curl/curl.h>
#include <glib.h>

#include "ankiconnectc.h"

#include <util.h>

#define AC_API_URL_EVAR "ANKICONNECT_API_URL"
#define DEFAULT_AC_API_URL "http://localhost:8765"

typedef size_t (*ResponseFunc)(char *ptr, size_t len, size_t nmemb, void *userdata);

typedef struct {
    union {
        char *string;
        char **stringv;
        _Bool boolean;
    } data;
    _Bool ok; // Signalizes if there was an error on not. The error msg is
    // stored in data.string
} retval_s;

static int get_array_len(const char *array[static 1]) {
    int n = 0;
    while (*array++)
        n++;
    return n;
}

/*
 * @str: The str to be escaped
 *
 * Returns: A json escaped copy of @str. The data is owned by the caller.
 */
static char *json_escape_str(const char *str) {
    if (!str)
        return NULL;

    /* Best would be to use something like glibs g_strescape, but
    it appears that ankiconnect can't handle unicode escape sequences */

    stringbuilder_s sb = sb_init(strlen(str) + 20);

    while (*str) {
        switch (*str) {
            case '\b':
                sb_append(&sb, S("\\b"));
                break;
            case '\f':
                sb_append(&sb, S("\\f"));
                break;
            case '\n':
                sb_append(&sb, S("<br>"));
                break;
            case '\r':
                sb_append(&sb, S("\\r"));
                break;
            case '\t':
                sb_append(&sb, S("&#9")); // html tab
                break;
            case '"':
                sb_append(&sb, S("\\\""));
                break;
            case '\\':
                sb_append(&sb, S("\\\\"));
                break;
            default:
                sb_append_char(&sb, *str);
        }

        str++;
    }

    return sb_steal_str(&sb);
}

/*
 * @array: An array of strings to be json escaped
 * @n: number of elements in array or -1 if @array is null terminated.
 *
 * Returns: A null terminated copy of @array with every string json escaped.
 */
static char **json_escape_str_array(int n, const char **array) {
    if (!array)
        return NULL;

    if (n < 0)
        n = get_array_len(array);

    char **r = new (char *, n + 1);
    for (int i = 0; i < n; i++)
        r[i] = json_escape_str(array[i]);
    r[n] = NULL;

    return r;
}

/* ------ Callback functions ------ */
static size_t search_checker(char *ptr, size_t len, size_t nmemb, void *userdata) {
    assert(userdata);
    retval_s *ret = userdata;
    s8 resp = (s8){.s = (u8 *)ptr, .len = nmemb};

    *ret = (retval_s){.data.boolean = !s8equals(resp, S("{\"result\": [], \"error\": null}")),
                      .ok = true};

    return nmemb;
}

static size_t check_add_response(char *ptr, size_t len, size_t nmemb, void *userdata) {
    assert(userdata);
    retval_s *ret = userdata;
    s8 resp = (s8){.s = (u8 *)ptr, .len = nmemb};

    if (endswith(resp, S("\"error\": null}")))
        *ret = (retval_s){.ok = true};
    else {
        char *err = (char *)s8dup(resp).s;
        *ret = (retval_s){.data.string = err, .ok = false};
    }

    return nmemb;
}
/* ------- End Callback functions ----------- */

void ankicard_free(ankicard *ac) {
    free(ac->deck);
    free(ac->notetype);

    // TODO: It's no longer allocated by glib
    g_strfreev(ac->fieldnames);
    g_strfreev(ac->fieldentries);
    g_strfreev(ac->tags);
}

DEFINE_DROP_FUNC_PTR(ankicard, ankicard_free)

static const char *get_api_url(void) {
    const char *env = getenv(AC_API_URL_EVAR); // Warning: String can change
    return env && *env ? env : DEFAULT_AC_API_URL;
}

static retval_s sendRequest(s8 request, ResponseFunc response_checker) {
    CURL *curl = curl_easy_init();
    if (!curl)
        return (retval_s){.data.string = strdup("Error initializing cURL"), .ok = false};

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, get_api_url());
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, request.len);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request.s);

    retval_s ret = {.ok = true}; // Maybe add a generic checker instead of
                                 // defaulting to true
    if (response_checker) {
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, response_checker);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ret);
    }

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        ret = (retval_s){.data.string = strdup("Could not connect to AnkiConnect. Is Anki running?"),
                         .ok = false};
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return ret;
}

bool ac_check_connection(void) {
    s8 req = S("{\"action\": \"version\", \"version\": 6}");
    retval_s ret = sendRequest(req, NULL);
    return ret.ok;
}

static s8 form_search_req(bool include_suspended, bool include_new, char *deck, char *field,
                          char *entry) {
    return concat(S("{ \"action\": \"findCards\", \"version\": 6, \"params\": "
                    "{ \"query\" : \"\\\""),
                  fromcstr_(deck ? "deck:" : ""), fromcstr_(deck ? deck : ""), S("\\\" \\\""),
                  fromcstr_(field), S(":"), fromcstr_(entry), S("\\\""),
                  include_suspended ? S("") : S(" -is:suspended"),
                  include_new ? S("") : S(" -is:new"),
                  S("\" } }"));
}

static int ac_check_exists_with(bool include_suspended, bool include_new, char *deck, char *field,
                                 char *str, char **error) {
    _drop_(frees8) s8 req = form_search_req(include_suspended, include_new, deck, field, str);
    retval_s r = sendRequest(req, search_checker);
    if (!r.ok) {
        *error = r.data.string;
        return -1;
    }

    return r.data.boolean ? 1 : 0;
}

/**
 * @deck: The deck to search in. Can be null.
 * @field: The field where @entry should be searched in.
 * @str: The string to search for
 *
 * Returns: 1 if cards with @str as @field exist, which are not suspended and not nerw
 *          2 if there are new cards with @str as @field (and maybe also suspended cards)
 *          3 if there are only suspended cards with @str as @field
 *          -1 on error
 */
int ac_check_exists(char *deck, char *field, char *str, char **error) {
    int rc = ac_check_exists_with(false, false, deck, field, str, error);
    if (rc == -1)
        return -1;
    if (rc == 1)
        return 1;

    rc = ac_check_exists_with(false, true, deck, field, str, error);
    if (rc == -1)
        return -1;
    if (rc == 1)
        return 2;

    rc = ac_check_exists_with(true, true, deck, field, str, error);
    if (rc == -1)
        return -1;
    if (rc == 1)
        return 3;

    return 0;
}

static s8 form_gui_search_req(const char *deck, const char *field, const char *entry) {
    return concat(
        S("{ \"action\": \"guiBrowse\", \"version\": 6, \"params\": { \"query\" : \"\\\""),
        deck ? S("deck:") : S(""), deck ? fromcstr_((char *)deck) : S(""), S("\\\" \\\""),
        fromcstr_((char *)field), S(":"), fromcstr_((char *)entry), S("\\\"\" } }"));
}

void ac_gui_search(const char *deck, const char *field, const char *entry, char **error) {

    _drop_(frees8) s8 req = form_gui_search_req(deck, field, entry);
    retval_s r = sendRequest(req, NULL);
    if (!r.ok)
        *error = r.data.string;
}

s8 *ac_get_notetypes(char **error) {
    // TODO: Implement
    return 0;
}

static s8 form_store_file_req(char const *filename, char const *path) {
    return concat(
        S("{ \"action\": \"storeMediaFile\", \"version\": 6, \"params\": { \"filename\" : \""),
        fromcstr_((char *)filename), S("\", \"path\": \""), fromcstr_((char *)path),
        S("\", \"deleteExisting\": false } }"));
}

/*
 * Stores the file at @path in the Anki media collection under the name
 * @filename. Doesn't overwrite existing files.
 */
void ac_store_file(char const *filename, char const *path, char **error) {

    _drop_(frees8) s8 req = form_store_file_req(filename, path);
    retval_s r = sendRequest(req, NULL); // TODO: Check error response
    if(!r.ok)
        *error = r.data.string;
}

/*
 * Creates a json escaped and \n -> <br> converted copy of the anki card.
 */
static ankicard ankicard_dup_json_esc(const ankicard ac) {
    size_t num_fields = ac.num_fields;
    return (ankicard){.deck = json_escape_str(ac.deck),
                      .notetype = json_escape_str(ac.notetype),
                      .num_fields = num_fields,
                      .fieldnames = json_escape_str_array(num_fields, (const char **)ac.fieldnames),
                      .fieldentries =
                          json_escape_str_array(num_fields, (const char **)ac.fieldentries),
                      .tags = ac.tags ? json_escape_str_array(-1, (const char **)ac.tags) : NULL};
}

static char *check_card(ankicard const ac) {
    return !ac.deck           ? "No deck specified."
           : !ac.notetype     ? "No notetype specified."
           : !ac.num_fields   ? "Number of fields is 0"
           : !ac.fieldnames   ? "No fieldnames provided."
           : !ac.fieldentries ? "No fieldentries provided."
                              : NULL;
}

static s8 form_addNote_req(ankicard ac) {
    _drop_(ankicard_free) ankicard ac_je = ankicard_dup_json_esc(ac);

    stringbuilder_s sb = sb_init(1 << 9);

    sb_append(&sb, S("{"
                     "\"action\": \"addNote\","
                     "\"version\": 6,"
                     "\"params\": {"
                     "\"note\": {"
                     "\"deckName\": \""));
    sb_append(&sb, fromcstr_(ac_je.deck));
    sb_append(&sb, S("\","
                     "\"modelName\": \""));
    sb_append(&sb, fromcstr_(ac_je.notetype));
    sb_append(&sb, S("\","
                     "\"fields\": {"));

    assert(ac_je.fieldnames && ac_je.fieldentries);
    for (size_t i = 0; i < ac_je.num_fields; i++) {
        if (i)
            sb_append(&sb, S(","));
        sb_append(&sb, S("\""));
        sb_append(&sb, fromcstr_(ac_je.fieldnames[i]));
        sb_append(&sb, S("\" : \""));
        sb_append(&sb, fromcstr_(ac_je.fieldentries[i] ? ac_je.fieldentries[i] : ""));
        sb_append(&sb, S("\""));
    }

    sb_append(&sb, S("},"
                     "\"options\": {"
                     "\"allowDuplicate\": true"
                     "},"
                     "\"tags\": ["));

    if (ac_je.tags) {
        for (size_t i = 0; ac_je.tags[i]; i++) {
            if (i)
                sb_append(&sb, S(","));
            sb_append(&sb, S("\""));
            sb_append(&sb, fromcstr_(ac_je.tags[i]));
            sb_append(&sb, S("\""));
        }
    }

    sb_append(&sb, S("]"
                     "}"
                     "}"
                     "}"));

    return sb_steals8(sb);
}

void ac_addNote(ankicard const ac, char **error) {
    *error = check_card(ac);
    if (*error)
        return;

    _drop_(frees8) s8 req = form_addNote_req(ac);
    retval_s r = sendRequest(req, check_add_response);
    if(!r.ok)
        *error = r.data.string;
}
