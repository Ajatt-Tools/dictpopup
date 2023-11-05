#include <gtk/gtk.h>
#include "xlib.h"
#include "config.h"
#include "structs.h"

#include <wchar.h>

// FIXME: Non global?
struct dictentry *des;
size_t *curde;
size_t numde;

GtkWidget *window;
GtkWidget *dict_tw;
GtkTextBuffer *dict_tw_buffer;
GtkWidget *dictnum_lbl;

int EXIT_CODE = 0;
/*
   EXIT CODE:
   0 = close
   1 = Anki Card
*/

void
update_buffer()
{
    gtk_text_buffer_set_text(dict_tw_buffer, des[*curde].definition, -1); //necessary?
}

gboolean
close_with_code(int exit_code, GtkWidget *widget)
{
	EXIT_CODE = exit_code;
	gtk_window_close(GTK_WINDOW (widget));
	return TRUE;
}

void
add_anki(GtkWidget *widget)
{
  close_with_code(1, widget);
}

static void destroy()
{
    gtk_main_quit();
}

void
update_dictnum_info()
{
    char info_txt[10];
    snprintf(info_txt, sizeof(info_txt), "%li/%li", *curde+1, numde);
    gtk_label_set_text(GTK_LABEL (dictnum_lbl), info_txt);
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
    {
      fprintf(stderr, "Wrong usage of change_de function. Doing nothing.");
      return TRUE;
    }

    update_buffer();
    update_dictnum_info();
    return TRUE;
}

void change_de_up() { change_de('+'); }
void change_de_down() { change_de('-'); }

gboolean
key_press_on_win(GtkWidget *widget, GdkEventKey *event)
{
    /* Keyboard shortcuts */
    if (event->keyval == GDK_KEY_Escape || event->keyval == GDK_KEY_q)
	return close_with_code(0, widget);
    else if ((event->state & GDK_CONTROL_MASK) && (event->keyval == GDK_KEY_s))
	return close_with_code(1, widget);
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
set_font_size()
{
    char *css = "textview { font-size: " FONT_SIZE "; }";
    GtkCssProvider *cssProvider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(cssProvider, css, -1, NULL);
    gtk_style_context_add_provider(gtk_widget_get_style_context(dict_tw),
				   GTK_STYLE_PROVIDER(cssProvider),
				   GTK_STYLE_PROVIDER_PRIORITY_USER);
}

int
popup(struct dictentry *dictionary_entries, size_t num_dictionary_entries, size_t *de_toadd)
{
    des = dictionary_entries;
    numde = num_dictionary_entries;
    curde = de_toadd;

    gtk_init(NULL, NULL);

    /* window = gtk_window_new(GTK_WINDOW_POPUP); */
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size (GTK_WINDOW (window), WIN_WIDTH, WIN_HEIGHT);
    gtk_window_set_title(GTK_WINDOW(window), "floating");
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
    gtk_window_set_keep_above(GTK_WINDOW(window), 1);

    /* gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_MOUSE); */
    int x, y;
    getPointerPosition(&x, &y);
    gtk_window_move(GTK_WINDOW(window), x, y);

    g_signal_connect(window, "delete-event", G_CALLBACK(destroy), NULL);
    g_signal_connect(G_OBJECT(window), "key-press-event", G_CALLBACK(key_press_on_win), NULL);


    GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER (window), main_vbox);


    GtkWidget* swindow = gtk_scrolled_window_new(NULL, NULL);
    gtk_box_pack_start (GTK_BOX (main_vbox), swindow, TRUE, TRUE, 0);
    
    dict_tw = gtk_text_view_new ();
    dict_tw_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (dict_tw));


    dict_tw = gtk_text_view_new_with_buffer(dict_tw_buffer);
    gtk_container_add (GTK_CONTAINER (swindow), dict_tw);

    gtk_text_view_set_editable(GTK_TEXT_VIEW(dict_tw), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(dict_tw), GTK_WRAP_CHAR);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(dict_tw), FALSE);

    set_margins();
    set_font_size();

    update_buffer();

    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(main_vbox), hbox, 0, 0, 0);

    GtkWidget* btn_l = gtk_button_new_with_label ("<");
    GtkWidget* btn_r = gtk_button_new_with_label (">");
    GtkWidget *add_anki_btn = gtk_button_new_with_label ("+");
    g_signal_connect (btn_l, "clicked", G_CALLBACK (change_de_down), NULL);
    g_signal_connect (btn_r, "clicked", G_CALLBACK (change_de_up), NULL);
    g_signal_connect (add_anki_btn, "clicked", G_CALLBACK (add_anki), dict_tw);

    dictnum_lbl = gtk_label_new (NULL);
    update_dictnum_info();

    gtk_box_pack_start(GTK_BOX (hbox), btn_l, FALSE, FALSE, 0);
    gtk_box_set_center_widget(GTK_BOX (hbox), dictnum_lbl);
    gtk_box_pack_start(GTK_BOX (hbox), add_anki_btn, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX (hbox), btn_r, FALSE, FALSE, 0);

    gtk_widget_show_all(window);
    gtk_main();
    
    return EXIT_CODE;
}
