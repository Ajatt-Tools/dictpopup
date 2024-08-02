#ifndef CALLBACKS_H
#define CALLBACKS_H

#include "gtk/gtk.h"

void on_settings_button_clicked(GtkButton *button, gpointer user_data);
void on_add_to_anki_clicked(GtkButton *button, gpointer user_data);
void on_pronounce_clicked(GtkButton *button, gpointer user_data);
void on_button_right_clicked(GtkButton *button, gpointer user_data);
void on_button_left_clicked(GtkButton *button, gpointer user_data);

#endif // CALLBACKS_H
