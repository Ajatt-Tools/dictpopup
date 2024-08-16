#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // access

#include "jppron/ajt_audio_index_parser.h"
#include "jppron/database.h"
#include "jppron/jppron.h"

#include "deinflector.h"
#include "platformdep/audio.h"
#include "platformdep/file_operations.h"
#include "utils/messages.h"
#include "utils/util.h"

DEFINE_DROP_FUNC(DIR *, closedir)

#define err_ret_on(cond, fmt, ...)                                                                 \
    do {                                                                                           \
        if (unlikely(cond)) {                                                                      \
            err(fmt, ##__VA_ARGS__);                                                               \
            return;                                                                                \
        }                                                                                          \
    } while (0)

static void freefileinfo(fileinfo_s fi[static 1]) {
    frees8(&fi->origin);
    frees8(&fi->hira_reading);
    frees8(&fi->pitch_number);
    frees8(&fi->pitch_pattern);
}
DEFINE_DROP_FUNC_PTR(fileinfo_s, freefileinfo)

static s8 get_default_database_path(void) {
    return buildpath(fromcstr_((char *)get_user_data_dir()), S("jppron"));
}

// static void print_fileinfo(fileinfo_s fi) {
//     printf("Source: %.*s\nReading: %.*s\nPitch number: %.*s\nPitch pattern: "
//            "%.*s\n\n",
//            (int)fi.origin.len, (char *)fi.origin.s, (int)fi.hira_reading.len,
//            (char *)fi.hira_reading.s, (int)fi.pitch_number.len, (char *)fi.pitch_number.s,
//            (int)fi.pitch_pattern.len, (char *)fi.pitch_pattern.s);
// }

static void jppron_create(char *audio_dir_path, s8 dbpth) {
    jdb_remove(dbpth);
    createdir(dbpth);

    _drop_(closedir) DIR *audio_dir = opendir(audio_dir_path);
    err_ret_on(!audio_dir, "Failed to open audio directory '%s': %s", audio_dir_path,
               strerror(errno));
    _drop_(jdb_close) database *db = jdb_open((char *)dbpth.s, false);

    struct dirent *entry;
    while ((entry = readdir(audio_dir)) != NULL) {
        s8 fn = fromcstr_(entry->d_name);

        if (s8equals(fn, S(".")) || s8equals(fn, S("..")))
            continue;

        _drop_(frees8) s8 curdir = buildpath(fromcstr_(audio_dir_path), fromcstr_(entry->d_name));
        _drop_(frees8) s8 index_path = buildpath(curdir, S("index.json"));
        dbg("Processing path: %.*s", (int)curdir.len, (char *)curdir.s);

        if (access((char *)index_path.s, F_OK) == 0)
            parse_audio_index_from_file(
                curdir, (char *)index_path.s, (void (*)(void *, s8, s8))jdb_add_headword_with_file,
                db, (void (*)(void *, s8, fileinfo_s))jdb_add_file_with_fileinfo, db);
        else
            dbg("Index file of folder '%s' not existing or not accessible", entry->d_name);
    }
}

static s8 normalize_reading(s8 reading) {
    s8 r = s8dup(reading);
    kata2hira(r);
    // TODO: Replace 長音
    return r;
}

void free_pronfile(pronfile_s pronfile[static 1]) {
    frees8(&pronfile->filepath);
    freefileinfo(&pronfile->fileinfo);
}

void free_pronfile_buffer(pronfile_s *pronfiles) {
    while (buf_size(pronfiles) > 0)
        free_pronfile(&buf_pop(pronfiles));
    buf_free(pronfiles);
}

static fileinfo_s *get_fileinfo_for_array(database *db, s8 *files) {
    fileinfo_s *fi = new (fileinfo_s, buf_size(files));
    for (size_t i = 0; i < buf_size(files); i++) {
        fi[i] = jdb_get_fileinfo(db, files[i]);
    }
    return fi;
}

static void get_all_files_and_fileinfo_for(s8 word, s8 db_path, s8 *files[static 1],
                                           fileinfo_s *fileinfo[static 1]) {
    _drop_(jdb_close) database *db = jdb_open((char *)db_path.s, true);

    *files = jdb_get_files(db, word);
    if (!*files)
        return;

    *fileinfo = get_fileinfo_for_array(db, *files);
}

pronfile_s *get_pronfiles_for(s8 word, s8 reading, s8 db_path) {
    _drop_(frees8buffer) s8 *files = 0;
    _drop_(free) fileinfo_s *fileinfo = 0;
    get_all_files_and_fileinfo_for(word, db_path, &files, &fileinfo);

    if (!files)
        return NULL;

    _drop_(frees8) s8 normread = normalize_reading(reading);

    pronfile_s *pronfiles = NULL;

    if (reading.len) {
        for (size_t i = 0; i < buf_size(files); i++) {
            if (s8equals(normread, fileinfo[i].hira_reading)) {
                pronfile_s pf = (pronfile_s){.filepath = files[i], .fileinfo = fileinfo[i]};
                // Transfer ownership
                files[i] = (s8){0};
                fileinfo[i] = (fileinfo_s){0};
                buf_push(pronfiles, pf);
            }
        }
    }

    if (!pronfiles) {
        // Add all
        for (size_t i = 0; i < buf_size(files); i++) {
            pronfile_s pf = (pronfile_s){.filepath = files[i], .fileinfo = fileinfo[i]};
            files[i] = (s8){0};
            fileinfo[i] = (fileinfo_s){0};
            buf_push(pronfiles, pf);
        }
    }

    for (size_t i = 0; i < buf_size(files); i++) {
        freefileinfo(&fileinfo[i]);
    }

    return pronfiles;
}

static void play_pronfiles(pronfile_s *pronfiles) {
    for (size_t i = 0; i < buf_size(pronfiles); i++) {
        play_audio(pronfiles[i].filepath);
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

    /* _drop_(free_pronfile_buffer) pronfile_s *pronfiles = get_pronfiles_for(word, reading,
     * dbpath); */
    pronfile_s *pronfiles = get_pronfiles_for(word, reading, dbpath);
    if (pronfiles) {
        play_pronfiles(pronfiles);
        free_pronfile_buffer(pronfiles);
    } else
        msg("No pronunciation found.");
}

// #ifdef JPPRON_MAIN
// int main(int argc, char **argv) {
//     if (argc < 2)
//         die("Usage: %s word", argc > 0 ? argv[0] : "jppron");
//
//     char *default_audio_path =
//         buildpath(fromcstr_((char *)get_user_data_dir()), S("ajt_japanese_audio"));
//
//     if (strcmp(argv[1], "-c") == 0)
//         jppron_create(default_audio_path, get_database_path());
//     else
//         jppron(fromcstr_(argv[1]), fromcstr_(argc > 2 ? argv[2] : 0), default_audio_path);
// }
// #endif
