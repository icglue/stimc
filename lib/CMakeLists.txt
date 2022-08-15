# uncrustify preparation
include (${CMAKE_SOURCE_DIR}/scripts/cmake_uncrustify_helpers.cmake)

add_custom_target (
    uncrustify
    COMMENT "uncrustify sources"
)

# compile checks
if (WARN_LEVEL STREQUAL strict)
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Weffc++>)
    add_compile_options(-Wall -Wextra -Wshadow -pedantic -Werror)
endif ()

# subdirs
add_subdirectory (src)
add_subdirectory (addons)
add_subdirectory (pkgconfig)

