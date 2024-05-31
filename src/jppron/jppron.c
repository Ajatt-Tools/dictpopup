#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>   // access
#include <errno.h>

#include "messages.h"
#include "platformdep.h"
#include "util.h"
#include "database.h"
#include "jppron.h"
#include "ajt_audio_index_parser.h"

DEFINE_DROP_FUNC(DIR *, closedir)

#define err_ret_on(cond, fmt, ...)                                                                 \
    do {                                                                                           \
        if (unlikely(cond)) {                                                                      \
            err(fmt, ##__VA_ARGS__);                                                               \
            return;                                                                                \
        }                                                                                          \
    } while (0)

static s8 get_default_database_path(void) {
    return buildpath(fromcstr_((char*)get_user_data_dir()), S("jppron"));
}

static void freefileinfo(fileinfo fi[static 1]) {
    frees8(&fi->origin);
    frees8(&fi->hira_reading);
    frees8(&fi->pitch_number);
    frees8(&fi->pitch_pattern);
}

DEFINE_DROP_FUNC_PTR(fileinfo, freefileinfo)

static void print_fileinfo(fileinfo fi) {
    printf("Source: %.*s\nReading: %.*s\nPitch number: %.*s\nPitch pattern: "
           "%.*s\n\n",
           (int)fi.origin.len, (char *)fi.origin.s, (int)fi.hira_reading.len,
           (char *)fi.hira_reading.s, (int)fi.pitch_number.len, (char *)fi.pitch_number.s,
           (int)fi.pitch_pattern.len, (char *)fi.pitch_pattern.s);
}

static void jppron_create(char *audio_dir_path, s8 dbpth) {
    s8 dbfile = buildpath(dbpth, S("data.mdb"));
    remove((char *)dbfile.s);
    frees8(&dbfile);

    createdir((char *)dbpth.s);

    _drop_(closedir) DIR *audio_dir = opendir(audio_dir_path);
    err_ret_on(!audio_dir, "Failed to open audio directory '%s': %s", audio_dir_path,
               strerror(errno));

    _drop_(jdb_close) database *db = jdb_open((char *)dbpth.s, false);
    // TODO: Graceful error handling on db problems

    struct dirent *entry;
    while ((entry = readdir(audio_dir)) != NULL) {
        s8 fn = fromcstr_(entry->d_name);

        if (s8equals(fn, S(".")) || s8equals(fn, S("..")))
            continue;

        _drop_(frees8) s8 curdir = buildpath(fromcstr_(audio_dir_path), fromcstr_(entry->d_name));
        _drop_(frees8) s8 index_path = buildpath(curdir, S("index.json"));
        dbg("Processing path: %.*s", (int)curdir.len, (char *)curdir.s);

        if (access((char *)index_path.s, F_OK) == 0)
            parse_audio_index_from_file(curdir, (char *)index_path.s,
                                        (void (*)(void *, s8, s8))jdb_add_headword_with_file, db,
                                        (void (*)(void *, s8, fileinfo))jdb_add_file_with_fileinfo, db);
        else
            dbg("Index file of folder '%s' not existing or not accessible", entry->d_name);
    }
}

static s8 normalize_reading(s8 reading) {
    s8 r = s8dup(reading);
    // TODO: Implement
    return r;
}

static void play_word(s8 word, s8 reading, s8 database_path) {
    _drop_(jdb_close) database *db = jdb_open((char *)database_path.s, true);
    _drop_(frees8) s8 normread = normalize_reading(reading);
    _drop_(frees8buffer) s8 *files = jdb_get_files(db, word);

    if (!files) {
        msg("No pronunciation found.");
        return;
    }

    dbg("Playing word: %.*s with reading: %.*s", (int)word.len, (char *)word.s, (int)normread.len,
        (char *)normread.s);
    if (reading.len) {
        bool match = false;
        for (size_t i = 0; i < buf_size(files); i++) {
            _drop_(freefileinfo) fileinfo fi = jdb_get_fileinfo(db, files[i]);

            if (s8equals(normread, fi.hira_reading)) {
                printf("File path: %.*s\n", (int)files[i].len, (char *)files[i].s);
                print_fileinfo(fi);
                play_audio(files[i]);
                match = true;
            }
        }
        if (!match) {
            dbg("Could not find an audio file with corresponding reading. "
                "Playing all..");
            reading.len = 0;
            // FALLTHROUGH
        }
    }
    if (!reading.len) {
        // Play all
        for (size_t i = 0; i < buf_size(files); i++) {
            _drop_(freefileinfo) fileinfo fi = jdb_get_fileinfo(db, files[i]);
            printf("\nFile path: %.*s\n", (int)files[i].len, (char *)files[i].s);
            print_fileinfo(fi);

            play_audio(files[i]);
        }
    }
}

/**
 * jppron:
 * @word: word to be pronounced
 * @audiopth: (optional): Path to the ajt-style audio file directories
 *
 */
void jppron(s8 word, s8 reading, char *audio_folders_path) {
    _drop_(frees8) s8 dbpath = get_default_database_path();

    if (!jdb_check_exists(dbpath)) {
        if (audio_folders_path) {
            msg("Indexing files..");
            jppron_create(audio_folders_path, dbpath); // TODO: エラー処理
            msg("Index completed.");
        } else {
            dbg("No (readable) database file and no audio path provided. Exiting..");
            return;
        }
    }

    play_word(word, reading, dbpath);
}

#ifdef JPPRON_MAIN
int main(int argc, char **argv) {
    if (argc < 2)
        die("Usage: %s word", argc > 0 ? argv[0] : "jppron");

    char *default_audio_path = buildpath(fromcstr_((char *)get_user_data_dir()), S("ajt_japanese_audio"));

    if (strcmp(argv[1], "-c") == 0)
        jppron_create(default_audio_path, get_database_path());
    else
        jppron(fromcstr_(argv[1]), fromcstr_(argc > 2 ? argv[2] : 0), default_audio_path);
}
#endif
