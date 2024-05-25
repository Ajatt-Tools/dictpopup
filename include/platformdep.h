#ifndef PLATFORMDEP_H
#define PLATFORMDEP_H

#include "util.h"

void _nonnull_n_(1) notify(const char *title, _Bool urgent, char const *fmt, ...);
s8 get_selection(void);
s8 get_sentence(void);
s8 get_windowname(void);
void free_windowname(s8 windowname);
void play_audio(s8 filepath);
void _nonnull_ createdir(char *dirpath);
const char *get_user_data_dir(void);

#endif // PLATFORMDEP_H