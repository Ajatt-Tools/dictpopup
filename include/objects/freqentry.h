#ifndef FREQENTRY_H
#define FREQENTRY_H
#include <utils/str.h>

typedef struct {
    s8 word;
    s8 reading;
    u32 frequency;
} freqentry;

freqentry freqentry_dup(freqentry fe);
void freqentry_free(freqentry *fe);

#endif // FREQENTRY_H
