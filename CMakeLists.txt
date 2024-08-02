cmake_minimum_required(VERSION 3.5)
project(dictpopup C)

include(GNUInstallDirs)
include(CTest)

find_package(PkgConfig REQUIRED)
# ##############################################################################
# Compile Dependencies
# ##############################################################################
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
pkg_check_modules(NOTIFY REQUIRED libnotify)
find_package(LibZip REQUIRED)
find_package(CURL REQUIRED)

find_program(GLIB_COMPILE_RESOURCES NAMES glib-compile-resources)
if (NOT GLIB_COMPILE_RESOURCES)
    message(FATAL_ERROR "glib-compile-resources binary not found")
endif ()

find_program(GPERF NAMES gperf)
if (NOT GPERF)
    message(FATAL_ERROR "gperf binary not found")
endif ()

# Optional dependencies
find_package(X11)
if (X11_FOUND)
    add_definitions(-DHAVE_X11)
    set(X11_LIBRARIES ${X11_LIBRARIES} ${X11_Xfixes_LIB})
else ()
    set(X11_LIBRARIES "")
    message(STATUS "X11 not found, building without X11 support")
endif ()

# ##############################################################################
# Dev Dependencies
# ##############################################################################
find_program(CLANG_FORMAT "clang-format")
find_program(CPPCHECK cppcheck)
#if (NOT CLANG_FORMAT)
#    message(STATUS "clang-format not found, formatting target will not be available")
#endif ()

# ##############################################################################
# General compilation
# ##############################################################################
set(CMAKE_C_COMPILER gcc)
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DDEBUG)
    set(CMAKE_C_FLAGS
            "${CMAKE_C_FLAGS} -Wall -Wextra -Werror -Wstrict-prototypes -Wshadow -Wdouble-promotion \
	     -Wno-unused-parameter -Wno-sign-conversion -Wno-unused-function -Wpointer-arith \
	     -Wmissing-prototypes -Wstrict-prototypes -Wstrict-overflow -Wcast-align \
	     -Wno-overlength-strings -Wno-missing-field-initializers \
	     -O0 -ggdb")
    #-Wdouble-promotion
    set(CMAKE_C_FLAGS
            "${CMAKE_C_FLAGS} -fsanitize=address,undefined -fsanitize-undefined-trap-on-error -fstack-protector-strong")
else ()
    add_compile_options(-Oz -flto -march=native -mtune=native -g -fno-omit-frame-pointer)
endif ()

add_definitions(${GTK3_CFLAGS_OTHER} ${NOTIFY_CFLAGS_OTHER} ${LIBZIP_DEFINITIONS} ${CURL_DEFINITIONS})

set(JPPRON_SRCS
        src/jppron/jppron.c
        src/jppron/database.c
        src/jppron/ajt_audio_index_parser.c
        lib/yyjson.c
        src/objects/dict.c
        include/objects/freqentry.h
)

set(ANKICONNECTC_SRCS
        src/ankiconnectc/send_request.c
        src/ankiconnectc/ankiconnectc.c
)

set(DICTPOPUP_SRCS
        src/dictpopup.c
)

set(DEINFLECTOR_SRCS
        ${DEINFLECTION_RULES_C}
        src/deinflector/deinflector.c
)


