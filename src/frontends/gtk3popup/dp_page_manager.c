#include "callbacks.h"
#include "dictpopup-application.h"

#include <jppron/jppron.h>
#include <objects/dict.h>
#include <utils/messages.h>

static PageData *_nonnull_ dict_pages_from_dictentries(Dict dict, size_t *num_pages) {
    size_t num = num_entries(dict);
    if (num == 0)
        return NULL;

    PageData *pages = calloc(num, sizeof(PageData));
    for (size_t i = 0; i < num; i++) {
        pages[i] = (PageData){.entry = dictentry_at_index(dict, i),
                              .pronfiles = NULL,
                              .pronfiles_loaded = false,
                              .anki_exists_status = AC_ERROR,
                              .anki_status_loaded = false};
    }

    dict_free(dict, false);

    *num_pages = num;
    return pages;
}

static void dict_pages_free(PageData *pages) {
    // TODO
    free(pages);
}

void _nonnull_ page_manager_init(PageManager *self, DpSettings *settings) {
    self->pages = NULL;
    self->index_visible = 0;
    self->num_pages = 0;
    self->settings = settings;
}

static PageData *_nonnull_ get_current_data(PageManager *self) {
    assert(self->pages);
    return &self->pages[self->index_visible];
}

static AnkiCollectionStatus get_anki_exists_with_current_settings(Word word, DpSettings *settings) {
    char *deck = dp_settings_get_anki_deck(settings);
    char *search_field = dp_settings_get_anki_search_field(settings);

    return ac_check_exists(deck, search_field, (char *)word.kanji.s, 0);
}

static void pm_load_anki_status(PageManager *self) {
    PageData *current_data = get_current_data(self);

    Word word = dictentry_get_word(current_data->entry);
    current_data->anki_exists_status = get_anki_exists_with_current_settings(word, self->settings);
    current_data->anki_status_loaded = true;
}

static void pm_load_pronfiles(PageManager *self) {
    PageData *current_data = get_current_data(self);

    Word word = dictentry_get_word(current_data->entry);
    current_data->pronfiles = get_pronfiles_for(word);
    current_data->pronfiles_loaded = true;
}

static void _nonnull_ set_index_visible(PageManager *self, size_t new_index) {
    assert(new_index < self->num_pages);
    self->index_visible = new_index;
}

bool _nonnull_ pm_increment(PageManager *self) {
    size_t new_index = (self->index_visible + 1 < self->num_pages) ? self->index_visible + 1 : 0;

    if (new_index != self->index_visible) {
        set_index_visible(self, new_index);
        return true;
    }

    return false;
}

bool _nonnull_ pm_decrement(PageManager *self) {
    size_t new_index = (self->index_visible > 0) ? self->index_visible - 1
                       : self->num_pages > 0     ? self->num_pages - 1
                                                 : 0;

    if (new_index != self->index_visible) {
        set_index_visible(self, new_index);
        return true;
    }
    return false;
}

void _nonnull_ pm_swap_dict(PageManager *self, Dict new_dict) {
    PageData *new_page_data = dict_pages_from_dictentries(new_dict, &self->num_pages);

    dict_pages_free(self->pages);
    self->pages = new_page_data;
    set_index_visible(self, 0);
}

Pronfile *_nonnull_ pm_get_current_pronfiles(PageManager *self) {
    PageData *data = get_current_data(self);

    if (!data->pronfiles_loaded)
        pm_load_pronfiles(self);

    return data->pronfiles;
}

s8 _nonnull_ pm_get_path_of_current_pronunciation(PageManager *self) {
    s8 ret = {0};

    Pronfile *pronfiles = pm_get_current_pronfiles(self);
    if (pronfiles)
        ret = pronfiles[0].filepath;

    return ret;
}

Word _nonnull_ pm_get_current_word(PageManager *self) {
    PageData *data = get_current_data(self);
    return dictentry_get_word(data->entry);
}

Dictentry _nonnull_ pm_get_current_dictentry(PageManager *self) {
    PageData *data = get_current_data(self);
    return data->entry;
}

size_t pm_get_current_index(PageManager *self) {
    return self->index_visible;
}

size_t pm_get_num_pages(PageManager *self) {
    return self->num_pages;
}

AnkiCollectionStatus _nonnull_ pm_get_current_anki_status(PageManager *self) {
    PageData *data = get_current_data(self);

    if (!data->anki_status_loaded)
        pm_load_anki_status(self);

    return data->anki_exists_status;
}
