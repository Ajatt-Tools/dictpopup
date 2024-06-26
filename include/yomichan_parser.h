#ifndef YOMICHAN_PARSER_H
#define YOMICHAN_PARSER_H
#include <utils/util.h>

void parse_yomichan_dict(const char *zipfn, void (*foreach_dictentry)(void *, dictentry),
                         void *userdata_de, void (*forach_freqentry)(void *, freqentry),
                         void *userdata_fe);

#endif // YOMICHAN_PARSER_H
