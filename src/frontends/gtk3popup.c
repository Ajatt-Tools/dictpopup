#include <gtk/gtk.h>
#include <stdatomic.h>
#include <stdbool.h>

#include "ankiconnectc.h"
#include "dictpopup.h"
#include "jppron/jppron.h"
#include "settings.h"

#include "messages.h"
#include "platformdep/clipboard.h"

#include "utils/utf8.h"
#include "utils/util.h"

typedef struct {
    bool create_ac; // Whether or not the user wants to create an Anki card
    dictentry de;   // The entry that should be added to Anki if @create_ac is true
} WinArgs;

typedef struct {
    GtkWindow *win;
    GtkTextView *def_tw;
    GtkTextBuffer *def_tw_buffer;
    GtkWidget *lbl_dictnum;
    GtkWidget *lbl_dictname;
    GtkWidget *lbl_freq;
    GtkWidget *lbl_cur_reading;
    GtkWidget *exists_dot;
} dynamic_widgets_s;

typedef struct {
    WinArgs *winargs;
    gint *cur_entry_num;
} local_win_variables_s;

/* -------- GLOBALS ------------- */
dynamic_widgets_s dw = {0};
static int exists_in_anki = 0;

static GMutex dict_mutex;
static GCond vars_set_condition;
bool _Atomic UI_READY;
bool _Atomic DICT_DATA_READY;
dictentry *dict = NULL;
/* ------------------------------ */

#define PRON_DEBOUNCE_MICROSEC 500000 // 0.5 sec
#define dictentry_at(i) dictentry_at_index(dict, i)

/*
 * Returns true if time since last call was less than PRON_DEBOUNCE_MICROSEC
 */
static bool debounce(void) {
    static gint64 time_last_call = 0;
    gint64 time_now = g_get_monotonic_time();

    if ((time_now - time_last_call) < PRON_DEBOUNCE_MICROSEC)
        return true;

    time_last_call = time_now;
    return false;
}

static gpointer jppron_caller(gpointer data) {
    dictentry *de = (dictentry *)data;
    assert(de);

    jppron(de->kanji, de->reading, cfg.pron.dirPath);

    dictentry_free(*de);
    free(de);
    return NULL;
}

static void play_pronunciation_for_entry(int entry_num) {
    if (debounce())
        return;

    dictentry *cur_entry_dup = new (dictentry, 1);
    g_mutex_lock(&dict_mutex);
    *cur_entry_dup = dictentry_dup(dictentry_at(entry_num));
    g_mutex_unlock(&dict_mutex);

    // forks don't work with gtk's main loop...
    g_thread_unref(g_thread_new("jppron", jppron_caller, cur_entry_dup));
}

static void audio_button_clicked(GtkButton *self, gpointer user_data) {
    gint *cur_dictentry_num = user_data;
    play_pronunciation_for_entry(*cur_dictentry_num);
}

static gboolean keypress_on_audio_button(GtkWidget *self, GdkEventButton *event,
                                         gpointer user_data) {
    if (event->type == GDK_BUTTON_PRESS && event->button == GDK_BUTTON_SECONDARY) {
        // TODO: Implement
        return FALSE;
    }
    return FALSE;
}

static void anki_check_exists(s8 word) {
    if (!DICT_DATA_READY || !cfg.anki.checkExisting || !cfg.anki.enabled)
        return;

    char *errormsg = NULL;
    exists_in_anki =
        ac_check_exists(cfg.anki.deck, cfg.anki.searchField, (char *)word.s, &errormsg);

    if (errormsg) {
        dbg("%s", errormsg); // Don't notify on startup
        free(errormsg);
        return;
    }

    gtk_widget_queue_draw(dw.exists_dot);
}

static void draw_dot(cairo_t *cr) {
    // The colors are the same Anki uses
    switch (exists_in_anki) {
        case -1:
            // Don't draw on error
            // TODO: Draw when Anki gets started?
            return;
        case 0:
            cairo_set_source_rgb(cr, 0.9490, 0.4431, 0.4431); // red
            break;
        case 1:
            cairo_set_source_rgb(cr, 0.1333, 0.7725, 0.3686); // green
            break;
        case 2:
            cairo_set_source_rgb(cr, 0.5765, 0.7725, 0.9922); // blue
            break;
        case 3:
            cairo_set_source_rgb(cr, 1, 0.5, 0); // orange
            break;
        default:
            dbg("Encountered unknown unkown dot color: %i", exists_in_anki);
            return;
    }
    cairo_arc(cr, 6, 6, 6, 0, 2 * G_PI);
    cairo_fill(cr);
}

