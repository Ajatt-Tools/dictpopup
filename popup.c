#include <gtk/gtk.h>

#define MARGIN 5
#define WIN_WIDTH 480
#define WIN_HEIGHT 300

GtkCssProvider *provider;

// FIXME: Non global?
int cur_de = 0;
int num_de = 0;
char **de; 

int EXIT_CODE = 0;
/*
   EXIT CODE:
   0 = close
   1 = Anki Card
*/

gboolean
close_with_code(int exit_code, GtkWidget *widget)
{
	EXIT_CODE = exit_code;
	gtk_window_close(GTK_WINDOW (widget));
	return TRUE;
}

static void destroy()
{
    gtk_main_quit();
}

gboolean
updateBuffer(gpointer textview)
{
	GtkTextBuffer *textbuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
        gtk_text_buffer_set_text(textbuffer, de[cur_de], -1);
	return TRUE;
}

gboolean key_press_on_win(GtkWidget *widget, GdkEventKey *event, gpointer textview)
{
    if (event->keyval == GDK_KEY_Escape || event->keyval == GDK_KEY_q) 
	return close_with_code(0, widget);
    else if ((event->state & GDK_CONTROL_MASK) && (event->keyval == GDK_KEY_s))
	return close_with_code(1, widget);
    else if (event->keyval == GDK_KEY_p) 
    {
	cur_de = (cur_de != 0) ? cur_de-1 : num_de-1;
	return updateBuffer(textview);
    }
    else if (event->keyval == GDK_KEY_n) 
    {
	cur_de = (cur_de != num_de - 1) ? cur_de+1 : 0;
	return updateBuffer(textview);
    }

    return FALSE;
}

int popup(char **dictionary_entries, int num_dictionary_entries, int x, int y)
{    
    de = dictionary_entries;
    num_de = num_dictionary_entries;

    gtk_init(NULL, NULL);

    /* GtkWidget *window = gtk_window_new(GTK_WINDOW_POPUP); */
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_default_size (GTK_WINDOW (window), WIN_WIDTH, WIN_HEIGHT);
    gtk_window_set_title(GTK_WINDOW(window), "floating");
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
    gtk_window_set_keep_above(GTK_WINDOW(window), 1);

    g_signal_connect(window, "delete-event", G_CALLBACK(destroy), NULL);

    GtkWidget* swindow = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(window), swindow);
    
    GtkTextBuffer *textbuffer = gtk_text_buffer_new(NULL);
    gtk_text_buffer_set_text(textbuffer, de[cur_de], -1);
    GtkWidget *textview = gtk_text_view_new_with_buffer(textbuffer);
    gtk_container_add (GTK_CONTAINER (swindow), textview);

    gtk_text_view_set_editable(GTK_TEXT_VIEW(textview), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(textview), GTK_WRAP_CHAR);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(textview), FALSE);

    gtk_text_view_set_top_margin(GTK_TEXT_VIEW(textview), MARGIN);
    gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(textview), MARGIN);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(textview), MARGIN);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(textview), MARGIN);

    /* CSS */
    GtkCssProvider *cssProvider = gtk_css_provider_new();
    if( gtk_css_provider_load_from_path(cssProvider, "/usr/share/dictpopup/textview.css", NULL) ) 
    {
     gtk_style_context_add_provider(gtk_widget_get_style_context(textview),
                                        GTK_STYLE_PROVIDER(cssProvider),
                                        GTK_STYLE_PROVIDER_PRIORITY_USER);
    }

    g_signal_connect(G_OBJECT(window), "key-press-event", G_CALLBACK(key_press_on_win), textview);
    /* gdk_window_get_pointer(gdk_get_default_root_window(), &x, &y, 0); */
    gtk_window_move(GTK_WINDOW(window), x, y);
    gtk_widget_show_all(window);

    gtk_main();
    
    return EXIT_CODE;
}