# ##############################################################################
# Clang format
# ##############################################################################
if (CLANG_FORMAT)
    file(GLOB_RECURSE ALL_SOURCE_FILES
            ${CMAKE_SOURCE_DIR}/src/*.c
            ${CMAKE_SOURCE_DIR}/src/*.h
            ${CMAKE_SOURCE_DIR}/include/*.h
    )

    add_custom_target(
            format-check
            COMMAND ${CLANG_FORMAT}
            -style=file:${CMAKE_CURRENT_SOURCE_DIR}/.clang-format
            --dry-run
            --Werror
            ${ALL_SOURCE_FILES}
            COMMENT "Checking code format with clang-format"
    )

    add_custom_target(
            format
            COMMAND ${CLANG_FORMAT}
            -style=file
            -i
            ${ALL_SOURCE_FILES}
            COMMENT "Formatting code with clang-format"
    )
endif ()
# ##############################################################################
# Cppcheck
# ##############################################################################
if (CPPCHECK)
    add_custom_target(
            cppcheck
            COMMAND ${CPPCHECK}
            --library=gtk.cfg --library=libcurl.cfg
            --enable=all --std=c11
            --inconclusive
            --quiet
            --force
            -I ${CMAKE_SOURCE_DIR}/include
            ${CMAKE_SOURCE_DIR}/src
    )
endif ()

# ##############################################################################
# check-all
# ##############################################################################
add_custom_target(
        check-all
        COMMAND ${CMAKE_COMMAND} --build . --target format-check
        COMMAND ${CMAKE_COMMAND} --build . --target cppcheck
        DEPENDS format-check cppcheck
        COMMENT "Running format check, static analysis, and tests"
)

# ##############################################################################
# dictpopup
# ##############################################################################
# Compile the ui
set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${GENERATED_DIR})

# #########
set(GRESOURCE_XML ${CMAKE_CURRENT_SOURCE_DIR}/src/frontends/gtk3popup/dictpopup.gresource.xml)
set(GRESOURCE_C ${GENERATED_DIR}/resources.c)

add_custom_command(
        OUTPUT ${GRESOURCE_C}
        COMMAND ${GLIB_COMPILE_RESOURCES}
        ARGS
        --target=${GRESOURCE_C}
        --sourcedir=${CMAKE_CURRENT_SOURCE_DIR}/src/frontends/gtk3popup
        --generate-source
        ${GRESOURCE_XML}
        MAIN_DEPENDENCY ${GRESOURCE_XML}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/frontends/gtk3popup/main-window.ui
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/frontends/gtk3popup/settings-window.ui
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/frontends/gtk3popup/style.css
        VERBATIM
)
add_custom_target(gresource DEPENDS ${GRESOURCE_C})
# ###########

# #### TODO: move
set(DEINFLECTION_RULES_GPERF ${CMAKE_CURRENT_SOURCE_DIR}/src/deinflector/deinflection_rules.gperf)
set(DEINFLECTION_RULES_C ${GENERATED_DIR}/deinflection_rules.c)
add_custom_command(
        OUTPUT ${DEINFLECTION_RULES_C}
        COMMAND ${GPERF}
        ARGS
        -t
        --output-file=${DEINFLECTION_RULES_C}
        ${DEINFLECTION_RULES_GPERF}
        MAIN_DEPENDENCY ${DEINFLECTION_RULES_GPERF}
        VERBATIM
)
add_custom_target(deinflection_rules DEPENDS ${DEINFLECTION_RULES_C})
# #######

add_executable(dictpopup
        src/frontends/gtk3popup/dictpopup-application.c
        src/dictpopup.c
        src/db.c
        src/settings.c
        src/utils/util.c
        src/utils/utf8.c
        src/utils/str.c
        src/platformdep/audio.c
        src/platformdep/notifications.c
        src/platformdep/clipboard.c
        src/platformdep/file_operations.c
        src/platformdep/windowtitle.c
        src/objects/freqentry.c
        ${JPPRON_SRCS}
        ${ANKICONNECTC_SRCS}
        ${GRESOURCE_C}
        ${DEINFLECTION_RULES_C}
        src/deinflector/deinflector.c
        src/frontends/gtk3popup/dict_state_manager.c
        src/frontends/gtk3popup/callbacks.c
        src/frontends/gtk3popup/main.c
)
add_dependencies(dictpopup gresource)
target_compile_definitions(dictpopup PUBLIC NOTIFICATIONS CLIPBOARD GTK3)
target_include_directories(
        dictpopup PRIVATE
        ${GTK3_INCLUDE_DIRS}
        ${NOTIFY_INCLUDE_DIRS}
        ${CURL_INCLUDE_DIRS}
        ${X11_INCLUDE_DIR}
        include/
        lib/
)
target_link_directories(dictpopup PRIVATE
        ${GTK3_LIBRARY_DIRS}
        ${NOTIFY_LIBRARY_DIRS} ${CURL_LIBRARY_DIRS}
)
target_link_libraries(dictpopup PRIVATE
        ${GTK3_LIBRARIES}
        ${NOTIFY_LIBRARIES}
        ${CURL_LIBRARIES}
        ${X11_LIBRARIES}
        -lmecab
        -llmdb
        -rdynamic
)

## X11 window title support
target_compile_definitions(dictpopup PUBLIC HAVEX11)
target_link_libraries(dictpopup PRIVATE -lXfixes -lX11)

# ##############################################################################
# dictpopup-create
# ##############################################################################
# Need to build ui with glib-compile-resources dictpopup.gresource.xml --target=resources.c --generate-source
add_executable(dictpopup-create
        src/dictpopup_create.c
        src/utils/util.c
        lib/yyjson.c
        src/utils/str.c
        src/db.c
        src/yomichan_parser.c
        src/platformdep/file_operations.c
        src/objects/dict.c
        src/objects/freqentry.c)
#target_include_directories(
#        dictpopup-create PRIVATE ${GTK3_INCLUDE_DIRS} ${LIBZIP_INCLUDE_DIRS} include/)
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
        src/settings.c
        src/utils/util.c
        src/platformdep/windowtitle.c
        src/platformdep/file_operations.c
        src/platformdep/notifications.c
        ${ANKICONNECTC_SRCS}
        ${DEINFLECTOR_SRCS}
)
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
        src/utils/util.c
        src/db.c
        src/settings.c
        src/platformdep/file_operations.c
        src/platformdep/clipboard.c
        src/platformdep/windowtitle.c
        ${ANKICONNECTC_SRCS}
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
            src/utils/util.c
            src/utils/utf8.c
            src/utils/str.c
            lib/yyjson.c
            src/objects/dict.c
            tests/main.c
            tests/deinflector_tests.c
            tests/ankiconnect_tests.c
            tests/yomichan_parser_tests.c
            tests/jppron_tests/ajt_audio_index_parser_tests.c
            tests/utils_tests/enclose_word_in_string_tests.c
            tests/dictpopup_tests.c
            tests/jppron_tests/jppron_tests.c
    )
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