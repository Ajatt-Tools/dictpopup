
#include <jppron/jppron_objects.h>
#include <utils/str.h>

// static void print_fileinfo(fileinfo_s fi) {
//     printf("Source: %.*s\nReading: %.*s\nPitch number: %.*s\nPitch pattern: "
//            "%.*s\n\n",
//            (int)fi.origin.len, (char *)fi.origin.s, (int)fi.hira_reading.len,
//            (char *)fi.hira_reading.s, (int)fi.pitch_number.len, (char *)fi.pitch_number.s,
//            (int)fi.pitch_pattern.len, (char *)fi.pitch_pattern.s);
// }

void file_info_free(FileInfo *fi) {
    frees8(&fi->origin);
    frees8(&fi->hira_reading);
    frees8(&fi->pitch_number);
    frees8(&fi->pitch_pattern);
}