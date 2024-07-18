cmake_minimum_required(VERSION 3.5)
project(dictpopup C)

include(GNUInstallDirs)
include(CTest)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
pkg_check_modules(NOTIFY REQUIRED libnotify)
find_package(LibZip REQUIRED)
find_package(CURL REQUIRED)

set(CMAKE_C_COMPILER gcc)
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DDEBUG)
    set(CMAKE_C_FLAGS
            "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic -Werror -Wstrict-prototypes -Wshadow \
	     -Wno-unused-parameter -Wno-sign-conversion -Wno-unused-function -Wpointer-arith \
	     -Wmissing-prototypes -Wstrict-prototypes -Wstrict-overflow -Wcast-align \
	     -O0 -ggdb")
    #-Wdouble-promotion
    set(CMAKE_C_FLAGS
            "${CMAKE_C_FLAGS} -fsanitize=address,undefined -fsanitize-undefined-trap-on-error -fstack-protector-strong")
else ()
    add_compile_options(-O3 -flto -march=native -mtune=native -g -fno-omit-frame-pointer)
endif ()

add_definitions(${GTK3_CFLAGS_OTHER} ${NOTIFY_CFLAGS_OTHER}
        ${LIBZIP_DEFINITIONS} ${CURL_DEFINITIONS})

set(JPPRON_SRCS
        src/jppron/jppron.c
        src/jppron/database.c
        src/jppron/ajt_audio_index_parser.c
        src/utils/yyjson.c
        src/objects/dict.c
        include/objects/freqentry.h)

# ##############################################################################
# dictpopup
# ##############################################################################
add_executable(dictpopup ${JPPRON_SRCS}
        src/frontends/gtk3popup.c
        src/dictpopup.c
        src/db.c
        src/ankiconnectc/ankiconnectc.c
        src/deinflector.c
        src/settings.c
        src/utils/util.c
        src/utils/utf8.c
        src/utils/str.c
        src/platformdep/audio.c
        src/platformdep/notifications.c
        src/platformdep/clipboard.c
        src/platformdep/file_operations.c
        src/platformdep/windowtitle.c
        src/objects/freqentry.c)
target_compile_definitions(dictpopup PUBLIC NOTIFICATIONS CLIPBOARD)
target_include_directories(
        dictpopup PRIVATE ${GTK3_INCLUDE_DIRS} ${NOTIFY_INCLUDE_DIRS}
        ${CURL_INCLUDE_DIRS} include/)
target_link_directories(dictpopup PRIVATE ${GTK3_LIBRARY_DIRS}
        ${NOTIFY_LIBRARY_DIRS} ${CURL_LIBRARY_DIRS})
target_link_libraries(dictpopup PRIVATE ${GTK3_LIBRARIES} ${NOTIFY_LIBRARIES}
        ${CURL_LIBRARIES} -lmecab -llmdb)

## X11 window title support
target_compile_definitions(dictpopup PUBLIC HAVEX11)
target_link_libraries(dictpopup PRIVATE -lXfixes -lX11)

# ##############################################################################
# dictpopup-create
# ##############################################################################
add_executable(dictpopup-create
        src/dictpopup_create.c
        src/utils/util.c
        src/utils/yyjson.c
        src/db.c
        src/yomichan_parser.c
        src/platformdep/file_operations.c
        src/utils/str.c
        src/objects/dict.c
        src/objects/freqentry.c)
target_include_directories(
        dictpopup-create PRIVATE ${GTK3_INCLUDE_DIRS} ${LIBZIP_INCLUDE_DIRS} include/)
target_link_directories(dictpopup-create PRIVATE ${GTK3_LIBRARY_DIRS}
        ${LIBZIP_LIBRARY_DIRS} ${CURL_LIBRARY_DIRS})
target_link_libraries(dictpopup-create PRIVATE ${GTK3_LIBRARIES} -lzip -llmdb
        ${CURL_LIBRARIES} -lmecab)

