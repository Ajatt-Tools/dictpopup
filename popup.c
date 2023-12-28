#include <gtk/gtk.h>
#include <ankiconnectc.h>

#include "util.h"
#include "structs.h"
#include "readsettings.h"

#define WIN_WIDTH 530
#define WIN_HEIGHT 350
#define WIN_MARGIN 5

GMutex vars_mutex;
GCond vars_set_condition;
gboolean vars_set = 0;

GPtrArray *dict;
size_t *curde = 0;
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

void
dictionary_data_done(settings *cfg)
{
	g_mutex_lock(&vars_mutex);
	while (!vars_set)
		g_cond_wait(&vars_set_condition, &vars_mutex);
	g_mutex_unlock(&vars_mutex);

	update_window();
	check_if_exists(cfg); // Check only once
}

void
check_search_response(int exists)
{
	if (word_exists_in_db)
		return;
	else if (exists)
		word_exists_in_db = 1;

	gtk_widget_queue_draw(exists_dot);
}

enum { WORD, DEFINITION, DICTNAME };

const char*
get_cur(int entry)
{
	dictentry *cur_entry = g_ptr_array_index(dict, *curde);
	const char *retstr = NULL;

	switch (entry)
	{
	case WORD:
		retstr = cur_entry->word;
		break;
	case DEFINITION:
		retstr = cur_entry->definition;
		break;
	case DICTNAME:
		retstr = cur_entry->dictname;
		break;
	}

	return retstr;
}

void
check_if_exists(settings *cfg)
{
	// Accessing user settings should be safe since update occurs after reading dict
	if(!cfg->checkexisting || !cfg->ankisupport)
	  return;

	char **kanji_writings = extract_kanji_array(get_cur(WORD));
	char **ptr = kanji_writings;
	const char *err = NULL;
	while (*ptr && !err)
	{
		err = ac_search(cfg->deck, cfg->searchfield, *ptr++, check_search_response);
		if (err) notify("Could not connect to AnkiConnect. Is Anki running?");
	}

	g_strfreev(kanji_writings);
}

void
play_pronunciation()
{
	// TODO: Look up different writing if nothing found. Currently simply uses first one
	const char* curword = get_cur(WORD);
	char **kanji_writings = extract_kanji_array(curword);
	const char *pron_word = *kanji_writings ? *kanji_writings : curword;

	if (!spawn_cmd_async("jppron '%s'", pron_word))
		notify("Error calling jppron. Is it installed?");

	g_strfreev(kanji_writings);
}

void
draw_dot(GtkWidget *widget, cairo_t *cr)
{
	if (word_exists_in_db)
		cairo_set_source_rgb(cr, 0, 1, 0); // green
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
	gtk_text_buffer_set_text(dict_tw_buffer, get_cur(DEFINITION), -1);

	GtkTextIter start, end;
	gtk_text_buffer_get_bounds(dict_tw_buffer, &start, &end);
	gtk_text_buffer_apply_tag_by_name(dict_tw_buffer, "x-large", &start, &end);
}

void
update_win_title()
{
	// Might switch to dynamic, but there isn't much space anyway in the title bar.
	char title[100];
	snprintf(title, sizeof(title), "dictpopup - %s", get_cur(WORD));

	gtk_window_set_title(GTK_WINDOW(window), title);
}

void
update_dictnum_info()
{
	char info_txt[8]; // max 999 dictionaries
	snprintf(info_txt, sizeof(info_txt), "%li/%i", *curde + 1, dict->len);

	gtk_label_set_text(GTK_LABEL(dictnum_lbl), info_txt);
}

void
update_dictname_info()
{
	gtk_label_set_text(GTK_LABEL(dictname_lbl), get_cur(DICTNAME));
}

void
update_window()
{
	update_buffer();
	update_win_title();
	update_dictnum_info();
	update_dictname_info();
}

gboolean
change_de(char c)
{
	/* incr == '+' -> increment
	   incr == '-' -> decrement */
	if (c == '+')
		*curde = (*curde != dict->len - 1) ? *curde + 1 : 0;
	else if (c == '-')
		*curde = (*curde != 0) ? *curde - 1 : dict->len - 1;
	else
		fprintf(stderr, "ERROR: Wrong usage of change_de function. Doing nothing.");

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
		*def = g_strdup(get_cur(DEFINITION));

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
	gtk_text_view_set_top_margin(GTK_TEXT_VIEW(dict_tw), WIN_MARGIN);
	gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(dict_tw), WIN_MARGIN);
	gtk_text_view_set_left_margin(GTK_TEXT_VIEW(dict_tw), WIN_MARGIN);
	gtk_text_view_set_right_margin(GTK_TEXT_VIEW(dict_tw), WIN_MARGIN);
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

int
popup(GPtrArray *passed_dictionary, char **passed_definition, size_t *passed_curde, settings* cfg)
{
	dict = passed_dictionary;
	def = passed_definition;
	curde = passed_curde;
	*curde = 0;

	gtk_init(NULL, NULL);

	g_mutex_lock(&vars_mutex);
	/* ------------ WINDOW ------------ */
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WINDOW(window), WIN_WIDTH, WIN_HEIGHT);

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

	gtk_box_pack_start(GTK_BOX(bottom_bar), btn_l, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(bottom_bar), add_anki_btn, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(bottom_bar), GTK_WIDGET(exists_dot), FALSE, FALSE, 0);
	gtk_box_set_center_widget(GTK_BOX(bottom_bar), dictnum_lbl);
	gtk_box_pack_end(GTK_BOX(bottom_bar), btn_r, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(bottom_bar), btn_pron, FALSE, FALSE, 0);

	g_signal_connect(G_OBJECT(exists_dot), "draw", G_CALLBACK(on_draw_event), NULL);
	/* ------------------------------------ */

	vars_set = 1;
	g_cond_signal(&vars_set_condition);
	g_mutex_unlock(&vars_mutex);

	gtk_widget_show_all(window);
	gtk_main();

	return EXIT_CODE;
}