static gboolean on_draw_event(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    draw_dot(cr);
    return FALSE;
}

static void update_definition(s8 definition) {
    gtk_text_buffer_set_text(dw.def_tw_buffer, (char *)definition.s, (gint)definition.len);

    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(dw.def_tw_buffer, &start, &end);
    gtk_text_buffer_apply_tag_by_name(dw.def_tw_buffer, "x-large", &start, &end);
}

static void update_win_title(dictentry current_entry) {
    _drop_(frees8) s8 title = concat(S("dictpopup - "), current_entry.reading, S("【"),
                                     current_entry.kanji, S("】from "), current_entry.dictname);
    gtk_window_set_title(dw.win, (char *)title.s);
}

static void update_dictnum_label(int cur_entry_index) {
    char tmp[20] = {0};
    snprintf_safe(tmp, arrlen(tmp), "%i/%li", cur_entry_index + 1, dictlen(dict));
    gtk_label_set_text(GTK_LABEL(dw.lbl_dictnum), tmp);
}

static void update_dictname_label(s8 dictname) {
    gtk_label_set_text(GTK_LABEL(dw.lbl_dictname), (char *)dictname.s);
}

static void update_frequency_label(size_t freq) {
    char tmp[10] = {0};
    const char *freqstr = freq == 0 ? "" : tmp;
    if (freqstr == tmp)
        snprintf_safe(tmp, arrlen(tmp), "%ld", freq);
    gtk_label_set_text(GTK_LABEL(dw.lbl_freq), freqstr);
}

static void update_reading_label(s8 kanji, s8 reading) {
    _drop_(frees8) s8 txt = {0};
    if (kanji.len && reading.len) {
        if (s8equals(kanji, reading))
            txt = s8dup(reading.len ? reading : kanji);
        else
            txt = concat(reading, S("【"), kanji, S("】"));
    } else if (reading.len)
        txt = s8dup(reading);
    else if (kanji.len)
        txt = s8dup(kanji);
    else
        txt = s8dup(S(""));

    gtk_label_set_text(GTK_LABEL(dw.lbl_cur_reading), (char *)txt.s);
}

static void update_widgets(int cur_entry_num) {
    g_mutex_lock(&dict_mutex);
    dictentry cur_ent = dictentry_at(cur_entry_num);
    if (DICT_DATA_READY) {
        update_definition(cur_ent.definition);
        update_win_title(cur_ent);
        update_dictnum_label(cur_entry_num);
        update_dictname_label(cur_ent.dictname);
        update_frequency_label(cur_ent.frequency);
        update_reading_label(cur_ent.kanji, cur_ent.reading);
    }
    g_mutex_unlock(&dict_mutex);
}

static void _nonnull_ previous_dictentry(int *cur_dictentry_num) {
    *cur_dictentry_num = (*cur_dictentry_num != 0) ? *cur_dictentry_num - 1 : dictlen(dict) - 1;
    update_widgets(*cur_dictentry_num);
}

static void _nonnull_ next_dictentry(int *cur_entry_num) {
    *cur_entry_num = (*cur_entry_num < dictlen(dict) - 1) ? *cur_entry_num + 1 : 0;
    update_widgets(*cur_entry_num);
}

static gboolean next_dictentry_button_pressed(GtkButton *self, gpointer user_data) {
    gint *cur_entry_num = user_data;
    ;
    next_dictentry(cur_entry_num);
    return TRUE;
}

static gboolean prev_dictentry_button_pressed(GtkButton *self, gpointer user_data) {
    gint *cur_dictentry_num = user_data;
    previous_dictentry(cur_dictentry_num);
    return TRUE;
}

static void close_window(local_win_variables_s *lv) {
    gtk_widget_destroy(GTK_WIDGET(dw.win));
    free(lv->cur_entry_num);
    free(lv);
}

