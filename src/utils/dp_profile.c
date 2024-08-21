#include <stdio.h>
#include <time.h>

#include "utils/dp_profile.h"

#include <utils/messages.h>

clock_t start_time;

void set_start_time_now(void) {
    start_time = clock();
}

void set_end_time_now(void) {
    clock_t end_time = clock();
    dbg("Startup time: %fs\n", (double)(end_time - start_time) / CLOCKS_PER_SEC);
}