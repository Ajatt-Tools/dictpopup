#include <errno.h>
#include <stdio.h>
#include <unistd.h> // getopt

#include <dirent.h> // opendir

#include "db.h"
#include "dictpopup_create.h"
#include "utils/messages.h"
#include "utils/util.h"
#include "yomichan_parser.h"

#include "platformdep/file_operations.h"

DEFINE_DROP_FUNC(FILE *, fclose)
DEFINE_DROP_FUNC(DIR *, closedir)

static ParserCallbacks get_callbacks(database_t *db) {
    return (ParserCallbacks){.foreach_dictentry = (void (*)(void *, Dictentry))db_put_dictent,
                             .userdata_de = db,
                             .foreach_freqentry = (void (*)(void *, freqentry))db_put_freq,
                             .userdata_fe = db,
                             .foreach_dictname = (void (*)(void *, s8))db_put_dictname,
                             .userdata_dn = db};
}

static void create_db_dir_if_necessary(void) {
    s8 dbpath = db_get_dbpath();
    createdir(dbpath);
}

void _nonnull_ dictpopup_create(const char *dictionaries_path,
                                bool (*should_overwrite_existing_db)(void *user_data), void *user_data,
                                atomic_bool *cancel_flag) {
    if (db_check_exists()) {
        if(!should_overwrite_existing_db(user_data))
            return;

        db_remove();
    }

    create_db_dir_if_necessary();

    _drop_(db_close) database_t *db = db_open(false);
    _drop_(closedir) DIR *dir = opendir(dictionaries_path);
    die_on(!dir, "Error opening directory '%s': %s", dictionaries_path, strerror(errno));

    ParserCallbacks callbacks = get_callbacks(db);

    struct dirent *entry;
    while ((entry = readdir(dir)) && !atomic_load(cancel_flag)) {
        _drop_(frees8) s8 fn =
            buildpath(fromcstr_((char *)dictionaries_path), fromcstr_(entry->d_name));
        if (endswith(fn, S(".zip")))
            parse_yomichan_dict((char *)fn.s, callbacks, cancel_flag);
    }
}