static bool check_can_add_to_anki(void) {
    if (!ac_check_connection()) {
        err("Cannot connect to Anki. Is Anki running?");
        return false;
    }
    if (!DICT_DATA_READY) {
        err("Cannot access Anki when still loading.");
        return false;
    }
    return true;
}

static void prepare_add_to_anki(WinArgs *winargs, gint cur_entry_num) {
    winargs->create_ac = true;
    winargs->de = dictentry_dup(dictentry_at(cur_entry_num));
}

static void set_anki_definition(WinArgs *winargs, s8 definition) {
    frees8(&winargs->de.definition);
    winargs->de.definition = definition;
}

static void add_to_anki_from_clipboard(GtkWidget *widget, gpointer user_data) {
    local_win_variables_s *lv = user_data;

    if (!check_can_add_to_anki())
        return;

    prepare_add_to_anki(lv->winargs, *lv->cur_entry_num);

    s8 clip = get_clipboard();
    set_anki_definition(lv->winargs, clip);

    close_window(lv);
}

static s8 get_definition_buffer_text_selection(void) {
    GtkTextIter start, end;
    if (gtk_text_buffer_get_selection_bounds(dw.def_tw_buffer, &start, &end))
        return fromcstr_(gtk_text_buffer_get_text(dw.def_tw_buffer, &start, &end, FALSE));
    return (s8){0};
}

static void add_to_anki_normal(local_win_variables_s *lv) {
    if (!check_can_add_to_anki())
        return;

    prepare_add_to_anki(lv->winargs, *lv->cur_entry_num);

    s8 selection = get_definition_buffer_text_selection();
    if (selection.len)
        set_anki_definition(lv->winargs, selection);

    close_window(lv);
}

static void show_anki_button_right_click_menu(GtkWidget *button, local_win_variables_s *lv) {
    GtkWidget *menu = gtk_menu_new();

    GtkWidget *menu_item = gtk_menu_item_new_with_label("Add with clipboard content as definition");
    g_signal_connect(menu_item, "activate", G_CALLBACK(add_to_anki_from_clipboard), lv);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

    gtk_widget_show_all(menu);
    gtk_menu_popup_at_widget(GTK_MENU(menu), button, GDK_GRAVITY_SOUTH, GDK_GRAVITY_WEST, NULL);
}

static gboolean key_press_on_anki_button(GtkWidget *self, GdkEventButton *event,
                                         gpointer user_data) {
    local_win_variables_s *lv = user_data;

    if (event->type == GDK_BUTTON_PRESS && event->button == GDK_BUTTON_SECONDARY) {
        show_anki_button_right_click_menu(self, lv);
    }
    return FALSE;
}

static void anki_button_clicked(GtkButton *self, gpointer user_data) {
    local_win_variables_s *lv = user_data;
    add_to_anki_normal(lv);
}

static void move_win_to_mouse_ptr(GtkWindow *win) {
    GdkDisplay *display = gdk_display_get_default();
    GdkDevice *device = gdk_seat_get_pointer(gdk_display_get_default_seat(display));
    gint x, y;
    gdk_device_get_position(device, NULL, &x, &y);
    gtk_window_move(win, x, y);
}

static void search_in_anki_browser(s8 word) {
    char *errormsg = NULL;
    ac_gui_search(cfg.anki.deck, cfg.anki.searchField, (char *)word.s, &errormsg);
    if (errormsg) {
        err("%s", errormsg);
        free(errormsg);
    }
}

static void button_press_on_dot_indicator(GtkWidget *self, GdkEventButton *event,
                                          gpointer user_data) {
    gint *cur_dictentry_num = user_data;
    if (event->type == GDK_BUTTON_PRESS) {
        g_mutex_lock(&dict_mutex);
        _drop_(frees8) s8 word_dup = dictentry_at(*cur_dictentry_num).kanji;
        g_mutex_unlock(&dict_mutex);

        search_in_anki_browser(word_dup);
    }
}

static GtkTextView *create_definition_text_view(void) {
    GtkTextView *def_tw = GTK_TEXT_VIEW(gtk_text_view_new());
    gtk_text_view_set_editable(def_tw, FALSE);
    gtk_text_view_set_wrap_mode(def_tw, GTK_WRAP_CHAR);
    gtk_text_view_set_cursor_visible(def_tw, FALSE);

    gtk_text_view_set_top_margin(def_tw, cfg.popup.margin);
    gtk_text_view_set_bottom_margin(def_tw, cfg.popup.margin);
    gtk_text_view_set_left_margin(def_tw, cfg.popup.margin);
    gtk_text_view_set_right_margin(def_tw, cfg.popup.margin);
    return def_tw;
}

