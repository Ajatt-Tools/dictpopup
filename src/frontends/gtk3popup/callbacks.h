#ifndef CALLBACKS_H
#define CALLBACKS_H

#include "gtk/gtk.h"

#include <ankiconnectc.h>

struct Color;

void on_settings_button_clicked(GtkButton *button, gpointer user_data);
void on_anki_status_clicked(GtkWidget *widget, GdkEventButton *event, gpointer user_data);

void add_to_anki_activated(GSimpleAction *action, GVariant *parameter, gpointer data);
void pronounce_activated(GSimpleAction *action, GVariant *parameter, gpointer data);
void next_definition_activated(GSimpleAction *action, GVariant *parameter, gpointer data);
void previous_definition_activated(GSimpleAction *action, GVariant *parameter, gpointer data);
void quit_activated(GSimpleAction *action, GVariant *parameter, gpointer data);

void pronounce_current_word(DpApplication *app);
struct Color map_ac_status_to_color(AnkiCollectionStatus status);

void dict_lookup_async(DpApplication *app);

typedef struct Color {
    double red;
    double green;
    double blue;
} Color;

#define ANKI_RED                                                                                   \
    (Color) {                                                                                      \
        .red = 0.9490, .green = 0.4431, .blue = 0.4431                                             \
    }
#define ANKI_GREEN                                                                                 \
    (Color) {                                                                                      \
        .red = 0.1333, .green = 0.7725, .blue = 0.3686                                             \
    }
#define ANKI_BLUE                                                                                  \
    (Color) {                                                                                      \
        .red = 0.5765, .green = 0.7725, .blue = 0.9922                                             \
    }
#define ORANGE                                                                                     \
    (Color) {                                                                                      \
        .red = 1, .green = 0.5, .blue = 0                                                          \
    }
#define GREY                                                                                       \
    (Color) {                                                                                      \
        .red = 0.5, .green = 0.5, .blue = 0.5                                                      \
    }

#endif // CALLBACKS_H
