#ifndef CLIPBOARD_H
#define CLIPBOARD_H

#include <utils/str.h>

s8 get_selection(void);
char *get_clipboard(void);
char *get_next_clipboard(void);

#endif // CLIPBOARD_H
