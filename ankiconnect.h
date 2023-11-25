#ifndef ANKICONNECT_H
#define ANKICONNECT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <stdbool.h>

#include "util.h"

typedef size_t (*ResponseFunction)(char *ptr, size_t size, size_t nmemb, void *userdata);


static size_t check_add_response(char *ptr, size_t size, size_t nmemb, void *userdata);
static void sendRequest(char *request, ResponseFunction respfunc);
static void search_query(const char *search_field, const char *query, ResponseFunction respf);
static char *create_request(char *pe[]);
static void addNote(char *pe[]);

size_t
check_add_response(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    char *errorstr = strstr(ptr, "\"error\": ");
    if (errorstr == NULL)
	die("Bad curl response.\n");
    errorstr = errorstr + strlen("\"error\": ");
    if (strncmp(errorstr, "null", strlen("null")) != 0)
	die("AnkiConnect returned the following error: %.*s", (int)nmemb, ptr);
    else
	notify("Request successfull");

    return nmemb;
}

void
sendRequest(char *request, ResponseFunction respfunc)
{
    CURL *curl;
    CURLcode res;
    curl_global_init(CURL_GLOBAL_ALL);
    if (!(curl = curl_easy_init()))
      die("Error initializing cURL.");

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Accept: application/json");
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, ANKI_API_URL);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, respfunc);

    res = curl_easy_perform(curl);
    if (res != CURLE_OK)
        die("Error with curl request: %s.", curl_easy_strerror(res));

    curl_easy_cleanup(curl);
}

void
search_query(const char *search_field, const char *query, ResponseFunction respf)
{
  /* Looks for  */
  char *request;
  asprintf(&request, "{ \"action\": \"findCards\", \"version\": 6, \"params\": { \"query\": \"%s:%s\" } }", search_field, query);
  sendRequest(request, respf);
  free(request);
}

char *
create_request(char *pe[])
{
    // TODO: JSON escape

    char *request;
    asprintf(&request,
        "{"
        "\"action\": \"addNote\","
        "\"version\": 6,"
        "\"params\": {"
            "\"note\": {"
                "\"deckName\": \"%s\","
                "\"modelName\": \"%s\","
                "\"fields\": {"
                    "\"%s\": \"%s\","
                    "\"%s\": \"%s\","
                    "\"%s\": \"%s\","
                    "\"%s\": \"%s\","
                    "\"%s\": \"%s\""
                "},"
                "\"options\": {"
                    "\"allowDuplicate\": true"
                "},"
                "\"tags\": []"
            "}"
        "}"
        "}",
        ANKI_DECK,
        ANKI_MODEL,
	fieldnames[0],
	pe[fieldentry[0]],
	fieldnames[1],
	pe[fieldentry[1]],
	fieldnames[2],
	pe[fieldentry[2]],
	fieldnames[3],
	pe[fieldentry[3]],
	fieldnames[4],
	pe[fieldentry[4]]
    );

    return request;
}

void
addNote(char *pe[])
{
    char *request = create_request(pe);
    sendRequest(request, check_add_response);
    free(request);
}

#endif //ANKICONNECT_H
