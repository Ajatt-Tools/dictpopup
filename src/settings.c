#include <errno.h>
#include <libgen.h> // dirname()
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h> // getopt

#include <gio/gio.h>
#include <glib.h>

#include "messages.h"
#include "platformdep.h"
#include "settings.h"
#include "util.h"

settings cfg = {0};
bool print_cfg = false;

static settings get_default_cfg(void) {
    settings default_cfg = {
        .general.sort = 0,
        .general.dictSortOrder = NULL,
        .general.dbpth = NULL, // Set by set_runtime_defaults()
        .general.nukeWhitespaceLookup = 1,
        .general.mecab = 0,
        .general.substringSearch = 1,
        //
        .anki.enabled = 0,
        .anki.deck = NULL, // Don't even try to guess
        .anki.notetype = "Japanese sentences",
        .anki.fieldnames =
            (char *[]){"SentKanji", "VocabKanji", "VocabFurigana", "VocabDef", "Notes"},
        .anki.numFields = 0,
        .anki.copySentence = 1,
        .anki.nukeWhitespaceSentence = 1,
        .anki.fieldMapping = (u32[]){3, 4, 7, 6, 8},
        .anki.checkExisting = 0,
        //
        .popup.width = 600,
        .popup.height = 400,
        .popup.margin = 5,
        //
        .pron.displayButton = 0,
        .pron.onStart = 0,
        .pron.dirPath = NULL, // Don't guess
    };
    return default_cfg;
}

static void set_runtime_defaults(void) {
    if (!cfg.general.dbpth) {
        cfg.general.dbpth =
            (char *)buildpath(fromcstr_((char *)g_get_user_data_dir()), S("dictpopup")).s;
    }
}

int parse_cmd_line_opts(int argc, char **argv) {
    int c;
    opterr = 0;

    while ((c = getopt(argc, argv, "chd:")) != -1)
        switch (c) {
            case 'c':
                print_cfg = 1;
                break;
            case 'd':
                if (optarg && *optarg) {
                    dbg("Setting dbpath");
                    free(cfg.general.dbpth);
                    cfg.general.dbpth = strdup(optarg);
                }
                break;
            case 'h':
                puts("See 'man dictpopup' for help.");
                exit(EXIT_SUCCESS);
            case '?':
                fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
                exit(EXIT_FAILURE);
        }

    return optind;
}

#define printyn(boolean) (boolean ? "yes" : "no")

// Expected to be null-terminated
static void print_array(char **arr) {
    putchar('[');
    if (arr) {
        for (char **p = arr; *p; p++) {
            if (p != arr)
                putchar(',');
            printf("\"%s\"", *p);
        }
    }
    putchar(']');
    putchar('\n');
}

void print_settings(void) {
    // TODO: Finish implementing
    puts("Settings:");
    puts("[General]");
    printf("Database path: '%s'\n", cfg.general.dbpth);
    printf("Sort: %s\n", printyn(cfg.general.sort));
    printf("Dictionary sort order: ");
    print_array(cfg.general.dictSortOrder);
    printf("Remove whitespace from lookup: %s\n", printyn(cfg.general.nukeWhitespaceLookup));
    printf("Fall back to hiragana conversion using MeCab: %s\n", printyn(cfg.general.mecab));
    printf("Decrese length until match: %s\n", printyn(cfg.general.substringSearch));
    putchar('\n');

    puts("[Anki]");
    printf("Anki support enabled: %s\n", printyn(cfg.anki.enabled));
    printf("Anki deck: \"%s\"\n", cfg.anki.deck);
    printf("Anki notetype: \"%s\"\n", cfg.anki.notetype);
    printf("Prompt to copy sentence: %s\n", printyn(cfg.anki.copySentence));
    printf("Remove whitespace from sentence: %s\n", printyn(cfg.anki.nukeWhitespaceSentence));
    printf("Fields: ");
    print_array(cfg.anki.fieldnames);
    printf("Mappings : [");
    for (size_t i = 0; i < cfg.anki.numFields; i++) {
        if (i)
            putchar(',');
        printf("%u", cfg.anki.fieldMapping[i]);
    }
    printf("]\n");
    putchar('\n');

    puts("[Popup]");
    printf("Width: %u\n", cfg.popup.width);
    printf("Height: %u\n", cfg.popup.height);
    printf("Margin: %u\n", cfg.popup.margin);
    putchar('\n');

    puts("[Pronunciation]");
    printf("Display Button: %s\n", printyn(cfg.pron.displayButton));
    printf("Pronounce on start: %s\n", printyn(cfg.pron.onStart));
    printf("Audio file directory: %s\n", cfg.pron.dirPath ? cfg.pron.dirPath : "empty");
    putchar('\n');
}