static bool should_draw_exists_indicator(void) {
    static int should_draw = -1;
    if (should_draw == -1) {
        should_draw = cfg.anki.enabled && cfg.anki.checkExisting && ac_check_connection();
    }
    return should_draw;
}

/* ------------------------- START Closure callbacks ------------------------- */
static void add_to_anki_closure_cb(GtkAccelGroup *self, GObject *acceleratable, guint keyval,
                                   GdkModifierType *modifier, gpointer user_data) {
    local_win_variables_s *lv = user_data;
    add_to_anki_normal(lv);
}

static void play_pronunciation_closure_cb(GtkAccelGroup *self, GObject *acceleratable, guint keyval,
                                          GdkModifierType *modifier, gpointer user_data) {
    int *cur_entry_num = user_data;
    play_pronunciation_for_entry(*cur_entry_num);
}

static void next_def_closure_cb(GtkAccelGroup *self, GObject *acceleratable, guint keyval,
                                GdkModifierType *modifier, gpointer user_data) {
    int *cur_entry_num = user_data;
    next_dictentry(cur_entry_num);
}

static void prev_def_closure_cb(GtkAccelGroup *self, GObject *acceleratable, guint keyval,
                                GdkModifierType *modifier, gpointer user_data) {
    int *cur_entry_num = user_data;
    previous_dictentry(cur_entry_num);
}

static void close_win_closure_cb(GtkAccelGroup *self, GObject *acceleratable, guint keyval,
                                 GdkModifierType *modifier, gpointer user_data) {
    local_win_variables_s *lv = user_data;
    close_window(lv);
}
/* ------------------------- END Closure callbacks ------------------------- */

