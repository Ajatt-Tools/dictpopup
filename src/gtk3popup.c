#include <gtk/gtk.h>
#include <stdbool.h>

#include "settings.h"
#include "ankiconnectc.h"
#include "util.h"
#include "jppron.h"

#include "messages.h"
#define CHECK_AC_RESP(retval)                         \
	do {                                          \
	    if (!retval.ok)                           \
	    {                                         \
		error_msg("%s", ac_resp.data.string); \
		return;                               \
	    }                                         \
	}                                             \
	while (0)

// TODO: Remove globals or store in struct

typedef struct {
} popwin;

static GMutex vars_mutex;
static GCond vars_set_condition;
static gboolean gtk_vars_set = FALSE;
static gboolean dict_data_ready = FALSE;

static dictentry* dict = NULL;
static dictentry curent = { 0 };
static size curent_num = 0;

static GtkWidget *window = NULL;
static GtkWidget *dict_tw;
static GtkTextBuffer *dict_tw_buffer;
static GtkWidget *lbl_dictnum;
static GtkWidget *lbl_dictname;
static GtkWidget *lbl_freq;
static GtkWidget *lbl_cur_reading;
static GtkWidget *exists_dot;

static int exists_in_anki = 0;
gint64 time_last_pron = 0;


static void play_pronunciation(void);
static void update_window(void);
static void enable_buttons(void);
static void anki_check_exists(void);

void
dictionary_data_done(dictentry* passed_dict)
{
    g_mutex_lock(&vars_mutex);

    dict = passed_dict;
    curent = dictentry_at_index(dict, curent_num);
    dict_data_ready = TRUE;

    while (!gtk_vars_set)
	g_cond_wait(&vars_set_condition, &vars_mutex);

    g_mutex_unlock(&vars_mutex);


    if (cfg.pron.onStart)
	play_pronunciation();

    update_window();
    enable_buttons();
    anki_check_exists();     // Check only once
}

static void
enable_buttons(void)
{
    // TODO: Implement
    // Buttons should be disabled at the beginning
}

static void
anki_check_exists(void)
{
    if (!cfg.anki.checkExisting || !cfg.anki.enabled)
	return;

    // exclude suspended cards
    retval_s ac_resp = ac_search(false, cfg.anki.deck, cfg.anki.searchField, (char*)curent.kanji.s);
    CHECK_AC_RESP(ac_resp);

    exists_in_anki = ac_resp.data.boolean;
    if (!exists_in_anki)
    {
	// include suspended cards
	ac_resp = ac_search(true, cfg.anki.deck, cfg.anki.searchField, (char*)curent.kanji.s);
	CHECK_AC_RESP(ac_resp);

	if (ac_resp.data.boolean)
	    exists_in_anki = 2;
    }

    gtk_widget_queue_draw(exists_dot);
}

static void*
jppron_caller(void* voidin)
{
    dictentry* de = (dictentry*)voidin;
    jppron(de->kanji, de->reading, cfg.pron.dirPath);
    return NULL;
}

static void
play_pronunciation(void)
{
    gint64 time_now = g_get_monotonic_time();
    if ((time_now - time_last_pron) < 500000) // 500ms
	return;
    time_last_pron = time_now;

    pthread_t thread;
    pthread_create(&thread, NULL, jppron_caller, &curent);
    pthread_detach(thread);
}

static void
draw_dot(cairo_t *cr)
{
    switch (exists_in_anki)
    {
	case 0:
	    cairo_set_source_rgb(cr, 1, 0, 0);         // red
	    break;
	case 1:
	    cairo_set_source_rgb(cr, 0, 1, 0);         // green
	    break;
	case 2:
	    cairo_set_source_rgb(cr, 1, 0.5, 0);       //orange
	    break;
	default:
	    debug_msg("Encountered unkown color for exists dot");

    }
    cairo_arc(cr, 6, 6, 6, 0, 2 * G_PI);
    cairo_fill(cr);
}

static gboolean
on_draw_event(GtkWidget *widget, cairo_t *cr)
{
    draw_dot(cr);
    return FALSE;
}

static void
update_buffer(void)
{
    gtk_text_buffer_set_text(dict_tw_buffer, (char*)curent.definition.s, (gint)curent.definition.len);

    GtkTextIter start, end;
    gtk_text_buffer_get_bounds(dict_tw_buffer, &start, &end);
    gtk_text_buffer_apply_tag_by_name(dict_tw_buffer, "x-large", &start, &end);
}

static void
update_win_title(void)
{
    s8 title = concat(S("dictpopup - "), curent.reading, S("【"), curent.kanji, S("】from "), curent.dictname);
    gtk_window_set_title(GTK_WINDOW(window), (char*)title.s);
    frees8(&title);
}

static void
update_dictnum_info(void)
{
    g_autofree char* info_txt = g_strdup_printf("%li/%li", curent_num + 1, dictlen(dict));
    gtk_label_set_text(GTK_LABEL(lbl_dictnum), info_txt);
}

