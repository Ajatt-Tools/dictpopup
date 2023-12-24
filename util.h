void notify(const char *message);
void die(const char *fmt, ...);

char *extract_kanji(const char *str);
char *extract_reading(const char *str);

void str_repl_by_char(char *str, char *target, char repl_c);
void nuke_whitespace(char *str);
