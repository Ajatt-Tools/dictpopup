#include <gtk/gtk.h>
#include <ankiconnectc.h>

#include "config.h"
#include "util.h"
#include "structs.h"

/* variables */
static dictentry **des;
static size_t numde;
static size_t *curde = 0;
static char **definition;

static GtkWidget *window;
static GtkWidget *dict_tw;
static GtkTextBuffer *dict_tw_buffer;
static GtkWidget *dictnum_lbl;
static GtkWidget *dictname_lbl;
static GtkWidget *exists_dot;
static int word_exists = 0;
static int EXIT_CODE = 0; /* 0 = close, 1 = anki */

static gboolean on_draw_event(GtkWidget *widget, cairo_t *cr, gpointer user_data);
static void draw_dot(GtkWidget *widget, cairo_t *cr);

char *
get_cur_word()
{
  return des[*curde]->word;
}

char *
get_cur_def()
{
  return des[*curde]->definition;
}

char *
get_cur_dictname()
{
  return des[*curde]->dictname;
}

void
play_pronunciation()
{
	char *curword, *cmd;
	FILE *fp;

	curword = extract_kanji(get_cur_word());
	asprintf(&cmd, "jppron %s &", curword);

	if(!(fp = popen(cmd, "r")))
	    fprintf(stderr, "Failed calling command: \"%s\". Is jppron istalled?", cmd);
	else
	  pclose(fp);

	free(curword);
	free(cmd);
}

void
draw_dot(GtkWidget *widget, cairo_t *cr)
{
    if (word_exists)
      cairo_set_source_rgb(cr, 0, 1, 0); // green
    else
      cairo_set_source_rgb(cr, 1, 0, 0); // red

    cairo_arc(cr, 5, 5, 5, 0, 2 * G_PI);

    cairo_fill(cr);
}

gboolean
on_draw_event(GtkWidget *widget, cairo_t *cr, gpointer user_data) {
    draw_dot(widget, cr);
    return FALSE;
}

size_t
check_search_response(char *ptr, size_t size, size_t nmemb, void *userdata)
{
  //FIXME
  if(strncmp(ptr, "{\"result\": [], \"error\": null}", nmemb) != 0)
    word_exists = 1;

  gtk_widget_queue_draw(exists_dot);

  return nmemb;
}

void
update_buffer()
{
    gtk_text_buffer_set_text(dict_tw_buffer, get_cur_def(), -1);

    GtkTextIter start, end;
    gtk_text_buffer_get_bounds (dict_tw_buffer, &start, &end);
    gtk_text_buffer_apply_tag_by_name (dict_tw_buffer, "x-large", &start, &end);
}

void
update_win_title()
{
    // FIXME
    static char title[100];
    snprintf(title, sizeof(title), "dictpopup - %s", get_cur_word());
    gtk_window_set_title(GTK_WINDOW(window), title);
}

void
update_dictnum_info()
{
    char info_txt[8]; // max 999 dictionaries
    snprintf(info_txt, sizeof(info_txt), "%li/%li", *curde+1, numde);
    gtk_label_set_text(GTK_LABEL (dictnum_lbl), info_txt);
}

void
update_dictname_info()
{
  gtk_label_set_text(GTK_LABEL (dictname_lbl), get_cur_dictname());
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
      *definition = gtk_text_buffer_get_text(dict_tw_buffer, &start, &end, FALSE);
  else
      *definition = strdup(get_cur_def());

  return close_with_code(1);
}

gboolean
quit()
{
  return close_with_code(0);
}



gboolean
change_de(char c)
{
    /* incr == '+' -> increment
       incr == '-' -> decrement */
    if (c == '+')
	*curde = (*curde != numde - 1) ? *curde+1 : 0;
    else if (c == '-')
	*curde = (*curde != 0) ? *curde-1 : numde-1;
    else
	fprintf(stderr, "INFO: Wrong usage of change_de function. Doing nothing.");

    update_buffer();
    update_win_title();
    update_dictnum_info();
    update_dictname_info();
    return TRUE;
}

