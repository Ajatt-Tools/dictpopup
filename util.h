#include <stdbool.h>

#define Stopif(assertion, error_action, ...)          \
	if (assertion) {                              \
		notify(1, __VA_ARGS__);	  	      \
		fprintf(stderr, __VA_ARGS__);         \
		fprintf(stderr, "\n");                \
		error_action;                         \
	}

/*
 * Sends a notification. @error=1 sets to urgency level to urgent.
 */
void notify(bool error, char const *fmt, ...);

char* sselp();

void remove_last_unichar(char *str, int *len);

/*
 * Returns an array of kanji readings
 * Currently supported format:
 *
 *  * かむ【嚙む・嚼む・咬む】
 *
 * The returned string should be freed with g_strfreev()
 */
char **extract_kanji_array(const char *str);
/*
 * Simply returns the kanji part of the string, which
 * is expected to be enclosed in 【】
 *
 * The returned string should be freed.
 */
char *extract_kanji(const char *str);
char *extract_reading(const char *str);

char* str_repl_by_char(char *str, char *target, char repl_c);
void nuke_whitespace(char *str);

/*
 * Calls a command asynchronously in printf style.
 *
 * Returns: TRUE on success and FALSE if there was an error
 */
int printf_cmd_async(char const *fmt, ...);
int printf_cmd_sync(char const *fmt, ...);
