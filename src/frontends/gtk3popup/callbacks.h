#ifndef CALLBACKS_H
#define CALLBACKS_H

#include "dictpopup-application.h"
#include "gtk/gtk.h"

#include "ankiconnectc.h"

struct Color;

void search_massif_activated(GSimpleAction *action, GVariant *parameter, gpointer app);
void edit_lookup_activated(GSimpleAction *action, GVariant *parameter, gpointer app);
void open_settings_activated(GSimpleAction *action, GVariant *parameter, gpointer app);

void on_anki_status_clicked(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
gboolean on_add_to_anki_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data);
gboolean on_pronounce_button_press(GtkWidget *widget, GdkEventButton *event, gpointer user_data);

void add_to_anki_activated(GSimpleAction *action, GVariant *parameter, gpointer data);
void pronounce_activated(GSimpleAction *action, GVariant *parameter, gpointer data);
void next_definition_activated(GSimpleAction *action, GVariant *parameter, gpointer data);
void previous_definition_activated(GSimpleAction *action, GVariant *parameter, gpointer data);
void quit_activated(GSimpleAction *action, GVariant *parameter, gpointer data);

struct Color map_ac_status_to_color(AnkiCollectionStatus status);

void dict_lookup_async(DpApplication *app);

#endif // CALLBACKS_H
