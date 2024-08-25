#ifndef JPPRON_OBJECTS_H
#define JPPRON_OBJECTS_H
#include <utils/str.h>

typedef struct {
    s8 origin;
    s8 hira_reading;
    s8 pitch_number;
    s8 pitch_pattern;
} FileInfo;

FileInfo file_info_dup(FileInfo fi);

void file_info_free(FileInfo *fi);
DEFINE_DROP_FUNC_PTR(FileInfo, file_info_free)

typedef struct {
    s8 filepath;
    FileInfo fileinfo;
} Pronfile;

Pronfile pron_file_dup(Pronfile pronfile);
void pron_file_free(Pronfile pronfile);

#endif //JPPRON_OBJECTS_H
