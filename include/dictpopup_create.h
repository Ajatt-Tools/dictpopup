#ifndef DICTPOPUP_CREATE_H
#define DICTPOPUP_CREATE_H

#include <stdbool.h>
#include <stdatomic.h>

void _nonnull_ dictpopup_create(const char *dictionaries_path,
                                bool (*should_overwrite_existing_db)(void *user_data), void *user_data, atomic_bool *cancel_flag);
#endif // DICTPOPUP_CREATE_H