static void activate(GtkApplication *app, gpointer user_data) {
    WinArgs *args = user_data;
    gint *cur_ent_num = new (gint, 1);
    local_win_variables_s *lv = new (local_win_variables_s, 1);
    lv->winargs = args;
    lv->cur_entry_num = cur_ent_num;

    /* ------------ WINDOW ------------ */
    dw.win = GTK_WINDOW(gtk_application_window_new(app));

    gtk_window_set_default_size(dw.win, cfg.popup.width, cfg.popup.height);
    gtk_window_set_decorated(dw.win, FALSE);
    gtk_window_set_type_hint(dw.win, GDK_WINDOW_TYPE_HINT_POPUP_MENU);
    gtk_window_set_resizable(dw.win, FALSE);
    gtk_window_set_keep_above(dw.win, 1);
    move_win_to_mouse_ptr(dw.win);
    /* -------------------------------- */

    GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(dw.win), main_vbox);

    /* ------------ TOP BAR ------------ */
    const int spacing_between_widgets = 0;
    GtkWidget *top_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, spacing_between_widgets);
    gtk_box_pack_start(GTK_BOX(main_vbox), top_bar, 0, 0, 0);

    GtkWidget *btn_l = gtk_button_new_from_icon_name("go-previous-symbolic", 2);
    g_signal_connect(btn_l, "clicked", G_CALLBACK(prev_dictentry_button_pressed), cur_ent_num);
    gtk_box_pack_start(GTK_BOX(top_bar), btn_l, FALSE, FALSE, 0);

    // Add to anki button
    if (cfg.anki.enabled) {
        GtkWidget *btn_add_anki = gtk_button_new_from_icon_name("list-add-symbolic", 2);
        g_signal_connect(btn_add_anki, "clicked", G_CALLBACK(anki_button_clicked), lv);
        g_signal_connect(btn_add_anki, "button-press-event", G_CALLBACK(key_press_on_anki_button),
                         lv);
        gtk_box_pack_start(GTK_BOX(top_bar), btn_add_anki, FALSE, FALSE, 0);
    }

    if (should_draw_exists_indicator()) {
        dw.exists_dot = gtk_drawing_area_new();
        gtk_widget_set_size_request(dw.exists_dot, 12, 12);
        gtk_widget_set_valign(dw.exists_dot, GTK_ALIGN_CENTER);
        g_signal_connect(G_OBJECT(dw.exists_dot), "draw", G_CALLBACK(on_draw_event), NULL);

        gtk_widget_add_events(dw.exists_dot, GDK_BUTTON_PRESS_MASK);
        g_signal_connect(dw.exists_dot, "button-press-event",
                         G_CALLBACK(button_press_on_dot_indicator), cur_ent_num);

        gtk_box_pack_start(GTK_BOX(top_bar), GTK_WIDGET(dw.exists_dot), FALSE, FALSE, 10);
    }

    dw.lbl_cur_reading = gtk_label_new(NULL);
    gtk_box_set_center_widget(GTK_BOX(top_bar), dw.lbl_cur_reading);

    GtkWidget *btn_r = gtk_button_new_from_icon_name("go-next-symbolic", 2);
    g_signal_connect(btn_r, "clicked", G_CALLBACK(next_dictentry_button_pressed), cur_ent_num);
    gtk_box_pack_end(GTK_BOX(top_bar), btn_r, FALSE, FALSE, 0);

    if (cfg.pron.displayButton) {
        GtkWidget *btn_pron = gtk_button_new_from_icon_name("audio-volume-high-symbolic", 2);
        g_signal_connect(btn_pron, "clicked", G_CALLBACK(audio_button_clicked), cur_ent_num);
        g_signal_connect(btn_pron, "button-press-event", G_CALLBACK(keypress_on_audio_button),
                         NULL);
        gtk_box_pack_end(GTK_BOX(top_bar), btn_pron, FALSE, FALSE, 0);
    }

    dw.lbl_dictnum = gtk_label_new(NULL);
    gtk_box_pack_end(GTK_BOX(top_bar), dw.lbl_dictnum, FALSE, FALSE, 5);
    /* --------------------------------- */

    /* ------------ TEXT WIDGET ------------ */
    GtkWidget *swindow = gtk_scrolled_window_new(NULL, NULL);
    gtk_box_pack_start(GTK_BOX(main_vbox), swindow, TRUE, TRUE, 0);

    dw.def_tw = create_definition_text_view();
    gtk_container_add(GTK_CONTAINER(swindow), GTK_WIDGET(dw.def_tw));

    dw.def_tw_buffer = gtk_text_view_get_buffer(dw.def_tw);
    gtk_text_buffer_set_text(dw.def_tw_buffer, "loading...", -1);
    gtk_text_buffer_create_tag(dw.def_tw_buffer, "x-large", "scale", PANGO_SCALE_X_LARGE, NULL);
    /* ------------------------------------- */

    /* ------- BOTTOM BAR-------------------- */
    GtkWidget *bottom_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, spacing_between_widgets);
    gtk_box_pack_start(GTK_BOX(main_vbox), bottom_bar, 0, 0, 0);

    dw.lbl_freq = gtk_label_new(NULL);
    gtk_box_pack_start(GTK_BOX(bottom_bar), dw.lbl_freq, FALSE, FALSE, 0);

    dw.lbl_dictname = gtk_label_new(NULL);
    gtk_box_set_center_widget(GTK_BOX(bottom_bar), dw.lbl_dictname);
    /* --------------------------- */

    /* ------------------- START Shortcuts ------------------- */
    GtkAccelGroup *accel_group = gtk_accel_group_new();

    GClosure *pron_closure =
        g_cclosure_new(G_CALLBACK(play_pronunciation_closure_cb), cur_ent_num, 0);
    gtk_accel_group_connect(accel_group, GDK_KEY_r, 0, 0, pron_closure);
    GClosure *anki_closure = g_cclosure_new(G_CALLBACK(add_to_anki_closure_cb), lv, 0);
    gtk_accel_group_connect(accel_group, GDK_KEY_s, GDK_CONTROL_MASK, 0, anki_closure);
    GClosure *next_def_closure = g_cclosure_new(G_CALLBACK(next_def_closure_cb), cur_ent_num, 0);
    gtk_accel_group_connect(accel_group, GDK_KEY_s, 0, 0, next_def_closure);
    GClosure *prev_def_closure = g_cclosure_new(G_CALLBACK(prev_def_closure_cb), cur_ent_num, 0);
    gtk_accel_group_connect(accel_group, GDK_KEY_a, 0, 0, prev_def_closure);
    GClosure *close_win_closure1 = g_cclosure_new(G_CALLBACK(close_win_closure_cb), lv, 0);
    gtk_accel_group_connect(accel_group, GDK_KEY_Escape, 0, 0, close_win_closure1);
    GClosure *close_win_closure2 = g_cclosure_new(G_CALLBACK(close_win_closure_cb), lv, 0);
    gtk_accel_group_connect(accel_group, GDK_KEY_q, 0, 0, close_win_closure2);

    gtk_window_add_accel_group(GTK_WINDOW(dw.win), accel_group);
    /* ------------------- END Shortcuts ------------------- */

    atomic_store(&UI_READY, true);
    g_mutex_lock(&dict_mutex);
    g_cond_signal(&vars_set_condition);
    g_mutex_unlock(&dict_mutex);

    gtk_widget_show_all(GTK_WIDGET(dw.win));
}

