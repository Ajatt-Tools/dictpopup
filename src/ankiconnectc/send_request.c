/*
 * This file contains functions that are not easily tested, thus split for mocking purposes
 */
#include <curl/curl.h>
#include "utils/util.h"
#include <string.h>
#include <stdbool.h>
#include "ankiconnectc/send_request.h"

#define AC_API_URL_EVAR "ANKICONNECT_API_URL"
#define DEFAULT_AC_API_URL "http://localhost:8765"

typedef size_t (*ResponseFunc)(char *ptr, size_t len, size_t nmemb, void *userdata);

static size_t noop_write_function(char *ptr, size_t size, size_t nmemb, void *userdata) {
    (void)ptr; // Suppress unused parameter warning
    (void)userdata;
    return size * nmemb;
}

static const char *get_api_url(void) {
    const char *env = getenv(AC_API_URL_EVAR); // Warning: String can change
    return env && *env ? env : DEFAULT_AC_API_URL;
}

static retval_s sendRequest(s8 request, ResponseFunc response_checker) {
    CURL *curl = curl_easy_init();
    if (!curl)
        return (retval_s){.data.string = strdup("Error initializing cURL"), .ok = false};

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, get_api_url());
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, request.len);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request.s);

    retval_s ret = {.ok = true}; // Maybe add a generic checker instead of
    // defaulting to true
    if (response_checker) {
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, response_checker);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ret);
    } else {
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, noop_write_function);
    }

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        ret =
            (retval_s){.data.string = strdup("Could not connect to AnkiConnect. Is Anki running?"),
                       .ok = false};
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return ret;
}