static void
update_dictname_info(void)
{
    gtk_label_set_text(GTK_LABEL(lbl_dictname), (char*)curent.dictname.s);
}

static void
update_frequency_info(void)
{
    char tmp[12];
    char* freqstr = curent.frequency == -1 ? "" : tmp;
    if (freqstr == tmp)
	snprintf(tmp, sizeof(tmp), "%d", curent.frequency);
    gtk_label_set_text(GTK_LABEL(lbl_freq), freqstr);
}

static void
update_cur_reading(void)
{
    s8 txt = { 0 };
    if (curent.kanji.len && curent.reading.len)
    {
	if (s8equals(curent.kanji, curent.reading))
	    txt = s8dup(curent.reading.len ? curent.reading : curent.kanji);
	else
	    txt = concat(curent.reading, S("【"), curent.kanji, S("】"));
    }
    else if (curent.reading.len)
	txt = s8dup(curent.reading);
    else if (curent.kanji.len)
	txt = s8dup(curent.kanji);
    else
	txt = s8dup(S(""));

    gtk_label_set_text(GTK_LABEL(lbl_cur_reading), (char*)txt.s);
    frees8(&txt);
}

static void
update_window(void)
{
    if (dict_data_ready)
    {
	update_buffer();
	update_win_title();
	update_dictnum_info();
	update_dictname_info();
	update_frequency_info();
	update_cur_reading();
    }
}

static gboolean
change_de(char c)
{
    assert(c == '+' || c == '-');

    if (c == '+')
	curent_num = (curent_num != dictlen(dict) - 1) ? curent_num + 1 : 0;
    else if (c == '-')
	curent_num = (curent_num != 0) ? curent_num - 1 : dictlen(dict) - 1;

    curent = dictentry_at_index(dict, curent_num);
    update_window();
    return TRUE;
}

static void
change_de_up(void)
{
    change_de('+');
}

static void
change_de_down(void)
{
    change_de('-');
}

static void
close_window(void)
{
    gtk_widget_destroy(window);
}

static gboolean
add_anki(void)
{
    g_mutex_lock(&vars_mutex);
    if (!dict_data_ready)
	return FALSE;
    g_mutex_unlock(&vars_mutex);

    curent = dictentry_dup(curent);
    dictionary_free(&dict);

    GtkTextIter start, end;
    if (gtk_text_buffer_get_selection_bounds(dict_tw_buffer, &start, &end))     // Use text selection if existing
    {
	frees8(&curent.definition);
	curent.definition = fromcstr_(gtk_text_buffer_get_text(dict_tw_buffer, &start, &end, FALSE));
    }

    close_window();
    return TRUE;
}

static gboolean
quit(void)
{
    curent = (dictentry){ 0 };
    dictionary_free(&dict);

    close_window();
    return TRUE;
}

static gboolean
key_press_on_win(GtkWidget *widget, GdkEventKey *event)
{
    /* Keyboard shortcuts */
    if (event->keyval == GDK_KEY_Escape || event->keyval == GDK_KEY_q)
	return quit();
    else if ((event->state & GDK_CONTROL_MASK) && (event->keyval == GDK_KEY_s))
	return add_anki();
    else if (event->keyval == GDK_KEY_p || event->keyval == GDK_KEY_a)
	return change_de('-');
    else if (event->keyval == GDK_KEY_n || event->keyval == GDK_KEY_s)
	return change_de('+');
    else if (event->keyval == GDK_KEY_r)
    {
	play_pronunciation();
	return TRUE;
    }

    return FALSE;
}

static void
set_margins(void)
{
    gtk_text_view_set_top_margin(GTK_TEXT_VIEW(dict_tw), cfg.popup.margin);
    gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(dict_tw), cfg.popup.margin);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(dict_tw), cfg.popup.margin);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(dict_tw), cfg.popup.margin);
}

static void
move_win_to_mouse_ptr(void)
{
    GdkDisplay *display = gdk_display_get_default();
    GdkDevice *device = gdk_seat_get_pointer(gdk_display_get_default_seat(display));
    gint x, y;
    gdk_device_get_position(device, NULL, &x, &y);

    gtk_window_move(GTK_WINDOW(window), x, y);
}

static void
search_in_anki_browser(void)
{
    retval_s ac_resp = ac_gui_search(cfg.anki.deck, cfg.anki.searchField, (char*)curent.kanji.s);
    CHECK_AC_RESP(ac_resp);
}

static void
button_press(GtkWidget *widget, GdkEventButton *event)
{
    if (event->type == GDK_BUTTON_PRESS)
	search_in_anki_browser();
}

