#include <errno.h>
#include <signal.h> // Catch SIGINT
#include <stdio.h>
#include <unistd.h> // getopt

#include <ctype.h>
#include <dirent.h> // opendir
#include <time.h>

#include "db.h"
#include "messages.h"
#include "utils/util.h"
#include "yomichan_parser.h"

#include "platformdep/file_operations.h"

typedef struct {
    s8 dbpath;
    s8 dictspath;
} config;

DEFINE_DROP_FUNC(FILE *, fclose)
DEFINE_DROP_FUNC(DIR *, closedir)

/*
 * Dialog which asks the user for confirmation (Yes/No)
 */
static bool askyn(const char *prompt) {
    printf("%s [y/n] ", prompt);
    char resp = tolower(getchar());
    return resp == 'y';
}

volatile sig_atomic_t sigint_received = 0;

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
    }

    if (!cfg->dictspath.len) {
        cfg->dictspath = S(".");
    }
}

int main(int argc, char *argv[]) {
    setup_sighandler();

    config cfg = {0};
    parse_cmd_line(argc, argv, &cfg);
    set_default_values(&cfg);

    msg("Using directory '%.*s' as dictionary path.", (int)cfg.dictspath.len,
        (char *)cfg.dictspath.s);
    msg("Storing database in: %.*s", (int)cfg.dbpath.len, (char *)cfg.dbpath.s);

    if (db_check_exists(cfg.dbpath)) {
        if (askyn("A database file already exists. "
                  "Would you like to delete the old one?"))
            db_remove(cfg.dbpath);
        else
            exit(EXIT_FAILURE);
    } else
        createdir((char *)cfg.dbpath.s);

    _drop_(db_close) database_t *db = db_open((char *)cfg.dbpath.s, false);
    _drop_(closedir) DIR *dir = opendir((char *)cfg.dictspath.s);
    die_on(!dir, "Error opening directory '%s': %s", (char *)cfg.dictspath.s, strerror(errno));

    double startTime = (double)clock() / CLOCKS_PER_SEC;
    struct dirent *entry;
    while ((entry = readdir(dir)) && !sigint_received) {
        _drop_(frees8) s8 fn = buildpath(cfg.dictspath, fromcstr_(entry->d_name));
        if (endswith(fn, S(".zip")))
            parse_yomichan_dict((char *)fn.s, (void (*)(void *, dictentry))db_put_dictent, db,
                                (void (*)(void *, freqentry))db_put_freq, db);
    }
    double endTime = (double)clock() / CLOCKS_PER_SEC;
    printf("Time taken: %.1fs\n", endTime - startTime);
}