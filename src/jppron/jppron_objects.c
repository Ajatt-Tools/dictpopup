
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

FileInfo file_info_dup(FileInfo fi) {
    return (FileInfo){
        .origin = s8dup(fi.origin),
        .hira_reading = s8dup(fi.hira_reading),
        .pitch_number = s8dup(fi.pitch_number),
        .pitch_pattern = s8dup(fi.pitch_pattern),
    };
}

Pronfile pron_file_dup(Pronfile pronfile) {
    return (Pronfile){
        .filepath = s8dup(pronfile.filepath),
        .fileinfo = file_info_dup(pronfile.fileinfo),
    };
}

void pron_file_free(Pronfile pronfile) {
    frees8(&pronfile.filepath);
    file_info_free(&pronfile.fileinfo);
}