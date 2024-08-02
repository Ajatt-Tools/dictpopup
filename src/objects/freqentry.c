#include "objects/freqentry.h"
#include <utils/str.h>

freqentry freqentry_dup(freqentry fe) {
    return (freqentry){
        .word = s8dup(fe.word), .reading = s8dup(fe.reading), .frequency = fe.frequency};
}

void freqentry_free(freqentry *fe) {
    frees8(&fe->word);
    frees8(&fe->reading);
}