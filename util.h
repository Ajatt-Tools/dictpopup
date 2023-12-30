#include <stdbool.h>

#define Stopif(assertion, error_action, ...)          \
	if (assertion) {                              \
		notify(1, __VA_ARGS__);	  	      \
		fprintf(stderr, __VA_ARGS__);         \
		fprintf(stderr, "\n");                \
		error_action;                         \
	}

void notify(bool error, char const *fmt, ...);

char **extract_kanji_array(const char *str);
char *extract_kanji(const char *str);
char *extract_reading(const char *str);

void str_repl_by_char(char *str, char *target, char repl_c);
void nuke_whitespace(char *str);

/* 
 * Reads the standard output of a command and waits for it to finish.
 * Argv is the null terminated list of arguments, argv[0] being the
 * full path to the executable.
 */
char* read_cmd_sync(char **argv);
int spawn_cmd_async(char const *fmt, ...);
