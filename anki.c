#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <stdbool.h>

#include <X11/Xlib.h> // for XFree

#include "xlib.h"
#include "util.h"
#include "config.h"
#include "structs.h"

static void nuke(char *str);
static char * boldWord(char *sent, char *word);
static void prepare_fields(char **bdsent, char **vocabKanji, char **vocabFurigana, char **de, char *lu_word, char *dict_word);
static void sendRequest(char *request);


void
nuke(char *str)
{
    if (!NUKE_SPACES && !NUKE_NEWLINES)
      return;

    int len = strlen(str);
    int skip = 0;
    for (int i = 0; i < len; i++)
    {
	if(NUKE_SPACES && (str[i] == ' ' || strcmp(str, "　") == 0))
	  skip++;
	else if (NUKE_NEWLINES && str[i] == '\n')
	  skip++;
	else
	  str[i-skip] = str[i];
    }
    str[len-skip] = '\0';
}

char *
boldWord(char *sent, char *word)
{
    char *bdword, *bdsent;
    if(asprintf(&bdword, "<b>%s</b>", word) == -1)
      die("Could not allocate memory for bold word."); // Not dying would be better
    bdsent = repl_str(sent, word, bdword);
    free(bdword);

    return bdsent;
}

void
prepare_fields(char **bdsent, char **vocabKanji, char **vocabFurigana,
    char **def, char *lu_word, char *dict_word)
{
    /* Prepare sentence */
    notify("Please select the sentence.");
    clipnotify();
    char *sent = sselp();

    nuke(sent);
    *bdsent = boldWord(sent, lu_word);
    XFree(sent);

    // Prepare dictionary entry
    //TODO: Replace " with &quot
    *def = repl_str(*def, "\n", "<br>"); // Freeing old de?
    
    //Prepare Kanji + Reading
    char *end_read = strstr(dict_word, "【");
    char *end_kanj = strstr(dict_word, "】");
    if (end_read != NULL && end_kanj != NULL)
    {
	*end_read = '\0';
	*end_kanj = '\0';
	*vocabKanji = end_read + strlen("【");
	// FIXME: Won't work as expected for 送り仮名 like 取り組む
	asprintf(vocabFurigana, "%s[%s]", *vocabKanji, dict_word);
    }
    else /* Either different format or no kanji */
    {
      *vocabKanji = dict_word;
      *vocabFurigana = dict_word;
    }
}

// TODO: Show if request was successfull or not
void
sendRequest(char *request)
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
    res = curl_easy_perform(curl);

    if (res != CURLE_OK)
        die("Error with curl request: %s.", curl_easy_strerror(res));

    curl_easy_cleanup(curl);
}

void
addNote(char *lu_word, dictentry de, char *def, char *wname)
{
    // TODO: Create a "anki card" struct?
    char *bdsent, *vocabKanji, *vocabFurigana;
    prepare_fields(&bdsent, &vocabKanji, &vocabFurigana, &def, lu_word, de.word);


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
	ANKI_SENTENCE_FIELD,
	bdsent,
        ANKI_KANJI_FIELD,
        vocabKanji,
	ANKI_FURIGANA_FIELD,
	vocabFurigana,
        ANKI_DEFINITION_FIELD,
        def,
	ANKI_NOTES_FIELD,
	wname
    );

    sendRequest(request);

    free(bdsent);
    free(vocabFurigana);
    free(request);
}
