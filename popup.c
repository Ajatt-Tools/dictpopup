#include <gtk/gtk.h>
#include <ankiconnectc.h>
#include <stdbool.h>

#include "util.h"
#include "dictionary.h"
#include "readsettings.h"

const char* error;
#define E(expr) if ((error = expr)) { notify(1, "%s", error); return; }

GMutex vars_mutex;
GCond vars_set_condition;
gboolean gtk_vars_set = 0;
gboolean dict_data_ready = 0;

dictionary* dict;
dictentry *cur_entry;
size_t *cur_entry_num = 0;
char **def;

GtkWidget *window = NULL;
GtkWidget *dict_tw;
GtkTextBuffer *dict_tw_buffer;
GtkWidget *dictnum_lbl;
GtkWidget *dictname_lbl;
GtkWidget *exists_dot;

int word_exists_in_db = 0;
int EXIT_CODE = 0; /* 0 = close, 1 = anki */

void update_window();
void check_if_exists();
void play_pronunciation();

void
dictionary_data_done()
{
	g_mutex_lock(&vars_mutex);

	dict_data_ready = TRUE;
	while (!gtk_vars_set)
		g_cond_wait(&vars_set_condition, &vars_mutex);

	g_mutex_unlock(&vars_mutex);

	cur_entry = dictentry_at_index(dict, *cur_entry_num);

	if (cfg.pronounceonstart)
		play_pronunciation();
	update_window();
	check_if_exists(); // Check only once
}

void
check_search_response(bool exists)
{
	word_exists_in_db = exists ? 1 : 0;
}

void
check_if_exists()
{
	if (!cfg.checkexisting || !cfg.ankisupport)
		return;

	// exclude suspended cards
	E(ac_search(false, cfg.deck, cfg.searchfield, cur_entry->kanji, check_search_response));

	if (!word_exists_in_db)
	{
		// include suspended cards
		E(ac_search(true, cfg.deck, cfg.searchfield, cur_entry->kanji, check_search_response));

		if (word_exists_in_db)
			word_exists_in_db = 2;
	}

	gtk_widget_queue_draw(exists_dot);
}

void
play_pronunciation()
{
	if (!printf_cmd_async("jppron '%s'", cur_entry->kanji))
		notify(1, "Error calling jppron. Is it installed?");
}

void
draw_dot(GtkWidget *widget, cairo_t *cr)
{
	if (word_exists_in_db == 1)
		cairo_set_source_rgb(cr, 0, 1, 0); // green
	else if (word_exists_in_db == 2)
		cairo_set_source_rgb(cr, 1, 0.5, 0); //orange
	else
		cairo_set_source_rgb(cr, 1, 0, 0); // red

	cairo_arc(cr, 5, 5, 5, 0, 2 * G_PI);

	cairo_fill(cr);
}

gboolean
on_draw_event(GtkWidget *widget, cairo_t *cr, gpointer user_data)
{
	draw_dot(widget, cr);
	return FALSE;
}

void
update_buffer()
{
	gtk_text_buffer_set_text(dict_tw_buffer, cur_entry->definition, -1);

	GtkTextIter start, end;
	gtk_text_buffer_get_bounds(dict_tw_buffer, &start, &end);
	gtk_text_buffer_apply_tag_by_name(dict_tw_buffer, "x-large", &start, &end);
}

void
update_win_title()
{
	g_autofree char* title = g_strdup_printf("dictpopup - %s (%s)", cur_entry->kanji, cur_entry->dictname);
	gtk_window_set_title(GTK_WINDOW(window), title);
}

void
update_dictnum_info()
{
	g_autofree char* info_txt = g_strdup_printf("%li/%i", *cur_entry_num + 1, dict->len);
	gtk_label_set_text(GTK_LABEL(dictnum_lbl), info_txt);
}

void
update_dictname_info()
{
	gtk_label_set_text(GTK_LABEL(dictname_lbl), cur_entry->dictname);
}

void
update_window()
{
	if (dict_data_ready) // Very unlikely but simultanous access possible
	{
		update_buffer();
		update_win_title();
		update_dictnum_info();
		update_dictname_info();
	}
}

gboolean
change_de(char c)
{
	/* incr == '+' -> increment
	   incr == '-' -> decrement */
	if (c == '+')
		*cur_entry_num = (*cur_entry_num != dict->len - 1) ? *cur_entry_num + 1 : 0;
	else if (c == '-')
		*cur_entry_num = (*cur_entry_num != 0) ? *cur_entry_num - 1 : dict->len - 1;
	else
		g_warning("Wrong usage of change_de function. Doing nothing.");

	cur_entry = dictentry_at_index(dict, *cur_entry_num);
	update_window();
	return TRUE;
}

