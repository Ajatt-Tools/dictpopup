cmake_minimum_required(VERSION 3.5)
project(dictpopup C)

set(CMAKE_C_COMPILER gcc)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
pkg_check_modules(NOTIFY REQUIRED libnotify)

find_package(LibZip REQUIRED)
find_package(CURL REQUIRED)

set(C_STANDARD 17)
add_compile_options(-Ofast -flto -march=native)

include_directories(${GTK3_INCLUDE_DIRS} ${NOTIFY_INCLUDE_DIRS} ${LIBZIP_INCLUDE_DIRS} ${CURL_INCLUDE_DIRS} include/)
link_directories(${GTK3_LIBRARY_DIRS} ${NOTIFY_LIBRARY_DIRS} ${LIBZIP_LIBRARY_DIRS} ${CURL_LIBRARY_DIRS})
add_definitions(${GTK3_CFLAGS_OTHER} ${NOTIFY_CFLAGS_OTHER} ${LIBZIP_DEFINITIONS} ${CURL_DEFINITIONS})

set(SRCS src/ankiconnectc.c src/database.c src/db.c src/deinflector.c src/dictpopup.c src/jppron.c src/platformdep.c src/settings.c src/util.c src/pdjson.c)

add_executable(dictpopup src/frontends/gtk3popup.c ${SRCS})

target_link_libraries(dictpopup PRIVATE ${GTK3_LIBRARIES} ${NOTIFY_LIBRARIES} ${CURL_LIBRARIES} ${LIBZIP_LIBRARIES} -llmdb -lmecab)