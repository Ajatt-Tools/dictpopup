MESSAGE(STATUS "Looking for liblmdb")

FIND_PATH(LMDB_INCLUDE_DIR
  NAMES lmdb.h
  PATH_SUFFIXES include/ include/lmdb/
  PATHS "${PROJECT_SOURCE_DIR}"
  ${LMDB_ROOT}
  $ENV{LMDB_ROOT}
  /usr/local/
  /usr/
)

if(STATIC)
  if(MINGW)
    find_library(LMDB_LIBRARY liblmdb.dll.a)
  else()
    find_library(LMDB_LIBRARY liblmdb.a)
  endif()
else()
  find_library(LMDB_LIBRARY lmdb)
endif()


if(LMDB_INCLUDE_DIR AND LMDB_LIBRARY)
    set(LMDB_FOUND TRUE)
    if(NOT TARGET LMDB::LMDB)
        add_library(LMDB::LMDB INTERFACE IMPORTED)
        set_target_properties(LMDB::LMDB PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${LMDB_INCLUDE_DIR}"
            INTERFACE_LINK_LIBRARIES "${LMDB_LIBRARY}"
        )
    endif()
else()
    message(FATAL_ERROR "LMDB not found. Please install LMDB or specify its location.")
endif()