void
change_de_up()
{
	change_de('+');
}

void
change_de_down()
{
	change_de('-');
}

gboolean
close_with_code(int exit_code)
{
	EXIT_CODE = exit_code;
	gtk_widget_destroy(window);
	gtk_main_quit();
	return TRUE;
}

gboolean
add_anki()
{
	GtkTextIter start, end;
	if (gtk_text_buffer_get_selection_bounds(dict_tw_buffer, &start, &end)) // Use text selection on selection
		*def = gtk_text_buffer_get_text(dict_tw_buffer, &start, &end, FALSE);
	else
		*def = g_strdup(cur_entry->definition);

	return close_with_code(1);
}

gboolean
quit()
{
	return close_with_code(0);
}

gboolean
key_press_on_win(GtkWidget *widget, GdkEventKey *event)
{
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

void
set_margins()
{
	gtk_text_view_set_top_margin(GTK_TEXT_VIEW(dict_tw), cfg.win_margin);
	gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(dict_tw), cfg.win_margin);
	gtk_text_view_set_left_margin(GTK_TEXT_VIEW(dict_tw), cfg.win_margin);
	gtk_text_view_set_right_margin(GTK_TEXT_VIEW(dict_tw), cfg.win_margin);
}

void
move_win_to_mouse_ptr()
{
	GdkDisplay *display = gdk_display_get_default();
	GdkDevice *device = gdk_seat_get_pointer(gdk_display_get_default_seat(display));
	gint x, y;
	gdk_device_get_position(device, NULL, &x, &y);
	gtk_window_move(GTK_WINDOW(window), x, y);
}

void
search_in_anki_browser()
{
	//TODO: Search for all kanji writings?
	const char *err = ac_gui_search(cfg.deck, cfg.searchfield, cur_entry->kanji);
	if (err)
		notify(1, "%s", err);
}

int
popup(dictionary* passed_dict, char **passed_definition, size_t *passed_curde)
{
	dict = passed_dict;
	def = passed_definition;
	cur_entry_num = passed_curde;
	*cur_entry_num = 0;

	/* gtk_init(NULL, NULL); */

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
	dictname_lbl = gtk_label_new(NULL);
	gtk_box_pack_start(GTK_BOX(main_vbox), dictname_lbl, 0, 0, 0);
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


	/* ------------ BOTTOM BAR ------------ */
	GtkWidget *bottom_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
	gtk_widget_set_size_request(bottom_bar, -1, 5);
	gtk_box_pack_start(GTK_BOX(main_vbox), bottom_bar, 0, 0, 0);

	GtkWidget* btn_l = gtk_button_new_with_label("<");
	GtkWidget* btn_r = gtk_button_new_with_label(">");
	GtkWidget* btn_pron = gtk_button_new_from_icon_name("audio-volume-high", 2);
	GtkWidget *add_anki_btn = gtk_button_new_with_label("+");
	g_signal_connect(btn_l, "clicked", G_CALLBACK(change_de_down), NULL);
	g_signal_connect(btn_r, "clicked", G_CALLBACK(change_de_up), NULL);
	g_signal_connect(btn_pron, "clicked", G_CALLBACK(play_pronunciation), NULL);
	g_signal_connect(add_anki_btn, "clicked", G_CALLBACK(add_anki), dict_tw);

	dictnum_lbl = gtk_label_new(NULL);

	exists_dot = gtk_drawing_area_new();
	gtk_widget_set_size_request(exists_dot, 10, 10);
	gtk_widget_set_valign(exists_dot, GTK_ALIGN_CENTER);
	/* g_signal_connect(exists_dot, "clicked", G_CALLBACK(search_in_anki_browser), NULL); */

	gtk_box_pack_start(GTK_BOX(bottom_bar), btn_l, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(bottom_bar), add_anki_btn, FALSE, FALSE, 0);
	if (cfg.checkexisting && cfg.ankisupport)
		gtk_box_pack_start(GTK_BOX(bottom_bar), GTK_WIDGET(exists_dot), FALSE, FALSE, 0);
	gtk_box_set_center_widget(GTK_BOX(bottom_bar), dictnum_lbl);
	gtk_box_pack_end(GTK_BOX(bottom_bar), btn_r, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(bottom_bar), btn_pron, FALSE, FALSE, 0);

	g_signal_connect(G_OBJECT(exists_dot), "draw", G_CALLBACK(on_draw_event), NULL);
	/* ------------------------------------ */

	gtk_vars_set = TRUE;
	g_cond_signal(&vars_set_condition);
	g_mutex_unlock(&vars_mutex);

	gtk_widget_show_all(window);
	gtk_main();

	return EXIT_CODE;
}