static void wait_for_ui_ready(void) {
    if (!UI_READY) {
        g_mutex_lock(&dict_mutex);
        while (!UI_READY)
            g_cond_wait(&vars_set_condition, &dict_mutex);
        g_mutex_unlock(&dict_mutex);
    }
}

static void on_dict_update(dictentry *first_entry) {
    if (cfg.pron.onStart)
        play_pronunciation_for_entry(0);

    update_widgets(0);
    if (should_draw_exists_indicator())
        anki_check_exists(first_entry->kanji); // TODO: Potentially a race condition
}

static void update_window_with_dict(dictentry *passed_dict) {
    dictentry *old_dict = dict;

    g_mutex_lock(&dict_mutex);
    dict = passed_dict;
    g_mutex_unlock(&dict_mutex);
    atomic_store(&DICT_DATA_READY, true);

    wait_for_ui_ready();
    on_dict_update(pointer_to_entry_at(dict, 0));

    dictionary_free(&old_dict);
}

static gpointer dictionary_lookup_thread(gpointer voidin) {
    s8 *lookup = voidin;
    static bool first_invocation = true;

    dictentry *local_dict = create_dictionary(lookup);
    if (local_dict == NULL) {
        msg("No dictionary entry found");

        if (first_invocation)
            exit(EXIT_FAILURE);
    } else {
        update_window_with_dict(local_dict);
    }

    first_invocation = false;
    return NULL;
}

static int parse_cmd_line_opts(int argc, char **argv) {
    int c;
    opterr = 0;
    bool print_cfg = false;

    while ((c = getopt(argc, argv, "chd:")) != -1)
        switch (c) {
            case 'c':
                print_cfg = true;
                break;
            case 'd':
                if (optarg && *optarg) {
                    free(cfg.general.dbpth); // 注意
                    cfg.general.dbpth = strdup(optarg);
                }
                break;
            case 'h':
                puts("See 'man dictpopup' for help.");
                exit(EXIT_SUCCESS);
            case '?':
                fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
                exit(EXIT_FAILURE);
            default:;
        }

    if (print_cfg) // Make sure to parse -d before printing
        print_settings();

    return optind;
}

static void run_popup(WinArgs *winargs) {
    GtkApplication *app =
        gtk_application_new("com.github.Ajatt-Tools.dictpopup", G_APPLICATION_NON_UNIQUE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), winargs);
    g_application_run(G_APPLICATION(app), 0, NULL);
    g_object_unref(app);
}

int main(int argc, char *argv[]) {
    dictpopup_init();
    int nextarg = parse_cmd_line_opts(argc, argv); // Should be second to overwrite settings

    _drop_(frees8) s8 lookup =
        argc - nextarg > 0 ? convert_to_utf8(argv[nextarg]) : get_selection();
    die_on(!lookup.len, "No selection and no argument provided. Exiting..");
    die_on(!g_utf8_validate((char *)lookup.s, lookup.len, NULL),
           "Lookup is not a valid UTF-8 string.");

    g_thread_unref(g_thread_new("Dictionary lookup thread", dictionary_lookup_thread, &lookup));

    WinArgs winargs = {0};
    run_popup(&winargs);

    if (winargs.create_ac)
        create_ankicard(lookup, winargs.de, cfg);
}