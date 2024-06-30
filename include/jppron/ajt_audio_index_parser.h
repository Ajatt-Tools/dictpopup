#ifndef AUDIO_INDEX_PARSER_H
#define AUDIO_INDEX_PARSER_H

#include <stddef.h>
#include "utils/util.h"

typedef struct {
    s8 origin;
    s8 hira_reading;
    s8 pitch_number;
    s8 pitch_pattern;
} fileinfo_s;

void parse_audio_index_from_file(s8 curdir, const char *index_filepath,
                                 void (*foreach_headword)(void *, s8, s8), void *userdata_hw,
                                 void (*foreach_file)(void *, s8, fileinfo_s), void *userdata_f);

#endif // AUDIO_INDEX_PARSER_H