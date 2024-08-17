#ifndef OBJECTS_H
#define OBJECTS_H

#include "objects/dict.h"
#include "objects/freqentry.h"

typedef struct {
    void (*foreach_dictentry)(void *, dictentry);
    void *userdata_de;
    void (*foreach_freqentry)(void *, freqentry);
    void *userdata_fe;
    void (*foreach_dictname)(void *, s8);
    void *userdata_dn;
} ParserCallbacks;

#endif // OBJECTS_H
