#ifndef JPPRON_H
#define JPPRON_H

#include "jppron/ajt_audio_index_parser.h"
#include "utils/util.h"
#include "jppron/jppron_objects.h"
#include "objects/dict.h"
#include <stdbool.h>
#include <stdatomic.h>

void free_pronfile(Pronfile pronfile[static 1]);
void free_pronfile_buffer(Pronfile *pronfiles);
DEFINE_DROP_FUNC_PTR(Pronfile, free_pronfile)
DEFINE_DROP_FUNC(Pronfile *, free_pronfile_buffer)

_deallocator_(free_pronfile_buffer) // TODO: Check why this gives compiler errors
Pronfile *get_pronfiles_for(Word word);

void jppron_create_index(const char *audio_folders_path, atomic_bool *cancel_flag);

#endif // JPPRON_H