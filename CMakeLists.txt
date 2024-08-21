cmake_minimum_required(VERSION 3.5)

project(dictpopup
        VERSION 0.3
        HOMEPAGE_URL "https://github.com/Ajatt-Tools/dictpopup"
        LANGUAGES C
)

include(GNUInstallDirs)
include(CTest)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Options
option(BUILD_TESTS "Build tests" OFF)

# Find packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED IMPORTED_TARGET gtk+-3.0)
pkg_check_modules(GLIB REQUIRED IMPORTED_TARGET glib-2.0)
pkg_check_modules(NOTIFY REQUIRED IMPORTED_TARGET libnotify)
find_package(LibZip REQUIRED)
find_package(CURL REQUIRED)
find_package(LMDB REQUIRED)
find_package(MECAB REQUIRED)

###### Binaries needed for compilation
find_program(GLIB_COMPILE_SCHEMAS NAMES glib-compile-schemas)
if (NOT GLIB_COMPILE_SCHEMAS)
    message(FATAL_ERROR "glib-compile-schemas binary not found")
endif ()

find_program(GLIB_COMPILE_RESOURCES NAMES glib-compile-resources)
if (NOT GLIB_COMPILE_RESOURCES)
    message(FATAL_ERROR "glib-compile-resources binary not found")
endif ()

find_program(GPERF NAMES gperf)
if (NOT GPERF)
    message(FATAL_ERROR "gperf binary not found")
endif ()

####### Optional dependencies
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

# ##############################################################################
# General compilation
# ##############################################################################
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_COMPILER gcc)

include_directories(
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/lib
)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif ()

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} \
            -Wall -Wextra -Werror -Wstrict-prototypes -Wshadow -Wdouble-promotion \
            -Wmissing-prototypes -Wstrict-prototypes -Wstrict-overflow -Wcast-align -Wpointer-arith \
            -Wno-sign-conversion -Wno-unused-function -Wno-unused-parameter \
            -Wno-overlength-strings -Wno-missing-field-initializers \
            -fsanitize=address,undefined -fsanitize-undefined-trap-on-error -fstack-protector-strong \
            -O0 -ggdb \
")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fsanitize=address")
#set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -pg -g -O0 -Wall")

set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Ofast -flto -march=native -mtune=native -g -fno-omit-frame-pointer")

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# ##############################################################################
# Sub modules
# ##############################################################################
set(JPPRON_SRCS
        src/jppron/jppron.c
        src/jppron/jppron_database.c
        src/jppron/ajt_audio_index_parser.c
        src/jppron/jppron_objects.c
        lib/yyjson.c
        src/objects/dict.c
        include/objects/freqentry.h
)

set(ANKICONNECTC_SRCS
        src/ankiconnectc/send_request.c
        src/ankiconnectc/ankiconnectc.c
        src/anki.c
)

set(DICTLOOKUP_SRCS
        src/dictionary_lookup.c
        src/deinflector/deinflector.c
        src/deinflector/kata2hira.c
        src/db.c
        #        ${DEINFLECTION_RULES_C} # Doesn't get generated if put in here
)

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
set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${GENERATED_DIR})

############# UI #################
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
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/frontends/gtk3popup/dp-preferences-window.ui
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/frontends/gtk3popup/style.css
        VERBATIM
)

############# SETTINGS #################
set(GSETTINGS_DIR ${CMAKE_INSTALL_DATADIR}/glib-2.0/schemas)
set(GSETTINGS_SCHEMA ${CMAKE_CURRENT_SOURCE_DIR}/src/frontends/gtk3popup/com.github.Ajatt-Tools.dictpopup.gschema.xml)
set(COMPILED_SETTINGS_SCHEMA ${CMAKE_CURRENT_BINARY_DIR}/gschemas.compiled)

