void notify(char const *fmt, ...);

char **extract_kanji_array(const char *str);
char *extract_kanji(const char *str);
char *extract_reading(const char *str);

void str_repl_by_char(char *str, char *target, char repl_c);
void nuke_whitespace(char *str);

char* read_cmd_sync(char const *fmt, ...);
int spawn_cmd_async(char const *fmt, ...);
