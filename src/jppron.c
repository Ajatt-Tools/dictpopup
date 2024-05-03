#include <alloca.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h> // mkdir
#include <unistd.h>   // access

#include <glib.h>

#include "database.h"
#include "deinflector.h"
#include "jppron.h"
#include "messages.h"
#include "pdjson.h"
#include "platformdep.h" // for play_audio
#include "util.h"

// For access()
#ifdef _WIN32
    #include <io.h>
    #define F_OK 0
    #define access _access
#endif

typedef struct {
    s8 origin;
    s8 hira_reading;
    s8 pitch_number;
    s8 pitch_pattern;
} fileinfo;

const char json_typename[][16] = {
    [JSON_ERROR] = "ERROR",           [JSON_DONE] = "DONE",     [JSON_OBJECT] = "OBJECT",
    [JSON_OBJECT_END] = "OBJECT_END", [JSON_ARRAY] = "ARRAY",   [JSON_ARRAY_END] = "ARRAY_END",
    [JSON_STRING] = "STRING",         [JSON_NUMBER] = "NUMBER", [JSON_TRUE] = "TRUE",
    [JSON_FALSE] = "FALSE",           [JSON_NULL] = "NULL",
};

static void print_fileinfo(fileinfo fi) {
    printf("Source: %.*s\nReading: %.*s\nPitch number: %.*s\nPitch pattern: "
           "%.*s\n\n",
           (int)fi.origin.len, (char *)fi.origin.s, (int)fi.hira_reading.len,
           (char *)fi.hira_reading.s, (int)fi.pitch_number.len, (char *)fi.pitch_number.s,
           (int)fi.pitch_pattern.len, (char *)fi.pitch_pattern.s);
}

static void freefileinfo(fileinfo fi[static 1]) {
    /* frees8(&fi->origin); */
    frees8(&fi->hira_reading);
    frees8(&fi->pitch_number);
    frees8(&fi->pitch_pattern);
}

/* static void */
/* prints8(s8 z) */
/* { */
/*     for (int i = 0; i < z.len; i++) */
/* 	putchar((int)z.s[i]); */
/*     putchar('\n'); */
/* } */

static void add_filename(database db, s8 headw, s8 fullpth) {
    addtodb1(db, headw, fullpth);
}

static void add_fileinfo(database db, s8 fullpth, fileinfo fi) {
    s8 sep = S("\0");
    s8 data = concat(fi.origin, sep, fi.hira_reading, sep, fi.pitch_number, sep, fi.pitch_pattern);
    addtodb2(db, fullpth, data);
    frees8(&data);
}

// wrapper for json api
static s8 json_get_string_(json_stream *json) {
    size_t slen = 0;
    s8 r = {0};
    r.s = (u8 *)json_get_string(json, &slen);
    r.len = slen > 0 ? (size)(slen - 1) : 0; // API includes terminating 0 in
                                             // length
    return r;
}

static b32 endswith_o(s8 hc) {
    return s8equals(hc, S("こ")) || s8equals(hc, S("そ")) || s8equals(hc, S("と")) ||
           s8equals(hc, S("の")) || s8equals(hc, S("ほ")) || s8equals(hc, S("も")) ||
           s8equals(hc, S("よ")) || s8equals(hc, S("ょ")) || s8equals(hc, S("ろ")) ||
           s8equals(hc, S("を")) || s8equals(hc, S("ご")) || s8equals(hc, S("ぞ")) ||
           s8equals(hc, S("ど")) || s8equals(hc, S("ぼ")) || s8equals(hc, S("ぽ"));
}

static b32 endswith_u(s8 hc) {
    return s8equals(hc, S("く")) || s8equals(hc, S("す")) || s8equals(hc, S("つ")) ||
           s8equals(hc, S("ぬ")) || s8equals(hc, S("ふ")) || s8equals(hc, S("む")) ||
           s8equals(hc, S("ゆ")) || s8equals(hc, S("ゅ")) || s8equals(hc, S("る")) ||
           s8equals(hc, S("ぐ")) || s8equals(hc, S("ず")) || s8equals(hc, S("づ")) ||
           s8equals(hc, S("ぶ")) || s8equals(hc, S("ぷ"));
}

