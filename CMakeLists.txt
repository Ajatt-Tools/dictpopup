cmake_minimum_required(VERSION 3.5)
project(dictpopup C)

include(GNUInstallDirs)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
pkg_check_modules(NOTIFY REQUIRED libnotify)
find_package(LibZip REQUIRED)
find_package(CURL REQUIRED)

set(CMAKE_C_COMPILER gcc)
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DDEBUG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic -Werror -Wstrict-prototypes -Wdouble-promotion -Wshadow \
	     -Wno-unused-parameter -Wno-sign-conversion -Wno-unused-function -Wpointer-arith \
	     -Wmissing-prototypes -Wstrict-prototypes -Wstrict-overflow -Wcast-align \
	     -fsanitize=address,undefined -fsanitize-undefined-trap-on-error -fstack-protector-strong \
	     -O0 -ggdb")
else ()
    add_compile_options(-Ofast -flto -march=native)
endif ()

add_definitions(${GTK3_CFLAGS_OTHER} ${NOTIFY_CFLAGS_OTHER} ${LIBZIP_DEFINITIONS} ${CURL_DEFINITIONS})
set(SRCS src/ankiconnectc.c src/database.c src/db.c src/deinflector.c src/dictpopup.c src/jppron.c src/platformdep.c src/settings.c src/util.c src/pdjson.c)


############################################################
# dictpopup
############################################################
add_executable(dictpopup src/frontends/gtk3popup.c ${SRCS})
target_compile_definitions(dictpopup PUBLIC NOTIFICATIONS)
target_include_directories(dictpopup PRIVATE ${GTK3_INCLUDE_DIRS} ${NOTIFY_INCLUDE_DIRS} ${CURL_INCLUDE_DIRS} include/)
target_link_directories(dictpopup PRIVATE ${GTK3_LIBRARY_DIRS} ${NOTIFY_LIBRARY_DIRS} ${CURL_LIBRARY_DIRS})
target_link_libraries(dictpopup PRIVATE ${GTK3_LIBRARIES} ${NOTIFY_LIBRARIES} ${CURL_LIBRARIES} -llmdb -lmecab)


############################################################
# dictpopup-create
############################################################
add_executable(dictpopup-create src/dictpopup_create.c ${SRCS})
target_include_directories(dictpopup-create PRIVATE ${GTK3_INCLUDE_DIRS} ${LIBZIP_INCLUDE_DIRS} include/)
target_link_directories(dictpopup-create PRIVATE ${GTK3_LIBRARY_DIRS} ${LIBZIP_LIBRARY_DIRS} ${CURL_LIBRARY_DIRS})
target_link_libraries(dictpopup-create PRIVATE ${GTK3_LIBRARIES} -lzip -llmdb)


############################################################
# Tests
############################################################
find_package(cgreen)
if (CGREEN_FOUND)
  enable_testing()
  add_executable(run_tests tests/main.c tests/deinflector_tests.c src/deinflector.c src/util.c)
  target_include_directories(run_tests PRIVATE ${GTK3_INCLUDE_DIRS} include/)
  target_link_libraries(run_tests PRIVATE cgreen ${GTK3_LIBRARIES} ${NOTIFY_LIBRARIES} ${CURL_LIBRARIES} -llmdb -lmecab)
  add_test(test_all run_tests)
endif()


############################################################
# Clang format check
############################################################
find_program(CLANG_FORMAT_EXECUTABLE clang-format)
file(GLOB ALL_SRC_FILES "src/*.c" "tests/*.c" "include/*.h")
function(run_clang_format target)
    add_custom_target(${target}
            COMMAND ${CLANG_FORMAT_EXECUTABLE} --dry-run --Werror ${ARGN}
            COMMENT "Running clang-format on ${ARGN}"
    )
endfunction()
run_clang_format(clang_format_check ${ALL_SRC_FILES})


############################################################
# Install
############################################################
install(TARGETS dictpopup dictpopup-create DESTINATION bin)
install(FILES config.ini DESTINATION etc)
install(DIRECTORY man1/ DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
        FILES_MATCHING PATTERN "*.1")


############################################################
# Create DEB
############################################################
set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "butterkeks")
set(CPACK_PACKAGE_VERSION 0.1.5)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "curl,libgtk-3-0,libzip4,mecab,libnotify4,liblmdb0")
set(CPACK_PACKAGE_CONTACT "butterkeks@fedora.email")
include(CPack)