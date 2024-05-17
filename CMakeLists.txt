cmake_minimum_required(VERSION 3.5)
project(dictpopup C)

set(CMAKE_C_COMPILER gcc)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
pkg_check_modules(NOTIFY REQUIRED libnotify)
find_package(LibZip REQUIRED)
find_package(CURL REQUIRED)
find_package(cgreen)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DDEBUG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic -Werror -Wstrict-prototypes -Wdouble-promotion -Wshadow \
	     -Wno-unused-parameter -Wno-sign-conversion -Wno-unused-function -Wpointer-arith \
	     -Wmissing-prototypes -Wstrict-prototypes -Wstrict-overflow -Wcast-align \
	     -fsanitize=address,undefined -fsanitize-undefined-trap-on-error -fstack-protector-strong \
	     -O0 -ggdb")
else()
    add_compile_options(-Ofast -flto -march=native)
endif()

add_definitions(${GTK3_CFLAGS_OTHER} ${NOTIFY_CFLAGS_OTHER} ${LIBZIP_DEFINITIONS} ${CURL_DEFINITIONS})
set(SRCS src/ankiconnectc.c src/database.c src/db.c src/deinflector.c src/dictpopup.c src/jppron.c src/platformdep.c src/settings.c src/util.c src/pdjson.c)

add_executable(dictpopup src/frontends/gtk3popup.c ${SRCS})
target_compile_definitions(dictpopup PUBLIC NOTIFICATIONS)
target_include_directories(dictpopup PRIVATE ${GTK3_INCLUDE_DIRS} ${NOTIFY_INCLUDE_DIRS} ${CURL_INCLUDE_DIRS} include/)
target_link_directories(dictpopup PRIVATE ${GTK3_LIBRARY_DIRS} ${NOTIFY_LIBRARY_DIRS} ${CURL_LIBRARY_DIRS})
target_link_libraries(dictpopup PRIVATE ${GTK3_LIBRARIES} ${NOTIFY_LIBRARIES} ${CURL_LIBRARIES} -llmdb -lmecab)


add_executable(dictpopup-create src/dictpopup_create.c ${SRCS})
target_include_directories(dictpopup-create PRIVATE ${GTK3_INCLUDE_DIRS} ${LIBZIP_INCLUDE_DIRS} include/)
target_link_directories(dictpopup-create PRIVATE ${GTK3_LIBRARY_DIRS} ${LIBZIP_LIBRARY_DIRS} ${CURL_LIBRARY_DIRS})
target_link_libraries(dictpopup-create PRIVATE ${GTK3_LIBRARIES} -lzip -llmdb)


add_executable(run_tests tests/main.c tests/deinflector_tests.c src/deinflector.c src/util.c)
target_include_directories(run_tests PRIVATE ${GTK3_INCLUDE_DIRS} include/)
target_link_libraries(run_tests PRIVATE cgreen ${GTK3_LIBRARIES} ${NOTIFY_LIBRARIES} ${CURL_LIBRARIES} -llmdb -lmecab)