# ##############################################################################
# dictpopup-window
# ##############################################################################
pkg_check_modules(GTK4 REQUIRED gtk4)
add_executable(dictpopup-win EXCLUDE_FROM_ALL
        src/frontends/gtk4window.c
        src/dictpopup.c
        src/db.c
        src/deinflector.c
        src/ankiconnectc/ankiconnectc.c
        src/settings.c
        src/utils/util.c
        src/platformdep/windowtitle.c
        src/platformdep/file_operations.c
        src/platformdep/notifications.c)
target_compile_definitions(dictpopup-win PUBLIC NOTIFICATIONS)
target_include_directories(
        dictpopup-win PRIVATE ${GTK4_INCLUDE_DIRS} ${NOTIFY_INCLUDE_DIRS}
        ${CURL_INCLUDE_DIRS} include/)
target_link_directories(dictpopup-win PRIVATE ${GTK4_LIBRARY_DIRS}
        ${NOTIFY_LIBRARY_DIRS} ${CURL_LIBRARY_DIRS})
target_link_libraries(dictpopup-win PRIVATE ${GTK4_LIBRARIES} ${NOTIFY_LIBRARIES}
        ${CURL_LIBRARIES} -lmecab -llmdb)

# ##############################################################################
# cli
# ##############################################################################
add_executable(dictpopup-cli EXCLUDE_FROM_ALL src/frontends/cli.c
        src/dictpopup.c
        src/deinflector.c
        src/ankiconnectc/ankiconnectc.c
        src/utils/util.c
        src/db.c
        src/settings.c
        src/platformdep/file_operations.c
        src/platformdep/clipboard.c
        src/platformdep/windowtitle.c
        )
target_include_directories(
        dictpopup-cli PRIVATE ${GTK3_INCLUDE_DIRS} ${CURL_INCLUDE_DIRS} include/)
target_link_directories(dictpopup-cli PRIVATE ${GTK3_LIBRARY_DIRS})
target_link_libraries(dictpopup-cli PRIVATE ${GTK3_LIBRARIES} ${NOTIFY_LIBRARIES})

# ##############################################################################
# Tests
# ##############################################################################
if (BUILD_TESTING)
    enable_testing()
    find_package(cgreen REQUIRED)
    add_executable(
            c_tests
            tests/main.c
            tests/deinflector_tests.c
            tests/ankiconnect_tests.c
            src/deinflector.c
            src/utils/util.c
            tests/yomichan_parser_tests.c
            src/utils/yyjson.c
            tests/jppron_tests/ajt_audio_index_parser_tests.c
            tests/utils_tests/enclose_word_in_string_tests.c
            tests/dictpopup_tests.c
            src/utils/utf8.c
            tests/jppron_tests/jppron_tests.c)
    target_compile_definitions(c_tests PRIVATE UNIT_TEST CLIPBOARD)
    target_include_directories(c_tests PRIVATE ${GTK3_INCLUDE_DIRS} ${CGREEN_INCLUDE_DIRS} include/ src/ src/jppron)
    target_link_libraries(
            c_tests
            PRIVATE ${CGREEN_LIBRARIES}
            ${GTK3_LIBRARIES}
            ${NOTIFY_LIBRARIES}
            ${CURL_LIBRARIES}
            -llmdb
            -lmecab
            -lzip)
    add_test(NAME run_c_tests COMMAND $<TARGET_FILE:c_tests>)

    #    add_test(NAME run_shell_test COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/full_cycle_test.sh)
endif ()

# ##############################################################################
# Install
# ##############################################################################
install(TARGETS dictpopup dictpopup-create DESTINATION bin)
install(FILES config.ini DESTINATION share/dictpopup)
install(FILES data.mdb DESTINATION share/dictpopup)
install(
        DIRECTORY man1/
        DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
        FILES_MATCHING
        PATTERN "*.1")

# ##############################################################################
# Create DEB
# ##############################################################################
set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "butterkeks")
set(CPACK_PACKAGE_VERSION 0.2.1)
set(CPACK_DEBIAN_PACKAGE_DEPENDS
        "curl,libgtk-3-0,libzip4,mecab,libnotify4,liblmdb-dev")
set(CPACK_PACKAGE_CONTACT "butterkeks@fedora.email")
include(CPack)