static void
activate(GtkApplication* app, gpointer user_data)
{
    g_mutex_lock(&vars_mutex);
    /* ------------ WINDOW ------------ */
    window = gtk_application_window_new(app);

    gtk_window_set_default_size(GTK_WINDOW(window), cfg.popup.width, cfg.popup.height);
    gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
    gtk_window_set_type_hint(GTK_WINDOW(window), GDK_WINDOW_TYPE_HINT_POPUP_MENU);
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
    gtk_window_set_keep_above(GTK_WINDOW(window), 1);

    g_signal_connect(window, "delete-event", G_CALLBACK(quit), NULL);
    g_signal_connect(window, "key-press-event", G_CALLBACK(key_press_on_win), NULL);
    /* -------------------------------- */

    GtkWidget* main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window), main_vbox);

    /* ------------ TOP BAR ------------ */
    const int spacing_between_widgets = 0;
    GtkWidget *top_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, spacing_between_widgets);
    gtk_box_pack_start(GTK_BOX(main_vbox), top_bar, 0, 0, 0);

    GtkWidget* btn_l = gtk_button_new_from_icon_name("go-previous-symbolic", 2);
    g_signal_connect(btn_l, "clicked", G_CALLBACK(change_de_down), NULL);
    gtk_box_pack_start(GTK_BOX(top_bar), btn_l, FALSE, FALSE, 0);

    if (cfg.anki.enabled)
    {
	GtkWidget* btn_add_anki = gtk_button_new_from_icon_name("list-add-symbolic", 2);
	g_signal_connect(btn_add_anki, "clicked", G_CALLBACK(add_anki), dict_tw);
	gtk_box_pack_start(GTK_BOX(top_bar), btn_add_anki, FALSE, FALSE, 0);
    }


    if (cfg.anki.enabled && cfg.anki.checkExisting)
    {
	exists_dot = gtk_drawing_area_new();
	gtk_widget_set_size_request(exists_dot, 12, 12);
	gtk_widget_set_valign(exists_dot, GTK_ALIGN_CENTER);
	g_signal_connect(G_OBJECT(exists_dot), "draw", G_CALLBACK(on_draw_event), NULL);

	gtk_widget_add_events(exists_dot, GDK_BUTTON_PRESS_MASK);
	g_signal_connect(exists_dot, "button-press-event", G_CALLBACK(button_press), NULL);

	gtk_box_pack_start(GTK_BOX(top_bar), GTK_WIDGET(exists_dot), FALSE, FALSE, 10);
    }

    lbl_cur_reading = gtk_label_new(NULL);
    gtk_box_set_center_widget(GTK_BOX(top_bar), lbl_cur_reading);


    GtkWidget* btn_r = gtk_button_new_from_icon_name("go-next-symbolic", 2);
    g_signal_connect(btn_r, "clicked", G_CALLBACK(change_de_up), NULL);
    gtk_box_pack_end(GTK_BOX(top_bar), btn_r, FALSE, FALSE, 0);

    if (cfg.pron.displayButton)
    {
	GtkWidget* btn_pron = gtk_button_new_from_icon_name("audio-volume-high-symbolic", 2);
	g_signal_connect(btn_pron, "clicked", G_CALLBACK(play_pronunciation), NULL);
	gtk_box_pack_end(GTK_BOX(top_bar), btn_pron, FALSE, FALSE, 0);
    }

    lbl_dictnum = gtk_label_new(NULL);
    gtk_box_pack_end(GTK_BOX(top_bar), lbl_dictnum, FALSE, FALSE, 5);
    /* --------------------------------- */

    /* ------------ TEXT WIDGET ------------ */
    GtkWidget* swindow = gtk_scrolled_window_new(NULL, NULL);
    gtk_box_pack_start(GTK_BOX(main_vbox), swindow, TRUE, TRUE, 0);

    dict_tw = gtk_text_view_new();
    dict_tw_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(dict_tw));
    gtk_text_buffer_set_text(dict_tw_buffer, "loading...", -1);
    gtk_text_buffer_create_tag(dict_tw_buffer, "x-large", "scale", PANGO_SCALE_X_LARGE, NULL);

    gtk_container_add(GTK_CONTAINER(swindow), dict_tw);

    gtk_text_view_set_editable(GTK_TEXT_VIEW(dict_tw), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(dict_tw), GTK_WRAP_CHAR);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(dict_tw), FALSE);

    set_margins();
    /* ------------------------------------- */

    /* ------- BOTOOM BAR-------------------- */
    GtkWidget *bottom_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, spacing_between_widgets);
    gtk_box_pack_start(GTK_BOX(main_vbox), bottom_bar, 0, 0, 0);

    lbl_freq = gtk_label_new(NULL);
    gtk_box_pack_start(GTK_BOX(bottom_bar), lbl_freq, FALSE, FALSE, 0);

    lbl_dictname = gtk_label_new(NULL);
    gtk_box_set_center_widget(GTK_BOX(bottom_bar), lbl_dictname);
    /* --------------------------- */

    gtk_vars_set = TRUE;
    g_cond_signal(&vars_set_condition);
    g_mutex_unlock(&vars_mutex);

    move_win_to_mouse_ptr();
    gtk_widget_show_all(window);
}


dictentry
popup(void)
{
    GtkApplication* app = gtk_application_new("com.github.btrkeks.dictpopup", G_APPLICATION_NON_UNIQUE);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    g_application_run(G_APPLICATION(app), 0, NULL);
    g_object_unref(app);
    return curent;
}
