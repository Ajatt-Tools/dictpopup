//
// Created by daniel on 24/06/26.
//

#ifndef FILE_PATHS_H
#define FILE_PATHS_H

#ifdef _WIN32
    // TODO: fill
    #define DEFAULT_DATABASE_LOCATIONS                                                             \
        (char *[]) {                                                                               \
            ""                                                                                     \
        }
    #define DEFAULT_SETTINGS_LOCATIONS                                                             \
        (char *[]) {                                                                               \
            "", ""                                                                                 \
        }
#else
    #define DEFAULT_DATABASE_LOCATIONS                                                             \
        (char *[]) {                                                                               \
            "/usr/share/dictpopup/data.mdb", "/usr/local/share/dictpopup/data.mdb"                 \
        }
    #define DEFAULT_SETTINGS_LOCATIONS                                                             \
        (char *[]) {                                                                               \
            "/usr/share/dictpopup/config.ini", "/usr/local/share/dictpopup/config.ini"             \
        }
#endif

#endif // FILE_PATHS_H