static b32 endswith_e(s8 hc) {
    return s8equals(hc, S("け")) || s8equals(hc, S("せ")) || s8equals(hc, S("て")) ||
           s8equals(hc, S("ね")) || s8equals(hc, S("へ")) || s8equals(hc, S("め")) ||
           s8equals(hc, S("れ")) || s8equals(hc, S("げ")) || s8equals(hc, S("ぜ")) ||
           s8equals(hc, S("で")) || s8equals(hc, S("べ")) || s8equals(hc, S("ぺ"));
}

/* static b32 */
/* endswith_a(s8 hc) */
/* { */
/*     return s8equals(hc, S("あ")) */
/* 	   || s8equals(hc, S("か")) */
/* 	   || s8equals(hc, S("さ")) */
/* 	   || s8equals(hc, S("た")) */
/* 	   || s8equals(hc, S("な")) */
/* 	   || s8equals(hc, S("は")) */
/* 	   || s8equals(hc, S("ま")) */
/* 	   || s8equals(hc, S("や")) */
/* 	   || s8equals(hc, S("ら")) */
/* 	   || s8equals(hc, S("わ")) */
/* 	   || s8equals(hc, S("が")) */
/* 	   || s8equals(hc, S("ざ")) */
/* 	   || s8equals(hc, S("だ")) */
/* 	   || s8equals(hc, S("ば")) */
/* 	   || s8equals(hc, S("ぱ")); */
/* } */

static b32 endswith_i(s8 hc) {
    return s8equals(hc, S("い"));
}

static bool rem_chouon(s8 word) {
    u8 *chouon = (u8 *)strstr((char *)word.s, "ー");
    if (chouon) {
        // FIXME: This is just completely brute forced using available functions
        size pos = chouon - word.s;
        s8 prefix = (s8){.s = word.s, .len = pos};
        if (prefix.len == 0)
            return false;
        size prevclen = prefix.len - s8striputf8chr(prefix).len;
        assert(prevclen > 0);
        s8 prevc = taketail(prefix, prevclen);
        s8 chouonpos = fromcstr_((char *)chouon);

        if (endswith_o(prevc) || endswith_u(prevc)) {
            s8copy(chouonpos, S("う"));
            return true;
        } else if (endswith_e(prevc) || endswith_i(prevc)) {
            s8copy(chouonpos, S("い"));
            return true;
        }
    }

    return false;
}

static s8 normalize_reading(s8 reading) {
    s8 r = s8dup(reading);
    kata2hira(r);         // inplace
    while (rem_chouon(r)) // inplace
        ;
    return r;
}

