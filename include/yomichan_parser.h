#ifndef YOMICHAN_PARSER_H
#define YOMICHAN_PARSER_H

#include <dictpopup_create/objects.h>
#include <stdatomic.h>
#include <stdbool.h>

void parse_yomichan_dict(const char *zipfn, ParserCallbacks callbacks, atomic_bool *cancel_flag);

#endif // YOMICHAN_PARSER_H
