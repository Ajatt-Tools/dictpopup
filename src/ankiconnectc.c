#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <curl/curl.h>
#include <glib.h>

#include "ankiconnectc.h"

#include <util.h>

#define AC_API_URL_EVAR "ANKICONNECT_API_URL"
#define DEFAULT_AC_API_URL "http://localhost:8765"

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
            case '\v':
                sb_append(&sb, S("\\v"));
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
/* ------ END UTILS ------------ */

typedef size_t (*ResponseFunc)(char *ptr, size_t len, size_t nmemb, void *userdata);

/* ------ Callback functions ------ */
static size_t search_checker(char *ptr, size_t len, size_t nmemb, void *userdata) {
    retval_s *ret = userdata;
    assert(ret);

    *ret =
        (retval_s){.data.boolean = (strncmp(ptr, "{\"result\": [], \"error\": null}", nmemb) != 0),
                   .ok = true};

    return nmemb;
}
/* -------------------------------- */

void ac_print_ankicard(ankicard ac) {
    puts("Ankicard:");
    printf("deck: %s\n", ac.deck);
    printf("notetype: %s\n", ac.notetype);
    printf("Number of fields: %zu\n", ac.num_fields);
    printf("Fieldnames: [");
    for (size_t i = 0; i < ac.num_fields; i++) {
        if (i)
            putchar(',');
        printf("%s", ac.fieldnames[i]);
    }
    puts("]");
    printf("Contents: \n");
    for (size_t i = 0; i < ac.num_fields; i++)
        printf("%zu: %s\n", i, ac.fieldentries[i]);
    if (ac.tags) {
        for (char **tagptr = ac.tags; *tagptr; tagptr++)
            printf("%s", *tagptr);
    } else
        puts("Tags: none");
}

void ankicard_free(ankicard ac) {
    free(ac.deck);
    free(ac.notetype);

    // TODO: It's no longer allocated by glib
    g_strfreev(ac.fieldnames);
    g_strfreev(ac.fieldentries);
    g_strfreev(ac.tags);
}

static const char *get_api_url(void) {
    const char *env = getenv(AC_API_URL_EVAR); // Warning: String can change
    return env && *env ? env : DEFAULT_AC_API_URL;
}

static retval_s sendRequest(s8 request, ResponseFunc response_checker) {
    CURL *curl = curl_easy_init();
    if (!curl)
        return (retval_s){.data.string = "Error initializing cURL", .ok = false};

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
        ret = (retval_s){.data.string = "Could not connect to AnkiConnect. Is Anki running?",
                         .ok = false};
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return ret;
}

static s8 form_search_req(bool include_suspended, char *deck, char *field, char *entry) {
    return concat(S("{ \"action\": \"findCards\", \"version\": 6, \"params\": "
                    "{ \"query\" : \"\\\""),
                  fromcstr_(deck ? "deck:" : ""), fromcstr_(deck ? deck : ""), S("\\\" \\\""),
                  fromcstr_(field), S(":"), fromcstr_(entry), S("\\\" "),
                  include_suspended ? S("") : S("-is:suspended"), S("\" } }"));
}

/*
 * @entry: The entry to search for
 * @field: The field where @entry should be searched in.
 * @deck: The deck to search in. Can be null.
 *
 * Returns: Error msg on error, null otherwise
 */
retval_s ac_search(bool include_suspended, char *deck, char *field, char *entry) {

    _drop_(frees8) s8 req = form_search_req(include_suspended, deck, field, entry);
    return sendRequest(req, search_checker);
}

static s8 form_gui_search_req(const char *deck, const char *field, const char *entry) {
    return concat(
        S("{ \"action\": \"guiBrowse\", \"version\": 6, \"params\": { \"query\" : \"\\\""),
        deck ? S("deck:") : S(""), deck ? fromcstr_((char *)deck) : S(""), S("\\\" \\\""),
        fromcstr_((char *)field), S(":"), fromcstr_((char *)entry), S("\\\"\" } }"));
}

retval_s ac_gui_search(const char *deck, const char *field, const char *entry) {

    _drop_(frees8) s8 req = form_gui_search_req(deck, field, entry);
    return sendRequest(req, NULL);
}

retval_s ac_get_notetypes() {
    // TODO: Implement
    return (retval_s){0};
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
retval_s ac_store_file(char const *filename, char const *path) {

    _drop_(frees8) s8 req = form_store_file_req(filename, path);
    return sendRequest(req, NULL); // TODO: Check error response
}

static int get_array_len(const char *array[static 1]) {
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

static size_t check_add_response(char *ptr, size_t len, size_t nmemb, void *userdata) {
    assert(userdata);
    retval_s *ret = userdata;

    if (strstr(ptr, "\"error\": null"))
        *ret = (retval_s){.ok = true};
    else {
        char *err = (char *)s8dup((s8){.s = (u8 *)ptr, .len = nmemb}).s;
        *ret = (retval_s){.data.string = err, .ok = false};
        // TODO: This is a memory leak
    }

    return nmemb;
}

static char *check_card(ankicard const ac) {
    return !ac.deck           ? "No deck specified."
           : !ac.notetype     ? "No notetype specified."
           : !ac.num_fields   ? "Number of fields is 0"
           : !ac.fieldnames   ? "No fieldnames provided."
           : !ac.fieldentries ? "No fieldentries provided."
                              : NULL;
}

retval_s ac_addNote(ankicard const ac) {
    if (check_card(ac))
        return (retval_s){.data.string = check_card(ac), .ok = false};

    ankicard ac_je = ankicard_dup_json_esc(ac);

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

    retval_s ret = sendRequest(sb_gets8(sb), check_add_response);
    sb_free(&sb);
    ankicard_free(ac_je);
    return ret;
}
