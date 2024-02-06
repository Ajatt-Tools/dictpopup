#include <gtk/gtk.h>
#include <stdbool.h>

#include "util.h"
#include "dictionary.h"
#include "readsettings.h"
#include "ankiconnectc.h"

// TODO: Remove globals or store in struct

static GMutex vars_mutex;
static GCond vars_set_condition;
static gboolean gtk_vars_set = 0;
static gboolean dict_data_ready = 0;

static dictionary* dict = NULL;
static dictentry *cur_entry = NULL;
static unsigned int cur_entry_num = 0;

static GtkWidget *window = NULL;
static GtkWidget *dict_tw;
static GtkTextBuffer *dict_tw_buffer;
static GtkWidget *lbl_dictnum;
static GtkWidget *lbl_dictname;
static GtkWidget *exists_dot;

static int word_exists_in_db = 0;

static void update_window();
static void check_if_exists();
static void play_pronunciation();

void
dictionary_data_done(dictionary* passed_dict)
{
	g_mutex_lock(&vars_mutex);

	dict = passed_dict;
	dict_data_ready = TRUE;
	while (!gtk_vars_set)
		g_cond_wait(&vars_set_condition, &vars_mutex);

	g_mutex_unlock(&vars_mutex);

	cur_entry = dictentry_at_index(dict, cur_entry_num);

	if (cfg.pronounceonstart)
		play_pronunciation();
	update_window();
	check_if_exists(); // Check only once
}

static void
check_if_exists()
{
	if (!cfg.checkexisting || !cfg.ankisupport)
		return;

	// exclude suspended cards
	retval_s ac_resp = ac_search(false, cfg.deck, cfg.searchfield, cur_entry->kanji);
	if (!check_ac_response(ac_resp))
		return;

	word_exists_in_db = ac_resp.data.boolean;
	if (!word_exists_in_db)
	{
		// include suspended cards
		ac_resp = ac_search(true, cfg.deck, cfg.searchfield, cur_entry->kanji);
		if (!check_ac_response(ac_resp))
			return;

		if (ac_resp.data.boolean)
			word_exists_in_db = 2;
	}

	gtk_widget_queue_draw(exists_dot);
}

static void
play_pronunciation()
{
	if (!printf_cmd_async("jppron '%s'", cur_entry->kanji))
		notify(1, "Error calling jppron. Is it installed?");
}

static void
draw_dot(cairo_t *cr)
{
	if (word_exists_in_db == 1)
		cairo_set_source_rgb(cr, 0, 1, 0); // green
	else if (word_exists_in_db == 2)
		cairo_set_source_rgb(cr, 1, 0.5, 0); //orange
	else
		cairo_set_source_rgb(cr, 1, 0, 0); // red

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
update_buffer()
{
	gtk_text_buffer_set_text(dict_tw_buffer, cur_entry->definition, -1);

	GtkTextIter start, end;
	gtk_text_buffer_get_bounds(dict_tw_buffer, &start, &end);
	gtk_text_buffer_apply_tag_by_name(dict_tw_buffer, "x-large", &start, &end);
}

static void
update_win_title()
{
	g_autofree char* title = g_strdup_printf("dictpopup - %s (%s)", cur_entry->kanji, cur_entry->dictname);
	gtk_window_set_title(GTK_WINDOW(window), title);
}

static void
update_dictnum_info()
{
	g_autofree char* info_txt = g_strdup_printf("%i/%i", cur_entry_num + 1, dict->len);
	gtk_label_set_text(GTK_LABEL(lbl_dictnum), info_txt);
}

static void
update_dictname_info()
{
	gtk_label_set_text(GTK_LABEL(lbl_dictname), cur_entry->dictname);
}

static void
update_window()
{
	if (dict_data_ready) // TODO: Very unlikely but data race possible
	{
		update_buffer();
		update_win_title();
		update_dictnum_info();
		update_dictname_info();
	}
}

static gboolean
change_de(char c)
{
	assert(c == '+' || c == '-');

	if (c == '+')
		cur_entry_num = (cur_entry_num != dict->len - 1) ? cur_entry_num + 1 : 0;
	else if (c == '-')
		cur_entry_num = (cur_entry_num != 0) ? cur_entry_num - 1 : dict->len - 1;

	cur_entry = dictentry_at_index(dict, cur_entry_num);
	update_window();
	return TRUE;
}

static void
change_de_up()
{
	change_de('+');
}

static void
change_de_down()
{
	change_de('-');
}

static void
close_window()
{
	gtk_widget_destroy(window);
	gtk_main_quit();
}

static gboolean
add_anki()
{
	g_mutex_lock(&vars_mutex);
	if (!dict_data_ready)
		return FALSE;
	g_mutex_unlock(&vars_mutex);

	cur_entry = dictentry_dup(*cur_entry);
	dictionary_free(dict);

	GtkTextIter start, end;
	if (gtk_text_buffer_get_selection_bounds(dict_tw_buffer, &start, &end)) // Use text selection if existing
	{
		g_free(cur_entry->definition);
		cur_entry->definition = gtk_text_buffer_get_text(dict_tw_buffer, &start, &end, FALSE);
	}

	close_window();
	return TRUE;
}

static gboolean
quit()
{
	cur_entry = NULL;
	dictionary_free(dict);
	close_window();

	return TRUE;
}

static gboolean
key_press_on_win(GtkWidget *widget, GdkEventKey *event)
{
	assert(event);

	/* Keyboard shortcuts */
	if (event->keyval == GDK_KEY_Escape || event->keyval == GDK_KEY_q)
		return quit();
	else if ((event->state & GDK_CONTROL_MASK) && (event->keyval == GDK_KEY_s))
		return add_anki();
	else if (event->keyval == GDK_KEY_p)
		return change_de('-');
	else if (event->keyval == GDK_KEY_n)
		return change_de('+');

	return FALSE;
}

static void
set_margins()
{
	gtk_text_view_set_top_margin(GTK_TEXT_VIEW(dict_tw), cfg.win_margin);
	gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(dict_tw), cfg.win_margin);
	gtk_text_view_set_left_margin(GTK_TEXT_VIEW(dict_tw), cfg.win_margin);
	gtk_text_view_set_right_margin(GTK_TEXT_VIEW(dict_tw), cfg.win_margin);
}