add_custom_command(
        OUTPUT ${COMPILED_SETTINGS_SCHEMA}
        COMMAND ${GLIB_COMPILE_SCHEMAS} --strict --dry-run --schema-file=${GSETTINGS_SCHEMA}
        COMMAND ${GLIB_COMPILE_SCHEMAS} --strict --targetdir=${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src/frontends/gtk3popup
        DEPENDS ${GSETTINGS_SCHEMA}
        COMMENT "Compiling GSettings schemas"
)

add_custom_target(gsettings-schemas ALL DEPENDS ${COMPILED_SETTINGS_SCHEMA})

install(FILES ${GSETTINGS_SCHEMA} DESTINATION ${GSETTINGS_DIR})
install(CODE "execute_process(COMMAND ${GLIB_COMPILE_SCHEMAS} ${CMAKE_INSTALL_PREFIX}/${GSETTINGS_DIR})")

############# DEINFLECTION RULES #################
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

############# MAIN #################
add_executable(dictpopup
        src/frontends/gtk3popup/dictpopup-application.c
        src/frontends/gtk3popup/dp-settings.c
        src/frontends/gtk3popup/dp-preferences-window.c
        src/frontends/gtk3popup/dict_state_manager.c
        src/frontends/gtk3popup/callbacks.c
        src/frontends/gtk3popup/main.c
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
        ${DICTLOOKUP_SRCS}
        ${GRESOURCE_C}
        ${DEINFLECTION_RULES_C}
        ${COMPILED_SETTINGS_SCHEMA}
)
target_compile_definitions(dictpopup PUBLIC NOTIFICATIONS CLIPBOARD GTK3)
target_link_libraries(dictpopup PRIVATE
        LMDB::LMDB
        MECAB::MECAB
        PkgConfig::GTK3
        PkgConfig::NOTIFY
        CURL::libcurl
        -rdynamic
)

if (X11_FOUND)
    target_link_libraries(dictpopup PRIVATE ${X11_LIBRARIES})
endif ()

# ##############################################################################
# dictpopup-config
# ##############################################################################

add_executable(dictpopup-config
        src/frontends/gtk3popup/dictpopup-config_main.c
        src/frontends/gtk3popup/dp-settings.c
        src/frontends/gtk3popup/dp-preferences-window.c
        src/utils/util.c
        src/utils/str.c
        src/db.c
        src/platformdep/file_operations.c
        src/platformdep/windowtitle.c # Remove?
        src/deinflector/kata2hira.c # Remove?
        src/utils/utf8.c # Remove?
        src/platformdep/audio.c # Remove?
        ${JPPRON_SRCS}
        ${ANKICONNECTC_SRCS}
        ${GRESOURCE_C}
        ${COMPILED_SETTINGS_SCHEMA}
)
target_link_libraries(dictpopup-config PRIVATE
        LMDB::LMDB
        PkgConfig::GTK3
        CURL::libcurl
)

# ##############################################################################
# dictpopup-create
# ##############################################################################
add_executable(dictpopup-create
        src/dictpopup_create/dictpopup_create.c
        src/dictpopup_create/objects.c
        src/utils/util.c
        lib/yyjson.c
        src/utils/str.c
        src/db.c
        src/dictpopup_create/yomichan_parser.c
        src/platformdep/file_operations.c
        src/objects/dict.c
        src/objects/freqentry.c
)
target_link_libraries(dictpopup-create PRIVATE
        PkgConfig::GTK3
        libzip::zip
        LMDB::LMDB
)

# ##############################################################################
# dictpopup-window
# ##############################################################################
pkg_check_modules(GTK4 REQUIRED gtk4)
add_executable(dictpopup-win EXCLUDE_FROM_ALL
        src/frontends/gtk4window.c
        src/db.c
        src/utils/util.c
        src/platformdep/windowtitle.c
        src/platformdep/file_operations.c
        src/platformdep/notifications.c
        ${ANKICONNECTC_SRCS}
        ${DEINFLECTOR_SRCS}
        ${DICTLOOKUP_SRCS}
)
target_compile_definitions(dictpopup-win PUBLIC NOTIFICATIONS)
target_include_directories(
        dictpopup-win PRIVATE
        ${GTK4_INCLUDE_DIRS}
        ${NOTIFY_INCLUDE_DIRS}
        ${CURL_INCLUDE_DIRS}
)
target_link_directories(dictpopup-win PRIVATE
        ${GTK4_LIBRARY_DIRS}
        ${NOTIFY_LIBRARY_DIRS}
        ${CURL_LIBRARY_DIRS}
)
target_link_libraries(dictpopup-win PRIVATE
        ${GTK4_LIBRARIES}
        ${NOTIFY_LIBRARIES}
        ${CURL_LIBRARIES}
        ${DICTLOOKUP_SRCS}
)

# ##############################################################################
# cli
# ##############################################################################
add_executable(dictpopup-cli EXCLUDE_FROM_ALL src/frontends/cli.c
        src/utils/util.c
        src/db.c
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
if (BUILD_TESTS)
    enable_testing()
    find_package(cgreen REQUIRED)
    add_executable(c_tests
            src/jppron/jppron_objects.c
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
            ${DEINFLECTION_RULES_C}
            src/deinflector/deinflector.c
            tests/anki_card_preparation_tests.c
    )
    target_compile_definitions(c_tests PRIVATE UNIT_TEST CLIPBOARD)
    target_include_directories(c_tests PRIVATE
            ${GTK3_INCLUDE_DIRS}
            ${CGREEN_INCLUDE_DIRS}
            lib/
            include/
            src/
            src/jppron
    )
    target_link_libraries(c_tests PRIVATE
            ${CGREEN_LIBRARIES}
            MECAB::MECAB
            PkgConfig::GTK3
            libzip::zip
    )
    add_test(NAME run_c_tests COMMAND $<TARGET_FILE:c_tests>)

    #    add_test(NAME run_shell_test COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/full_cycle_test.sh)
endif ()

# ##############################################################################
# Install
# ##############################################################################
install(TARGETS dictpopup DESTINATION bin)
install(TARGETS dictpopup-config DESTINATION bin)
install(FILES ${GSETTINGS_SCHEMA} DESTINATION ${GSETTINGS_DIR})
install(CODE "execute_process(COMMAND ${GLIB_COMPILE_SCHEMAS} ${CMAKE_INSTALL_PREFIX}/${GSETTINGS_DIR})")
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