/*
 * Read key @key from group @group into memory pointed to by @dest if exists and
 * non-negative. On error it will print a debug message and not write anything
 * to @dest
 */
static void read_uint(GKeyFile *kf, const char *group, const char *key, u32 dest[static 1]) {
    GError *error = NULL;
    int val = g_key_file_get_integer(kf, group, key, &error);
    if (error) {
        dbg("%s", error->message);
        g_error_free(error);
    } else if (val < 0)
        err("Expected a positive value in key: '%s' and group '%s', but received negative.", key,
            group);
    else
        *dest = (u32)val;
}

/*
 *
 */
static void read_bool(GKeyFile *kf, const char *group, const char *key, bool *dest) {
    GError *error = NULL;
    gboolean value = g_key_file_get_boolean(kf, group, key, &error);
    if (error) {
        dbg("%s", error->message);
        g_error_free(error);
    } else
        *dest = value;
}

static void read_string(GKeyFile *kf, const char *group, const char *key, char *dest[static 1]) {
    GError *error = NULL;
    char *value = g_key_file_get_string(kf, group, key, &error);
    if (error) {
        dbg("%s", error->message);
        g_error_free(error);
    } else
        *dest = value;
}

static void read_string_list(GKeyFile *kf, const char *group, const char *key,
                             char **dest[static 1], gsize *length) {
    GError *error = NULL;
    char **value = g_key_file_get_string_list(kf, group, key, length ? length : NULL, &error);
    if (error) {
        dbg("%s", error->message);
        g_error_free(error);
    } else
        *dest = value;
}

static void read_uint_list(GKeyFile *kf, const char *group, const char *key, u32 *dest[static 1],
                           gsize length[static 1]) {
    GError *error = NULL;
    int *val = g_key_file_get_integer_list(kf, group, key, length, &error);
    if (error) {
        dbg("%s", error->message);
        g_error_free(error);
        return;
    }

    for (gsize i = 0; i < *length; i++) {
        if (val[i] < 0) {
            err("Received a negative value at index '%li' in key: '%s' and group '%s' when expecting a positive.",
                i, key, group);
            return;
        }
    }

    *dest = (u32 *)val;
}

static void read_general(GKeyFile *kf) {
    read_string(kf, "General", "DatabasePath", &cfg.general.dbpth);
    read_bool(kf, "General", "Sort", &cfg.general.sort);
    if (cfg.general.sort)
        read_string_list(kf, "General", "DictSortOrder", &cfg.general.dictSortOrder, NULL);
    read_bool(kf, "General", "NukeWhitespaceLookup", &cfg.general.nukeWhitespaceLookup);
    read_bool(kf, "General", "MecabConversion", &cfg.general.mecab);
    read_bool(kf, "General", "SubstringSearch", &cfg.general.substringSearch);
}