void change_de_up() { change_de('+'); }
void change_de_down() { change_de('-'); }

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
check_if_exists()
{
    char *curword = extract_kanji(get_cur_word());
    char *query;
    asprintf(&query, "\"deck:" ANKI_DECK "\" \"" SEARCH_FIELD ":%s\"", curword);

    search_query(query, check_search_response);

    free(curword);
    free(query);
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
popup(GPtrArray *dictionary, char **anki_definition, size_t *dict_num)
{
    des = (dictentry **) dictionary->pdata;
    numde = dictionary->len;
    definition = anki_definition;
    curde = dict_num;
    *curde = 0;

    gtk_init(NULL, NULL);
    
    /* ------------ WINDOW ------------ */
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size (GTK_WINDOW (window), WIN_WIDTH, WIN_HEIGHT);

    update_win_title();

    gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
    gtk_window_set_type_hint(GTK_WINDOW(window), GDK_WINDOW_TYPE_HINT_POPUP_MENU);
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
    gtk_window_set_keep_above(GTK_WINDOW(window), 1);

    move_win_to_mouse_ptr();

    g_signal_connect(window, "delete-event", G_CALLBACK(quit), NULL);
    g_signal_connect(G_OBJECT(window), "key-press-event", G_CALLBACK(key_press_on_win), NULL);
    /* -------------------------------- */

    GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER (window), main_vbox);

    /* ------------ TOP BAR ------------ */
    dictname_lbl = gtk_label_new (NULL);
    gtk_box_pack_start(GTK_BOX(main_vbox), dictname_lbl, 0, 0, 0);
    update_dictname_info();
    /* --------------------------------- */


    /* ------------ TEXT WIDGET ------------ */
    GtkWidget* swindow = gtk_scrolled_window_new(NULL, NULL);
    gtk_box_pack_start (GTK_BOX (main_vbox), swindow, TRUE, TRUE, 0);
    
    dict_tw = gtk_text_view_new ();
    dict_tw_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (dict_tw));

    gtk_text_buffer_create_tag (dict_tw_buffer, "x-large", "scale", PANGO_SCALE_X_LARGE, NULL);

    gtk_container_add (GTK_CONTAINER (swindow), dict_tw);

    gtk_text_view_set_editable(GTK_TEXT_VIEW(dict_tw), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(dict_tw), GTK_WRAP_CHAR);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(dict_tw), FALSE);

    set_margins();

    update_buffer();
    /* ------------------------------------- */


    /* ------------ BOTTOM BAR ------------ */
    GtkWidget *bottom_bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_set_size_request(bottom_bar, -1, 5);
    gtk_box_pack_start(GTK_BOX(main_vbox), bottom_bar, 0, 0, 0);

    GtkWidget* btn_l = gtk_button_new_with_label ("<");
    GtkWidget* btn_r = gtk_button_new_with_label (">");
    GtkWidget* btn_pron = gtk_button_new_from_icon_name("audio-volume-high", 2);
    GtkWidget *add_anki_btn = gtk_button_new_with_label ("+");
    g_signal_connect (btn_l, "clicked", G_CALLBACK (change_de_down), NULL);
    g_signal_connect (btn_r, "clicked", G_CALLBACK (change_de_up), NULL);
    g_signal_connect (btn_pron, "clicked", G_CALLBACK (play_pronunciation), NULL);
    g_signal_connect (add_anki_btn, "clicked", G_CALLBACK (add_anki), dict_tw);

    dictnum_lbl = gtk_label_new (NULL);
    update_dictnum_info();

    exists_dot = gtk_drawing_area_new();
    gtk_widget_set_size_request(exists_dot, 10, 10);
    gtk_widget_set_valign(exists_dot, GTK_ALIGN_CENTER);

    check_if_exists();

    gtk_box_pack_start(GTK_BOX (bottom_bar), btn_l, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX (bottom_bar), add_anki_btn, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX (bottom_bar), GTK_WIDGET (exists_dot), FALSE, FALSE, 0);
    gtk_box_set_center_widget(GTK_BOX (bottom_bar), dictnum_lbl);
    gtk_box_pack_end(GTK_BOX (bottom_bar), btn_r, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX (bottom_bar), btn_pron, FALSE, FALSE, 0);

    g_signal_connect(G_OBJECT(exists_dot), "draw", G_CALLBACK(on_draw_event), NULL);
    /* ------------------------------------ */

    gtk_widget_show_all(window);
    gtk_main();
    
    return EXIT_CODE;
}
