find_path(MECAB_INCLUDE_DIR
    NAMES mecab.h
    PATHS
    /usr/include
    /usr/local/include
    /opt/local/include
    $ENV{MECAB_DIR}/include
)

find_library(MECAB_LIBRARY
    NAMES mecab
    PATHS
    /usr/lib
    /usr/local/lib
    /opt/local/lib
    $ENV{MECAB_DIR}/lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(MECAB
    REQUIRED_VARS MECAB_LIBRARY MECAB_INCLUDE_DIR
)

if(MECAB_FOUND)
    set(MECAB_LIBRARIES ${MECAB_LIBRARY})
    set(MECAB_INCLUDE_DIRS ${MECAB_INCLUDE_DIR})
    
    if(NOT TARGET MECAB::MECAB)
        add_library(MECAB::MECAB UNKNOWN IMPORTED)
        set_target_properties(MECAB::MECAB PROPERTIES
            IMPORTED_LOCATION "${MECAB_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${MECAB_INCLUDE_DIR}"
        )
    endif()
endif()

mark_as_advanced(MECAB_INCLUDE_DIR MECAB_LIBRARY)