static void read_anki(GKeyFile *kf) {
    read_bool(kf, "Anki", "Enabled", &cfg.anki.enabled);
    if (cfg.anki.enabled) {
        read_string(kf, "Anki", "Deck", &cfg.anki.deck);
        read_string(kf, "Anki", "NoteType", &cfg.anki.notetype);

        size_t n_fields = 0;
        read_string_list(kf, "Anki", "FieldNames", &cfg.anki.fieldnames, &n_fields);

        read_bool(kf, "Anki", "CopySentence", &cfg.anki.copySentence);
        read_bool(kf, "Anki", "NukeWhitespaceSentence", &cfg.anki.nukeWhitespaceSentence);

        size_t n_fieldmap = 0;
        read_uint_list(kf, "Anki", "FieldMapping", &cfg.anki.fieldMapping, &n_fieldmap);

        if (n_fieldmap != n_fields) {
            err("Number of fieldnames does not match number of fieldmappings "
                "in config. Disabling Anki support..");
            cfg.anki.enabled = 0;
        } else
            cfg.anki.numFields = n_fields;

        read_bool(kf, "Anki", "CheckIfExists", &cfg.anki.checkExisting);
        read_string(kf, "Anki", "SearchField", &cfg.anki.searchField);
    }
}

static void read_popup(GKeyFile *kf) {
    read_uint(kf, "Popup", "Width", &cfg.popup.width);
    read_uint(kf, "Popup", "Height", &cfg.popup.height);
    read_uint(kf, "Popup", "Margin", &cfg.popup.margin);
}

static void read_pronunciation(GKeyFile *kf) {
    read_bool(kf, "Pronunciation", "DisplayButton", &cfg.pron.displayButton);
    read_bool(kf, "Pronunciation", "PronounceOnStart", &cfg.pron.onStart);
    read_string(kf, "Pronunciation", "FolderPath", &cfg.pron.dirPath);
}

static void copy_ready_callback(GObject *source_object, GAsyncResult *res, gpointer data) {
    GError *error = NULL;
    g_file_copy_finish(G_FILE(source_object), res, &error);

    if (error)
        dbg("Error copying default config file: %s", error->message);
    else
        dbg("Successfully copied default config file to ~/.config/dictpopup/config.ini.");
}

static void copy_default_config(char *cfgfile) {
    // TODO: Make default loc OS independent
    const char *default_config_location = "/usr/local/share/dictpopup/config.ini";

    char *cfgdir = dirname(cfgfile);
    createdir(cfgdir);

    GFile *source = g_file_new_for_path(default_config_location);
    GFile *dest = g_file_new_for_path(cfgfile);
    g_file_copy_async(source, dest, G_FILE_COPY_NONE, G_PRIORITY_LOW, NULL, NULL, NULL,
                      copy_ready_callback, NULL);
    g_object_unref(source);
    g_object_unref(dest);
}

/*
 * The caller takes ownership of the data.
 */
static s8 get_config_filepath(void) {
    char *env = getenv("DICTPOPUP_CONFIG_DIR");
    if (env && *env)
        return buildpath(fromcstr_(env), S("config.ini"));
    else
        return buildpath(fromcstr_((char *)g_get_user_config_dir()), S("dictpopup"),
                         S("config.ini"));
}

void read_user_settings(int fieldmapping_max) {
    cfg = get_default_cfg(); // TODO: Put this to the end and only set missing values

    g_autoptr(GError) error = NULL;
    g_autoptr(GKeyFile) kf = g_key_file_new();
    _drop_(frees8) s8 cfgfile = get_config_filepath();
    dbg("Using config file: '%.*s'", (int)cfgfile.len, (char *)cfgfile.s);
    if (!g_key_file_load_from_file(kf, (char *)cfgfile.s, G_KEY_FILE_NONE, &error)) {
        if (g_error_matches(error, G_FILE_ERROR, G_FILE_ERROR_NOENT)) {
            err("Could not find a config file in: \"%s\". Copying default config.. ", cfgfile.s);
            copy_default_config((char *)cfgfile.s); // Uses the default config from above though
        } else
            err("Error opening \"%s\": %s. Falling back to default config.", cfgfile.s,
                error->message);
    } else {
        read_general(kf);
        read_anki(kf);
        read_popup(kf);
        read_pronunciation(kf);
    }

    set_runtime_defaults();

    if (print_cfg)
        print_settings();
}
