#ifndef DICT_STATE_MANAGER_H
#define DICT_STATE_MANAGER_H
#include "dp-settings.h"

#include <ankiconnectc.h>
#include <gtk/gtk.h>
#include <jppron/jppron_objects.h>
#include <objects/dict.h>

typedef struct {
    Dictentry entry;

    Pronfile *pronfiles;
    bool pronfiles_loaded;

    AnkiCollectionStatus anki_exists_status;
    bool anki_status_loaded;
} PageData;

typedef struct {
    GMutex mutex;

    PageData *pages;
    size_t index_visible;
    size_t num_pages;

    DpSettings *settings;
} PageManager;

void _nonnull_ page_manager_init(PageManager *self, DpSettings *settings);

bool _nonnull_ pm_increment(PageManager *self);
bool _nonnull_ pm_decrement(PageManager *self);
void _nonnull_ pm_swap_dict(PageManager *self, Dict new_dict);

s8 _nonnull_ pm_get_path_of_current_pronunciation(PageManager *self);
Pronfile *_nonnull_ pm_get_current_pronfiles(PageManager *self);
Word _nonnull_ pm_get_current_word(PageManager *self);
Dictentry _nonnull_ pm_get_current_dictentry(PageManager *self);



void pm_lock(PageManager *self);
Dictentry *pm_get_current_entry_ref(PageManager *self);
size_t pm_get_current_index(PageManager *self);
size_t pm_get_num_pages(PageManager *self);
AnkiCollectionStatus _nonnull_ pm_get_current_anki_status_nolock(PageManager *self);
Pronfile *pm_get_current_pronfiles_ref(PageManager *self);
void pm_unlock(PageManager *self);

#endif // DICT_STATE_MANAGER_H
