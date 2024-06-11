#ifndef PLATFORMDEP_H
#define PLATFORMDEP_H

#include "util.h"
#include <stdbool.h>

void notify(_Bool urgent, char const *fmt, ...);
s8 get_selection(void);
s8 get_clipboard(void);
s8 get_next_clipboard(void);
s8 get_windowname(void);
void free_windowname(s8 windowname);
void play_audio(s8 filepath);
void _nonnull_ createdir(char *dirpath);
const char *get_user_data_dir(void);
bool check_file_exists(const char *fn);

#endif // PLATFORMDEP_H