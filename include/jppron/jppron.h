#ifndef JPPRON_H
#define JPPRON_H

#include "jppron/ajt_audio_index_parser.h"
#include "utils/util.h"

typedef struct {
    s8 filepath;
    fileinfo_s fileinfo;
} pronfile_s;

void _nonnull_ free_pronfile(pronfile_s *pronfile);
void _nonnull_ free_pronfile_buffer(pronfile_s **pronfiles);
DEFINE_DROP_FUNC_PTR(pronfile_s, free_pronfile)
DEFINE_DROP_FUNC_PTR(pronfile_s *, free_pronfile_buffer)

void jppron(s8 word, s8 reading, char *audio_folders_path);
/* _deallocator_(free_pronfile_buffer) */ // TODO: Check why this gives compiler errors
pronfile_s *get_pronfiles_for(s8 word, s8 reading, s8 db_path);

#endif // JPPRON_H