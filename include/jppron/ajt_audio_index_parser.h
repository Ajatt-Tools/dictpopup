#ifndef AUDIO_INDEX_PARSER_H
#define AUDIO_INDEX_PARSER_H

#include "utils/str.h"
#include "jppron_objects.h"

void parse_audio_index_from_file(s8 curdir, const char *index_filepath,
                                 void (*foreach_headword)(void *, s8, s8), void *userdata_hw,
                                 void (*foreach_file)(void *, s8, FileInfo), void *userdata_f);

#endif // AUDIO_INDEX_PARSER_H
