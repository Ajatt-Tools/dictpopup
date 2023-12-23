#ifndef UTIL_H
#define UTIL_H

void notify(const char *message);
/* void *ecalloc(size_t nmemb, size_t size); */
void die(const char *fmt, ...);

char *extract_kanji(char *str);
char * extract_reading(char *str);

void str_repl_by_char(char *str, char *target, char repl_c);
void nuke_whitespace(char *str);
char *repl_str(const char *str, const char *from, const char *to);

#endif
