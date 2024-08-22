#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include "dp_page_manager.h"
#include "jppron/jppron_objects.h"
#include "objects/dict.h"

#include <gtk/gtk.h>
#include <objects/color.h>

typedef struct {
    Color current_anki_status_color;

    GtkWindow *main_window;
    GtkTextBuffer *definition_textbuffer;
    GtkLabel *entry_number_label;
    GtkLabel *dictname_label;
    GtkLabel *frequency_label;
    GtkLabel *current_word_label;
    GtkWidget *anki_status_dot;
    GtkWidget *pronounce_button;
    GtkWidget *btn_previous;
    GtkWidget *btn_next;
    GtkWidget *btn_pronounce;
    GtkWidget *btn_add_to_anki;
} UiManager;

void ui_manager_set_application(UiManager *self, GtkApplication *app);

void _nonnull_ ui_manager_show_window(UiManager *self);
void _nonnull_ ui_manager_hide_window(UiManager *self);

s8 _nonnull_ ui_manager_get_text_selection(UiManager *self);

void _nonnull_ ui_manager_init(UiManager *self, GtkBuilder *builder);
void _nonnull_ ui_refresh(UiManager *self, PageManager *pm);
void _nonnull_ ui_manager_set_error(UiManager *self, s8 message);

void show_pronunciation_button_right_click_menu(UiManager *self, Pronfile *pronfiles);

void ui_manager_show_anki_button_right_click_menu(UiManager *self,
                                                  void (*on_clipboard_definition)(void *user_data),
                                                  void *user_data);

void ui_manager_show_edit_lookup_dialog(UiManager *self, const char *current_lookup,
                                        void (*on_accept)(const char *new_lookup, void *user_data),
                                        void *user_data);

#endif //UI_MANAGER_H
