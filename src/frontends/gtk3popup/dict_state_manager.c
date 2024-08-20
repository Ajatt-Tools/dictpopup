#include "dict_state_manager.h"
#include <objects/dict.h>

void dict_manager_init(DictManager *self) {
    *self = (DictManager){0};
}

dictentry dm_get_currently_visible(const DictManager self) {
    return dictentry_at_index(self.dictionary, self.index_visible);
}

bool dm_increment(DictManager self[static 1]) {
    size_t new_index =
        (self->index_visible + 1 < num_entries(self->dictionary)) ? self->index_visible + 1 : 0;

    if (new_index != self->index_visible) {
        self->index_visible = new_index;
        return true;
    } else
        return false;
}

bool dm_decrement(DictManager self[static 1]) {
    size_t new_index = (self->index_visible > 0)           ? self->index_visible - 1
                      : num_entries(self->dictionary) > 0 ? num_entries(self->dictionary) - 1
                                                          : 0;

    if (new_index != self->index_visible) {
        self->index_visible = new_index;
        return true;
    } else
        return false;
}

void dm_swap_dict(DictManager self[static 1], Dict new_dict) {
    dict_free(self->dictionary);
    self->dictionary = new_dict;
    self->index_visible = 0;
}

size_t dm_get_current_index(const DictManager self) {
    return self.index_visible;
}

size_t dm_get_total_num_of_entries(const DictManager self) {
    return num_entries(self.dictionary);
}

const char *dm_get_current_dictname(const DictManager self) {
    return (const char *)dm_get_currently_visible(self).dictname.s;
}