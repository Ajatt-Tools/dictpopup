#include <errno.h>
#include <signal.h> // Catch SIGINT
#include <stdio.h>
#include <unistd.h> // getopt

#include <dirent.h> // opendir
#include <zip.h>

#include "db.h"
#include "messages.h"
#include "pdjson.h"
#include "platformdep.h"
#include "util.h"

typedef struct {
    s8 dbpath;
    s8 dictspath;
} config;

#define error_return(retval, ...)                                                                  \
    do {                                                                                           \
        err(__VA_ARGS__);                                                                          \
        return retval;                                                                             \
    } while (0)

#define return_on(cond, fmt, ...)                                                                  \
    do {                                                                                           \
        if (unlikely(cond)) {                                                                      \
            msg(fmt, ##__VA_ARGS__);                                                               \
            return;                                                                                \
        }                                                                                          \
    } while (0)

DEFINE_DROP_FUNC_PTR(json_stream, json_close)
DEFINE_DROP_FUNC(struct zip_file *, zip_fclose)

volatile sig_atomic_t sigint_received = 0;

static const char json_typename[][16] = {
    [JSON_ERROR] = "ERROR",           [JSON_DONE] = "DONE",     [JSON_OBJECT] = "OBJECT",
    [JSON_OBJECT_END] = "OBJECT_END", [JSON_ARRAY] = "ARRAY",   [JSON_ARRAY_END] = "ARRAY_END",
    [JSON_STRING] = "STRING",         [JSON_NUMBER] = "NUMBER", [JSON_TRUE] = "TRUE",
    [JSON_FALSE] = "FALSE",           [JSON_NULL] = "NULL",
};

enum tag_type { TAG_UNKNOWN, TAG_DIV, TAG_SPAN, TAG_UL, TAG_OL, TAG_LI, TAG_RUBY_RT };

static void frequency_entry_free(frequency_entry *fe) {
    frees8(&(fe->word));
    frees8(&(fe->reading));
}

static void append_definition(json_stream s[static 1], stringbuilder_s sb[static 1], s8 liststyle,
                              i32 listdepth);

static s8 unzip_file(zip_t *archive, const char *filename) {
    struct zip_stat finfo;
    zip_stat_init(&finfo);
    if (zip_stat(archive, filename, 0, &finfo) == -1)
        error_return((s8){0}, "Error stat'ing file '%s': %s\n", filename, zip_strerror(archive));

    s8 buffer = news8(finfo.size); // automatically null-terminated

    _drop_(zip_fclose) struct zip_file *index_file = zip_fopen(archive, filename, 0);
    if (!index_file)
        error_return((s8){0}, "Error opening file '%s': %s\n", filename, zip_strerror(archive));

    buffer.len = zip_fread(index_file, buffer.s, buffer.len);
    if (buffer.len == -1)
        error_return((s8){0}, "Could not read file '%s': %s", filename, zip_strerror(archive));
    if ((size_t)buffer.len != finfo.size)
        dbg("Did not read all of %s!", filename);

    return buffer;
}

static s8 json_get_string_(json_stream *json) {
    size_t slen = 0;
    s8 r = {0};
    r.s = (u8 *)json_get_string(json, &slen);
    r.len = slen > 0 ? (size)(slen - 1) : 0; // API includes terminating 0 in length
    return r;
}

static void add_dictent_to_db(database_t *db, dictentry de) {
    if (de.definition.len == 0)
        dbg("Could not parse a definition for entry with kanji: \"%s\" and "
            "reading: \"%s\". In dictionary: \"%s\"",
            (char *)de.kanji.s, (char *)de.reading.s, (char *)de.dictname.s);
    else if (de.kanji.len == 0 && de.reading.len == 0)
        dbg("Could not obtain kanji nor reading for entry with definition: "
            "\"%s\", in dictionary: \"%s\"",
            (char *)de.definition.s, (char *)de.dictname.s);
    else {
        if (de.kanji.len > 0)
            db_put_dictent(db, de.kanji, de);
        if (de.reading.len > 0) // Let the db figure out duplicates
            db_put_dictent(db, de.reading, de);
    }
}

static s8 parse_liststyletype(s8 lst) {
    // TODO: This is a memory leak
    if (startswith(lst, S("\"")) && endswith(lst, S("\"")))
        return s8dup(cuttail(cuthead(lst, 1), 1));
    else if (s8equals(lst, S("circle")))
        return S("◦");
    else if (s8equals(lst, S("square")))
        return S("▪");
    else if (s8equals(lst, S("none")))
        return S("  ");
    else
        return s8dup(lst); // そのまま
}

static void append_structured_content(json_stream *s, stringbuilder_s sb[static 1], s8 liststyle,
                                      i32 listdepth) {
    enum json_type type;
    enum tag_type tag = TAG_UNKNOWN;
    for (;;) {
        type = json_next(s);

        if (type == JSON_OBJECT_END || type == JSON_DONE)
            break;
        else if (type == JSON_ERROR) {
            err("%s", json_get_error(s));
            break;
        } else if (type == JSON_STRING) {
            s8 value = json_get_string_(s);

            if (s8equals(value, S("tag"))) {
                type = json_next(s);
                assert(type == JSON_STRING);
                value = json_get_string_(s);

                // Content needs to come last for this to work
                // TODO: Switch to a hash table
                if (s8equals(value, S("div")))
                    tag = TAG_DIV;
                else if (s8equals(value, S("ul"))) {
                    tag = TAG_UL;
                    listdepth++;

                    liststyle = S("•"); // default
                } else if (s8equals(value, S("ol"))) {
                    tag = TAG_OL;
                    listdepth++;
                } else if (s8equals(value, S("li"))) {
                    tag = TAG_LI;
                    // TODO: Default numbers for tag ol
                } else if (s8equals(value, S("rt"))) {
                    tag = TAG_RUBY_RT;
                } else
                    tag = TAG_UNKNOWN;
            } else if ((tag == TAG_UL || tag == TAG_LI) && s8equals(value, S("style"))) {
                // TODO: json_peek for the very unlikely event that there is an array
                type = json_next(s);

                if (type == JSON_OBJECT) {
                    type = json_next(s);
                    while (type != JSON_OBJECT_END && type != JSON_DONE && type != JSON_ERROR) {
                        if (type == JSON_STRING &&
                            s8equals(json_get_string_(s), S("listStyleType"))) {
                            type = json_next(s);
                            assert(type == JSON_STRING);
                            liststyle = parse_liststyletype(json_get_string_(s));
                        } else
                            json_skip(s);

                        type = json_next(s);
                    }
                } else if (type == JSON_ARRAY)
                    assert(0);
                else
                    dbg("Could not parse style of list. Skipping..");
            } else if (s8equals(value, S("content"))) {
                if (tag == TAG_RUBY_RT) {
                    json_skip(s);
                    continue;
                }

                if (tag == TAG_DIV || tag == TAG_LI) {
                    if (sb->len != 0 && sb->data[sb->len - 1] != '\n')
                        sb_append(sb, S("\n"));
                }

                if (tag == TAG_LI) {
                    for (i32 i = 0; i < listdepth - 1; i++)
                        sb_append(sb, S("\t"));
                    sb_append(sb, liststyle);
                    sb_append(sb, S(" "));
                }

                append_definition(s, sb, liststyle, listdepth);
            } else
                json_skip(s);
        } else {
            dbg("Encountered unknown type '%s' in structured content. Skipping..",
                json_typename[type]);
            json_skip(s);
        }
    }

    /* if (list) frees8(&liststyle); */
    assert(type == JSON_OBJECT_END);
}

static void append_definition(json_stream s[static 1], stringbuilder_s sb[static 1], s8 liststyle,
                              i32 listdepth) {
    enum json_type type = json_next(s);

    if (type == JSON_ARRAY) {
        type = json_next(s);

        while (type != JSON_ARRAY_END && type != JSON_ERROR && type != JSON_DONE) {
            if (type == JSON_STRING) {
                sb_append(sb, json_get_string_(s));
            } else if (type == JSON_OBJECT)
                append_structured_content(s, sb, liststyle, listdepth);
            else
                dbg("Encountered unknown type '%s' within definition.", json_typename[type]);

            if (json_get_depth(s) == 3)
                sb_append_char(sb, '\n');

            type = json_next(s);
        }

        assert(type == JSON_ARRAY_END);
    } else if (type == JSON_STRING)
        sb_append(sb, json_get_string_(s));
    else if (type == JSON_OBJECT)
        append_structured_content(s, sb, liststyle, listdepth);
    else
        dbg("Encountered unknown type '%s' at start of definition.", json_typename[type]);
}

static void freedictentry_(dictentry de[static 1]) {
    // dictname will be reused
    frees8(&de->kanji);
    frees8(&de->reading);
    frees8(&de->definition);
}

static s8 parse_definition(json_stream *s) {
    stringbuilder_s sb = sb_init(100);

    append_definition(s, &sb, (s8){0}, 0);

    s8 def = sb_steals8(sb);
    strip_trailing_whitespace(&def);
    return def;
}

static dictentry parse_dictionary_entry(json_stream *s) {
    enum json_type type;
    dictentry de = {0};

    /* first entry */
    type = json_next(s);
    assert(type == JSON_STRING);
    de.kanji = s8dup(json_get_string_(s));
    /* ----------- */

    /* second entry */
    type = json_next(s);
    assert(type == JSON_STRING);
    de.reading = s8dup(json_get_string_(s));
    /* ----------- */

    /* third entry */
    type = json_next(s);
    assert(type == JSON_STRING || type == JSON_NULL);
    // Skip
    /* ----------- */

    /* fourth entry */
    type = json_next(s);
    assert(type == JSON_STRING);
    // Skip
    /* ----------- */

    /* fifth entry */
    type = json_next(s);
    assert(type == JSON_NUMBER);
    // Skip
    /* ----------- */

    /* sixth entry */
    de.definition = parse_definition(s);
    /* ----------- */

    /* seventh entry */
    type = json_next(s);
    assert(type == JSON_NUMBER);
    // Skip
    /* ----------- */

    /* eighth entry */
    type = json_next(s);
    assert(type == JSON_STRING);
    // Skip
    /* ----------- */

    json_skip_until(s, JSON_ARRAY_END);
    return de;
}

static void add_dictionary(database_t *db, s8 buffer, s8 dictname) {
    _drop_(json_close) json_stream s;
    json_open_buffer(&s, buffer.s, buffer.len);

    enum json_type type = json_next(&s);
    return_on(type != JSON_ARRAY, "Dictionary format of '%.*s' not supported.", (int)dictname.len,
              (char *)dictname.s);

    while (!sigint_received) {
        type = json_next(&s);

        if (type == JSON_DONE)
            break;
        else if (type == JSON_ERROR) {
            err("%s", json_get_error(&s));
            break;
        } else if (type == JSON_ARRAY) {
            dictentry de = parse_dictionary_entry(&s);
            de.dictname = dictname;
            add_dictent_to_db(db, de);

            freedictentry_(&de);
        } else {
            dbg("Encountered unknown entry while parsing dictionary");
        }
    }
}

static frequency_entry parse_frequency_entry(json_stream *s) {
    frequency_entry fe = {0};

    /* first entry */
    enum json_type type = json_next(s);
    assert(type == JSON_STRING);
    fe.word = s8dup(json_get_string_(s));
    /* ----------- */

    /* second entry */
    type = json_next(s);
    assert(type == JSON_STRING);
    if (!s8equals(json_get_string_(s), S("freq"))) {
        dbg("Entry is not a frequency");

        json_skip_until(s, JSON_ARRAY_END);
        frees8(&fe.word);
        return (frequency_entry){0};
    }
    /* ----------- */

    /* third entry */
    // INFO: No handling for no reading or no freqency
    type = json_next(s);
    if (type == JSON_OBJECT) {
        type = json_next(s);
        while (type != JSON_OBJECT_END && type != JSON_DONE && type != JSON_ERROR) {
            if (type == JSON_STRING && s8equals(json_get_string_(s), S("reading"))) {
                type = json_next(s);
                assert(type == JSON_STRING);
                fe.reading = s8dup(json_get_string_(s));
            } else if (type == JSON_STRING && s8equals(json_get_string_(s), S("frequency"))) {
                type = json_next(s);
                assert(type == JSON_NUMBER);
                fe.frequency = atoi(json_get_string(s, NULL));
            } else {
                dbg("Skipping unknown frequency entry");
                json_skip(s);
            }

            type = json_next(s);
        }

        assert(type == JSON_OBJECT_END);
    } else if (type == JSON_NUMBER)
        fe.frequency = atoi(json_get_string(s, NULL));
    else {
        dbg("Unexpected frequency entry format");

        json_skip_until(s, JSON_ARRAY_END);
        frees8(&fe.word);
        return (frequency_entry){0};
    }
    /* ----------- */

    json_skip_until(s, JSON_ARRAY_END);
    return fe;
}

static void add_frequency(database_t *db, s8 buffer) {
    _drop_(json_close) json_stream s;
    json_open_buffer(&s, buffer.s, buffer.len);

    enum json_type type = json_next(&s);
    return_on(type != JSON_ARRAY, "Format of frequency dictionary not supported");

    while (!sigint_received) {
        type = json_next(&s);

        if (type == JSON_ARRAY_END || type == JSON_DONE)
            break;
        else if (type == JSON_ERROR) {
            err("%s", json_get_error(&s));
            break;
        } else if (type == JSON_ARRAY) {
            frequency_entry fe = parse_frequency_entry(&s);
            db_put_freq(db, fe);
            frequency_entry_free(&fe);
        } else
            assert(0);
    }
}

static s8 extract_dictname(zip_t *archive) {
    _drop_(frees8) s8 buffer = unzip_file(archive, "index.json");
    if (!buffer.len)
        return (s8){0};

    _drop_(json_close) json_stream s;
    json_open_buffer(&s, buffer.s, buffer.len);

    s8 dictname = {0};
    for (;;) {
        enum json_type type = json_next(&s);
        if (type == JSON_STRING) {
            s8 value = json_get_string_(&s);
            if (s8equals(value, S("title"))) {
                json_next(&s);
                dictname = s8dup(json_get_string_(&s));
                break;
            } else
                json_skip(&s);
        }
    }

    return dictname;
}

static zip_t *open_zip(char *filename) {
    int err = 0;
    zip_t *za;
    if ((za = zip_open(filename, 0, &err)) == NULL) {
        zip_error_t error;
        zip_error_init_with_code(&error, err);
        err("Cannot open zip archive '%s': %s\n", filename, zip_error_strerror(&error));
        zip_error_fini(&error);
        return NULL;
    }

    return za;
}

static void add_from_zip(database_t *db, char *filename) {
    zip_t *za = open_zip(filename);
    if (!za)
        return;

    _drop_(frees8) s8 dictname = extract_dictname(za);
    printf("Processing dictionary: %s\n", (char *)dictname.s);

    struct zip_stat finfo;
    zip_stat_init(&finfo);
    for (int i = 0; (zip_stat_index(za, i, 0, &finfo)) == 0 && !sigint_received; i++) {
        s8 fn = fromcstr_((char *)finfo.name); // Warning: Casts away const!
        if (startswith(fn, S("term_bank_"))) {
            _drop_(frees8) s8 buffer = unzip_file(za, finfo.name);
            if (!buffer.len)
                continue;

            add_dictionary(db, buffer, dictname);
        } else if (startswith(fn, S("term_meta_bank"))) {
            _drop_(frees8) s8 buffer = unzip_file(za, finfo.name);
            if (!buffer.len)
                continue;

            add_frequency(db, buffer);
        }
    }
}

/*
 * Dialog which asks the user for confirmation (Yes/No)
 */
static bool askyn(const char *msg) {
    return true;
}

static void sigint_handler(int s) {
    sigint_received = 1;
}

static void setup_sighandler(void) {
    struct sigaction act;
    act.sa_handler = sigint_handler;
    sigaction(SIGINT, &act, NULL);
}

static void parse_cmd_line(int argc, char **argv, config *cfg) {
    int c;
    opterr = 0;

    while ((c = getopt(argc, argv, "hd:i:")) != -1)
        switch (c) {
            case 'd':
                cfg->dbpath = s8dup(fromcstr_(optarg));
                break;
            case 'i':
                cfg->dictspath = s8dup(fromcstr_(optarg));
                break;
            case 'h':
                puts("See 'man dictpopup-create' for help.");
                exit(EXIT_SUCCESS);
            case '?':
                fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
                exit(EXIT_FAILURE);
            default:
                abort();
        }
}

static void set_default_values(config *cfg) {
    if (!cfg->dbpath.len) {
        cfg->dbpath = buildpath(fromcstr_((char *)get_user_data_dir()), S("dictpopup"));
        msg("Storing database in: %.*s", (int)cfg->dbpath.len, (char *)cfg->dbpath.s);
    }

    if (!cfg->dictspath.len) {
        msg("Using current directory as dictionary path.");
        cfg->dictspath = S(".");
    }
}

#ifndef UNIT_TEST
int main(int argc, char *argv[]) {
    setup_sighandler();

    config cfg = {0};
    parse_cmd_line(argc, argv, &cfg);
    set_default_values(&cfg);

    if (db_check_exists(cfg.dbpath)) {
        if (askyn("A database file already exists. Would you like to delete the old one?"))
            db_remove(cfg.dbpath);
        else
            exit(EXIT_FAILURE);
    } else
        createdir((char *)cfg.dbpath.s);

    _drop_(db_close) database_t *db = db_open((char *)cfg.dbpath.s, false);
    _drop_(closedir) DIR *dir = opendir((char *)cfg.dictspath.s);
    die_on(!dir, "Error opening current directory: %s", strerror(errno));

    struct dirent *entry;
    while ((entry = readdir(dir)) && !sigint_received) {
        _drop_(frees8) s8 fn = buildpath(cfg.dictspath, fromcstr_(entry->d_name));
        if (endswith(fn, S(".zip")))
            add_from_zip(db, (char *)fn.s);
    }
}
#endif