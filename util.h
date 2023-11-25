#ifndef UTIL_H
#define UTIL_H

void notify(const char *message);
void *ecalloc(size_t nmemb, size_t size);
void die(const char *fmt, ...);
char *repl_str(const char *str, const char *from, const char *to);

#endif
