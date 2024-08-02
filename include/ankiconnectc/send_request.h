#ifndef SEND_REQUEST_H
#define SEND_REQUEST_H

#include <stddef.h>
#include <utils/str.h>

#define AC_API_URL_EVAR "ANKICONNECT_API_URL"
#define DEFAULT_AC_API_URL "http://localhost:8765"

typedef size_t (*ResponseFunc)(char *ptr, size_t len, size_t nmemb, void *userdata);

typedef struct {
    union {
        char *string;
        _Bool boolean;
    } data;
    _Bool ok; // Signalizes if there was an error on not. The error msg is
    // stored in data.string
} retval_s;

retval_s sendRequest(s8 request, ResponseFunc response_checker);

#endif // SEND_REQUEST_H
