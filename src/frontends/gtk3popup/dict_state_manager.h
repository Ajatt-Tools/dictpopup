#ifndef DICT_STATE_MANAGER_H
#define DICT_STATE_MANAGER_H
#include <objects/dict.h>

typedef struct {
    Dict dictionary;
    size_t index_visible;
} DictManager;

void dict_manager_init(DictManager *self);
dictentry dm_get_currently_visible(const DictManager self);
bool dm_increment(DictManager self[static 1]);
bool dm_decrement(DictManager self[static 1]);
void dm_swap_dict(DictManager self[static 1], Dict new_dict);

/* getter */
size_t dm_get_current_index(const DictManager self);
size_t dm_get_total_num_of_entries(const DictManager self);
const char *dm_get_current_dictname(const DictManager self);

#endif // DICT_STATE_MANAGER_H