static void add_from_index(database db, const char *index_path, s8 curdir) {
    _drop_(fclose) FILE *index = fopen(index_path, "r");
    die_on(!index, "Failed to open the index '%s': %s", index_path, strerror(errno));

    json_stream s[1];
    json_open_stream(s, index);

    s8 cursrc = {0};
    s8 mediadir = {0};

    bool reading_meta = false;
    bool reading_headwords = false;
    bool reading_files = false;

    for (;;) {
        enum json_type type = json_next(s);
        s8 value = {0};
        switch (type) {
            case JSON_NULL:
                value = S("null");
                break;
            case JSON_TRUE:
                value = S("true");
                break;
            case JSON_FALSE:
                value = S("false");
                break;
            case JSON_NUMBER:
                value = json_get_string_(s);
                break;
            case JSON_STRING:
                value = json_get_string_(s);
            default:
                break;
        }

        if (type == JSON_ERROR) {
            err("%s", json_get_error(s));
            break;
        } else if (type == JSON_DONE)
            break;

        if (!reading_meta && json_get_depth(s) == 1 && type == JSON_STRING &&
            s8equals(value, S("meta"))) {
            reading_meta = true;
            type = json_next(s);
            assert(type == JSON_OBJECT);
        } else if (reading_meta && json_get_depth(s) == 1 && type == JSON_OBJECT_END) {
            reading_meta = false;
        } else if (reading_meta) {
            if (type == JSON_STRING) {
                value = json_get_string_(s);
                if (s8equals(value, S("name"))) {
                    type = json_next(s);
                    assert(type == JSON_STRING);
                    cursrc = s8dup(json_get_string_(s));
                } else if (s8equals(value, S("media_dir"))) {
                    type = json_next(s);
                    assert(type == JSON_STRING);
                    mediadir = s8dup(json_get_string_(s));
                } else
                    json_skip(s);
            } else
                json_skip(s);
        } else if (!reading_headwords && json_get_depth(s) == 1 && type == JSON_STRING &&
                   s8equals(value, S("headwords"))) {
            reading_headwords = true;
            type = json_next(s);
            assert(type == JSON_OBJECT);
        } else if (reading_headwords && json_get_depth(s) == 1 && type == JSON_OBJECT_END) {
            reading_headwords = false;
        } else if (reading_headwords && type == JSON_STRING) {
            s8 headword = s8dup(value);

            type = json_next(s);
            if (type == JSON_STRING) {
                s8 fn = json_get_string_(s);
                s8 fullpth = buildpath(curdir, mediadir, fn);
                add_filename(db, headword, fullpth);
                frees8(&fullpth);
            } else if (type == JSON_ARRAY) {
                type = json_next(s);
                while (type != JSON_ARRAY_END && type != JSON_DONE && type != JSON_ERROR) {
                    if (type == JSON_STRING) {
                        s8 fn = json_get_string_(s);
                        s8 fullpth = buildpath(curdir, mediadir, fn);
                        add_filename(db, headword, fullpth);
                        frees8(&fullpth);
                    } else
                        err("Encountered an unexpected type '%s', \
				in file name array.",
                            json_typename[type]);

                    type = json_next(s);
                }
            } else
                err("Encountered unexpected type '%s' \
				for filename of headword '%.*s'.",
                    json_typename[type], (int)headword.len, (char *)headword.s);

            frees8(&headword);
        } else if (reading_headwords) // Warning: Order is important
        {
            dbg("Skipping entry of type '%s' while reading headwords.", json_typename[type]);
            json_skip(s);
        } else if (!reading_files && json_get_depth(s) == 1 && type == JSON_STRING &&
                   s8equals(value, S("files"))) {
            reading_files = true;
            type = json_next(s);
            assert(type == JSON_OBJECT);
        } else if (reading_files && json_get_depth(s) == 1 && type == JSON_OBJECT_END) {
            /* reading_files = false; */
            break;
        } else if (reading_files && type == JSON_STRING) {
            // TODO: Add debug check for audio filename ending (.ogg, .mp3, ...)
            s8 fn = value;
            s8 fullpth = buildpath(curdir, mediadir, fn);

            type = json_next(s);
            assert(type == JSON_OBJECT);
            type = json_next(s);
            fileinfo fi = {.origin = cursrc};
            while (type != JSON_OBJECT_END && type != JSON_DONE) {
                if (type == JSON_STRING) {
                    value = json_get_string_(s);
                    if (s8equals(value, S("kana_reading"))) {
                        type = json_next(s);
                        assert(type == JSON_STRING);
                        fi.hira_reading = normalize_reading(json_get_string_(s));
                    } else if (s8equals(value, S("pitch_number"))) {
                        type = json_next(s);
                        assert(type == JSON_STRING);
                        fi.pitch_number = s8dup(json_get_string_(s));
                    } else if (s8equals(value, S("pitch_pattern"))) {
                        type = json_next(s);
                        assert(type == JSON_STRING);
                        fi.pitch_pattern = s8dup(json_get_string_(s));
                    } else
                        json_skip(s);
                } else if (type == JSON_ERROR) {
                    err("%s", json_get_error(s));
                    break;
                } else
                    json_skip(s);

                type = json_next(s);
            }
            if (type != JSON_OBJECT_END)
                json_skip_until(s, JSON_OBJECT_END);

            add_fileinfo(db, fullpth, fi);
            freefileinfo(&fi);
            frees8(&fullpth);
        } else if (reading_files) {
            dbg("Skipping entry of type '%s' while reading files.", json_typename[type]);
            json_skip(s);
        }
    }

    frees8(&cursrc);
    fclose(index);
    json_close(s);
}

/*
 * Returns: 0 on success, -1 on failure and sets errno
 */
static int create_dir(char *dir_path) {
    // TODO: Recursive implementation?
    int status = mkdir(dir_path, S_IRWXU | S_IRWXG | S_IXOTH);
    return (status == 0 || errno == EEXIST) ? 0 : -1;
}

