#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // access

#include "jppron/ajt_audio_index_parser.h"
#include "jppron/jppron.h"
#include "jppron/jppron_database.h"

#include "deinflector/kata2hira.h"
#include "platformdep/file_operations.h"
#include "utils/messages.h"
#include "utils/util.h"

#include <objects/dict.h>

DEFINE_DROP_FUNC(DIR *, closedir)

#define err_ret_on(cond, fmt, ...)                                                                 \
    do {                                                                                           \
        if (unlikely(cond)) {                                                                      \
            err(fmt, ##__VA_ARGS__);                                                               \
            return;                                                                                \
        }                                                                                          \
    } while (0)

static s8 get_default_database_path(void) {
    return buildpath(fromcstr_((char *)get_user_data_dir()), S("jppron"));
}

static void process_audio_subdirectory(const char *audio_dir_path, char *subdir_name,
                                       PronDatabase *db) {
    dbg("Processing audio path: %s", audio_dir_path);
    _drop_(frees8) s8 curdir = buildpath(fromcstr_((char *)audio_dir_path), fromcstr_(subdir_name));
    _drop_(frees8) s8 index_path = buildpath(curdir, S("index.json"));
    dbg("Processing path: %.*s", (int)curdir.len, (char *)curdir.s);

    if (check_file_exists((char *)index_path.s)) {
        parse_audio_index_from_file(curdir, (char *)index_path.s,
                                    (void (*)(void *, s8, s8))jdb_add_headword_with_file, db,
                                    (void (*)(void *, s8, FileInfo))jdb_add_file_with_fileinfo, db);
    } else
        dbg("Index file of folder '%s' not existing or not accessible", subdir_name);
}

static s8 normalize_reading(s8 reading) {
    s8 r = s8dup(reading);
    kata2hira(r);
    // TODO: Replace 長音
    return r;
}

void free_pronfile(Pronfile pronfile[static 1]) {
    frees8(&pronfile->filepath);
    file_info_free(&pronfile->fileinfo);
}

void free_pronfile_buffer(Pronfile *pronfiles) {
    while (buf_size(pronfiles) > 0)
        free_pronfile(&buf_pop(pronfiles));
    buf_free(pronfiles);
}

static FileInfo *get_fileinfo_for_array(PronDatabase *db, s8 *files) {
    FileInfo *fi = new (FileInfo, buf_size(files));
    for (size_t i = 0; i < buf_size(files); i++) {
        fi[i] = jdb_get_fileinfo(db, files[i]);
    }
    return fi;
}

static void _nonnull_ get_all_files_and_fileinfo_for(s8 kanji, s8 *files[static 1],
                                                     FileInfo *fileinfo[static 1]) {
    _drop_(jppron_close_db) PronDatabase *db = jppron_open_db(true);
    if (!db)
        return;

    *files = jdb_get_files(db, kanji);
    if (!*files)
        return;

    *fileinfo = get_fileinfo_for_array(db, *files);
}

Pronfile *get_pronfiles_for(Word word) {
    _drop_(s8_buf_free) s8Buf files = 0;
    _drop_(free) FileInfo *fileinfo = 0;
    get_all_files_and_fileinfo_for(word.kanji, &files, &fileinfo);

    if (!files)
        return NULL;

    _drop_(frees8) s8 normread = normalize_reading(word.reading);

    Pronfile *pronfiles = NULL;

    if (word.reading.len) {
        for (size_t i = 0; i < buf_size(files); i++) {
            if (s8equals(normread, fileinfo[i].hira_reading)) {
                Pronfile pf = (Pronfile){.filepath = files[i], .fileinfo = fileinfo[i]};
                // Transfer ownership
                files[i] = (s8){0};
                fileinfo[i] = (FileInfo){0};
                buf_push(pronfiles, pf);
            }
        }
    }

    if (!pronfiles) {
        // Add all
        for (size_t i = 0; i < buf_size(files); i++) {
            Pronfile pf = (Pronfile){.filepath = files[i], .fileinfo = fileinfo[i]};
            files[i] = (s8){0};
            fileinfo[i] = (FileInfo){0};
            buf_push(pronfiles, pf);
        }
    }

    for (size_t i = 0; i < buf_size(files); i++) {
        file_info_free(&fileinfo[i]);
    }

    return pronfiles;
}

void jppron_create_index(const char *audio_folders_path, atomic_bool *cancel_flag) {
    if (!audio_folders_path) {
        dbg("No audio index path provided");
        return;
    }

    _drop_(frees8) s8 dbpth = get_default_database_path();

    dbg("Indexing pronunciation files..");
    jdb_remove(dbpth);
    createdir(dbpth);

    _drop_(jppron_close_db) PronDatabase *db = jppron_open_db(false);

    _drop_(closedir) DIR *audio_dir = opendir(audio_folders_path);
    err_ret_on(!audio_dir, "Failed to open audio directory '%s': %s", audio_folders_path,
               strerror(errno));

    struct dirent *entry;
    while ((entry = readdir(audio_dir)) != NULL && !atomic_load(cancel_flag)) {
        s8 fn = fromcstr_(entry->d_name);

        if (s8equals(fn, S(".")) || s8equals(fn, S("..")))
            continue;

        process_audio_subdirectory(audio_folders_path, entry->d_name, db);
    }
    dbg("Done with pronunciation index.");
}