static void
move_win_to_mouse_ptr()
{
	GdkDisplay *display = gdk_display_get_default();
	GdkDevice *device = gdk_seat_get_pointer(gdk_display_get_default_seat(display));
	gint x, y;
	gdk_device_get_position(device, NULL, &x, &y);
	gtk_window_move(GTK_WINDOW(window), x, y);
}

static void
search_in_anki_browser()
{
	check_ac_response(ac_gui_search(cfg.deck, cfg.searchfield, cur_entry->kanji));
}

static void
button_press(GtkWidget *widget, GdkEventButton *event)
{
	assert(event);

	if (event->type == GDK_BUTTON_PRESS)
		search_in_anki_browser();
}

dictentry*
popup()
{
	gtk_init(NULL, NULL);

	g_mutex_lock(&vars_mutex);
	/* ------------ WINDOW ------------ */
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(window), cfg.win_width, cfg.win_height);

	gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
	gtk_window_set_type_hint(GTK_WINDOW(window), GDK_WINDOW_TYPE_HINT_POPUP_MENU);
	gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
	gtk_window_set_keep_above(GTK_WINDOW(window), 1);

	move_win_to_mouse_ptr();

	g_signal_connect(window, "delete-event", G_CALLBACK(quit), NULL);
	g_signal_connect(G_OBJECT(window), "key-press-event", G_CALLBACK(key_press_on_win), NULL);
	/* -------------------------------- */

	GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add(GTK_CONTAINER(window), main_vbox);

	/* ------------ TOP BAR ------------ */
	int spacing_between_widgets = 0;
	GtkWidget *top_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, spacing_between_widgets);
	gtk_box_pack_start(GTK_BOX(main_vbox), top_bar, 0, 0, 0);


	GtkWidget* btn_l = gtk_button_new_from_icon_name("go-previous", 2);
	g_signal_connect(btn_l, "clicked", G_CALLBACK(change_de_down), NULL);
	gtk_box_pack_start(GTK_BOX(top_bar), btn_l, FALSE, FALSE, 0);

	if (cfg.ankisupport)
	{
		GtkWidget* btn_add_anki = gtk_button_new_from_icon_name("list-add", 2);
		g_signal_connect(btn_add_anki, "clicked", G_CALLBACK(add_anki), dict_tw);
		gtk_box_pack_start(GTK_BOX(top_bar), btn_add_anki, FALSE, FALSE, 0);
	}


	if (cfg.ankisupport && cfg.checkexisting)
	{
		exists_dot = gtk_drawing_area_new();
		gtk_widget_set_size_request(exists_dot, 12, 12);
		gtk_widget_set_valign(exists_dot, GTK_ALIGN_CENTER);
		g_signal_connect(G_OBJECT(exists_dot), "draw", G_CALLBACK(on_draw_event), NULL);

		gtk_widget_add_events(exists_dot, GDK_BUTTON_PRESS_MASK);
		g_signal_connect(exists_dot, "button-press-event", G_CALLBACK(button_press), NULL);

		gtk_box_pack_start(GTK_BOX(top_bar), GTK_WIDGET(exists_dot), FALSE, FALSE, 10);
	}

	lbl_dictname = gtk_label_new(NULL);
	gtk_box_set_center_widget(GTK_BOX(top_bar), lbl_dictname);


	GtkWidget* btn_r = gtk_button_new_from_icon_name("go-next", 2);
	g_signal_connect(btn_r, "clicked", G_CALLBACK(change_de_up), NULL);
	gtk_box_pack_end(GTK_BOX(top_bar), btn_r, FALSE, FALSE, 0);

	if (cfg.pronunciationbutton)
	{
		GtkWidget* btn_pron = gtk_button_new_from_icon_name("audio-volume-high", 2);
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

	gtk_vars_set = TRUE;
	g_cond_signal(&vars_set_condition);
	g_mutex_unlock(&vars_mutex);

	gtk_widget_show_all(window);
	gtk_main();

	return cur_entry;
}
