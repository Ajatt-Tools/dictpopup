#include <stdbool.h>
#include <string.h>

#include "ankiconnectc.h"
#include <glib.h>

#include "utils/messages.h"

#include "ankiconnectc/send_request.h"

static int get_array_len(char *array[static 1]) {
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
static char *json_escape_str(char *str) {
    if (!str)
        return NULL;

    stringbuilder_s sb = sb_init(strlen(str) + 20);

    while (*str) {
        switch (*str) {
            case '\b':
                sb_append(&sb, S("\\b"));
                break;
            case '\f':
                sb_append(&sb, S("\\f"));
                break;
            case '\r':
                sb_append(&sb, S("\\r"));
                break;
            case '"':
                sb_append(&sb, S("\\\""));
                break;
            case '\\':
                sb_append(&sb, S("\\\\"));
                break;
            // Anki escape
            case '\t':
                sb_append(&sb, S("&#9")); // html tab
                break;
            case '\n':
                sb_append(&sb, S("<br>"));
                break;
            // TODO: If we escape '<' and '>', bold tags won't work anymore
            // case '<':
            // sb_append(&sb, S("&lt;"));
            // break;
            // case '>':
            // sb_append(&sb, S("&gt;"));
            // break;
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
static char **json_escape_str_array(int n, char **array) {
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
static size_t search_checker(char *ptr, size_t size, size_t nmemb, void *userdata) {
    assert(userdata);
    retval_s *ret = userdata;
    s8 resp = (s8){.s = (u8 *)ptr, .len = nmemb};

    *ret = (retval_s){.data.boolean = !s8equals(resp, S("{\"result\": [], \"error\": null}")),
                      .ok = true};

    return size * nmemb;
}

static size_t check_add_response(char *ptr, size_t size, size_t nmemb, void *userdata) {
    assert(userdata);
    retval_s *ret = userdata;
    s8 resp = (s8){.s = (u8 *)ptr, .len = nmemb};

    if (endswith(resp, S("\"error\": null}")))
        *ret = (retval_s){.ok = true};
    else {
        char *err = (char *)s8dup(resp).s;
        *ret = (retval_s){.data.string = err, .ok = false};
    }

    return size * nmemb;
}
/* ------- End Callback functions ----------- */

void ankicard_free(AnkiCard *ac) {
    free(ac->deck);
    free(ac->notetype);

    // TODO: It's no longer allocated by glib
    g_strfreev(ac->fieldnames);
    g_strfreev(ac->fieldentries);
    g_strfreev(ac->tags);
}

DEFINE_DROP_FUNC_PTR(AnkiCard, ankicard_free)

bool ac_check_connection(void) {
    s8 req = S("{\"action\": \"version\", \"version\": 6}");
    retval_s ret = sendRequest(req, NULL);

    // TODO: Switch to error parameter
    if (!ret.ok) {
        free(ret.data.string);
        return false;
    }
    return true;
}

static s8 form_search_req(bool include_suspended, bool include_new, char *deck, char *field,
                          char *entry) {
    return concat(S("{ \"action\": \"findCards\", \"version\": 6, \"params\": "
                    "{ \"query\" : \"\\\""),
                  fromcstr_(deck ? "deck:" : ""), fromcstr_(deck ? deck : ""), S("\\\" \\\""),
                  fromcstr_(field), S(":"), fromcstr_(entry), S("\\\""),
                  include_suspended ? S("") : S(" -is:suspended"),
                  include_new ? S("") : S(" -is:new"), S("\" } }"));
}

static int check_exists_with_(bool include_suspended, bool include_new, char *deck, char *field,
                              char *str, char **error) {
    _drop_(frees8) s8 req = form_search_req(include_suspended, include_new, deck, field, str);
    retval_s r = sendRequest(req, search_checker);
    if (!r.ok) {
        if (error)
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
 */
AnkiCollectionStatus ac_check_exists(char *deck, char *field, char *lookup, char **error) {
    if (!lookup) {
        if (error)
            *error = strdup("Lookup string called with NULL value in ac_check_exists!");
        return AC_ERROR;
    }

    int rc = check_exists_with_(false, false, deck, field, lookup, error);
    if (rc == 1)
        return AC_EXISTS;
    if (rc == -1)
        return AC_ERROR;

    rc = check_exists_with_(false, true, deck, field, lookup, error);
    if (rc == 1)
        return AC_EXISTS_NEW;
    if (rc == -1)
        return AC_ERROR;

    rc = check_exists_with_(true, false, deck, field, lookup, error);
    if (rc == 1)
        return AC_EXISTS_SUSPENDED;
    if (rc == -1)
        return AC_ERROR;

    return AC_DOES_NOT_EXIST;
}

static s8 form_gui_search_req(char *deck, char *field, char *entry) {
    return concat(
        S("{ \"action\": \"guiBrowse\", \"version\": 6, \"params\": { \"query\" : \"\\\""),
        deck ? S("deck:") : S(""), deck ? fromcstr_((char *)deck) : S(""), S("\\\" \\\""),
        fromcstr_((char *)field), S(":"), fromcstr_((char *)entry), S("\\\"\" } }"));
}

void ac_gui_search(char *deck, char *field, char *entry, char **error) {

    _drop_(frees8) s8 req = form_gui_search_req(deck, field, entry);
    retval_s r = sendRequest(req, NULL);

    if (!r.ok && error)
        *error = r.data.string;
}

s8 *ac_get_notetypes(char **error) {
    // TODO: Implement
    return 0;
}

static s8 form_store_file_req(char *filename, char *path) {
    return concat(
        S("{ \"action\": \"storeMediaFile\", \"version\": 6, \"params\": { \"filename\" : \""),
        fromcstr_((char *)filename), S("\", \"path\": \""), fromcstr_((char *)path),
        S("\", \"deleteExisting\": false } }"));
}

/*
 * Stores the file at @path in the Anki media collection under the name
 * @filename. Doesn't overwrite existing files.
 */
void ac_store_file(char *filename, char *path, char **error) {

    _drop_(frees8) s8 req = form_store_file_req(filename, path);
    retval_s r = sendRequest(req, NULL); // TODO: Check error response
    if (!r.ok && error)
        *error = r.data.string;
}

/*
 * Creates a json escaped and \n -> <br> converted copy of the anki card.
 */
static AnkiCard ankicard_dup_json_esc(AnkiCard ac) {
    size_t num_fields = ac.num_fields;
    return (AnkiCard){.deck = json_escape_str(ac.deck),
                      .notetype = json_escape_str(ac.notetype),
                      .num_fields = num_fields,
                      .fieldnames = json_escape_str_array(num_fields, ac.fieldnames),
                      .fieldentries = json_escape_str_array(num_fields, ac.fieldentries),
                      .tags = ac.tags ? json_escape_str_array(-1, ac.tags) : NULL};
}

static s8 form_addNote_req(AnkiCard ac) {
    _drop_(ankicard_free) AnkiCard ac_je = ankicard_dup_json_esc(ac);

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

static char *check_card(AnkiCard ac) {
    return !ac.deck           ? "No deck specified."
           : !ac.notetype     ? "No notetype specified."
           : !ac.num_fields   ? "Number of fields is 0"
           : !ac.fieldnames   ? "No fieldnames provided."
           : !ac.fieldentries ? "No fieldentries provided."
                              : NULL;
}

void ac_addNote(AnkiCard ac, char **error) {
    *error = check_card(ac);
    if (*error)
        return;

    _drop_(frees8) s8 req = form_addNote_req(ac);
    retval_s r = sendRequest(req, check_add_response);
    if (!r.ok)
        *error = r.data.string;
}
