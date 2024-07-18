#ifndef FILE_OPERATIONS_H
#define FILE_OPERATIONS_H

#include "utils/util.h"
#include <stdbool.h>

void _nonnull_ createdir(char *dirpath);
const char *get_user_data_dir(void);
bool check_file_exists(const char *fn);
void rem(char *filepath);
void _nonnull_ file_copy_sync(const char *source, const char *dest);
void _nonnull_ file_copy_async(const char *source, const char *dest);

#endif // FILE_OPERATIONS_H