static void jppron_create(char *audio_dir_path, s8 dbpth) {
    s8 dbfile = buildpath(dbpth, S("data.mdb"));
    remove((char *)dbfile.s);
    frees8(&dbfile);

    int stat = create_dir((char *)dbpth.s);
    die_on(stat != 0, "Creating directory '%s': %s", dbpth.s, strerror(errno));

    _drop_(closedir) DIR *audio_dir = opendir(audio_dir_path);
    die_on(!audio_dir, "Failed to open audio directory '%s': %s", audio_dir_path, strerror(errno));

    database db = opendb((char *)dbpth.s, false);

    struct dirent *entry;
    while ((entry = readdir(audio_dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        s8 curdir = buildpath(fromcstr_(audio_dir_path), fromcstr_(entry->d_name));
        s8 index_path = buildpath(curdir, S("index.json"));
        dbg("Processing path: %.*s", (int)curdir.len, (char *)curdir.s);

        if (access((char *)index_path.s, F_OK) == 0)
            add_from_index(db, (char *)index_path.s, curdir);
        else
            dbg("No index file found");

        frees8(&curdir);
        frees8(&index_path);
    }

    closedb(db);

    s8 lock_file = buildpath(dbpth, S("lock.mdb"));
    remove((char *)lock_file.s);
    frees8(&lock_file);
}

static fileinfo getfileinfo(database db, s8 fn) {
    s8 data = getfromdb2(db, fn);
    s8 data_split[4] = {0};

    s8 d = data;
    for (long unsigned int i = 0; i < arrlen(data_split) && d.len > 0; i++) {
        assert(d.len > 0);

        size len = 0;
        while (len < d.len && d.s[len] != '\0')
            len++;

        data_split[i] = news8(len);
        u8copy(data_split[i].s, d.s, data_split[i].len);

        d.s += data_split[i].len + 1;
        d.len -= data_split[i].len + 1;
    }

    frees8(&data);

    return (fileinfo){.origin = data_split[0],
                      .hira_reading = data_split[1],
                      .pitch_number = data_split[2],
                      .pitch_pattern = data_split[3]};
}

static void play_word(s8 word, s8 reading, s8 database_path) {
    s8 normread = normalize_reading(reading);

    database db = opendb((char *)database_path.s, true);
    s8 *files = getfiles(db, word);

    if (!files) {
        msg("Nothing found.");
        closedb(db);
        return;
    }

    dbg("Playing word: %.*s with reading: %.*s", (int)word.len, (char *)word.s, (int)normread.len,
        (char *)normread.s);
    if (reading.len) {
        bool match = false;
        for (size_t i = 0; i < buf_size(files); i++) {
            fileinfo fi = getfileinfo(db, files[i]);

            if (s8equals(normread, fi.hira_reading)) {
                printf("File path: %.*s\n", (int)files[i].len, (char *)files[i].s);
                print_fileinfo(fi);
                play_audio(files[i]);
                match = true;
            }
            freefileinfo(&fi);
        }
        if (!match) {
            dbg("Could not find an audio file with corresponding reading. "
                "Playing all..");
            reading.len = 0;
        }
    }
    if (!reading.len) {
        // Play all
        for (size_t i = 0; i < buf_size(files); i++) {
            fileinfo fi = getfileinfo(db, files[i]);
            printf("\nFile path: %.*s\n", (int)files[i].len, (char *)files[i].s);
            print_fileinfo(fi);
            freefileinfo(&fi);

            play_audio(files[i]);
        }
    }

    frees8buffer(&files);
    closedb(db);
}

static s8 get_database_path(void) {
    return fromcstr_(g_build_filename(g_get_user_data_dir(), "jppron", NULL));
}

/**
 * jppron:
 * @word: word to be pronounced
 * @audiopth: (optional): Path to the ajt-style audio file directories
 *
 */
void jppron(s8 word, s8 reading, char *audiopth) {
    s8 dbpth = get_database_path();

    s8 dbfile = buildpath(dbpth, S("data.mdb"));
    i32 no_access = access((char *)dbfile.s, R_OK);
    frees8(&dbfile);

    if (no_access) {
        if (audiopth) {
            msg("Indexing files..");
            jppron_create(audiopth, dbpth); // TODO: エラー処理
            msg("Index completed.");
        } else {
            dbg("No (readable) database file and no audio path provided. Exiting..");
            return;
        }
    }

    play_word(word, reading, dbpth);

    frees8(&dbpth);
}

#ifdef INCLUDE_MAIN
int main(int argc, char **argv) {
    if (argc < 2)
        die("Usage: %s word", argc > 0 ? argv[0] : "jppron");

    char *default_audio_path = g_build_filename(g_get_user_data_dir(), "ajt_japanese_audio", NULL);

    if (strcmp(argv[1], "-c") == 0)
        jppron_create(default_audio_path, get_database_path());
    else
        jppron(fromcstr_(argv[1]), fromcstr_(argc > 2 ? argv[2] : 0), default_audio_path);
}
#endif
