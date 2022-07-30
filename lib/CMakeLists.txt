# uncrustify preparation
set (UNCRUSTIFY_CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/.uncrustify.cfg)

macro (add_uncrustify_file f l)
    string (MAKE_C_IDENTIFIER ${f} target)

    add_custom_target (
        uncrustify_${target}
        uncrustify -L 2 -l ${l} -c ${UNCRUSTIFY_CONFIG} --replace --no-backup ${f}
        VERBATIM
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        SOURCES ${UNCRUSTIFY_CONFIG}
        COMMENT "uncrustifying ${f}"
    )
    add_dependencies (uncrustify uncrustify_${target})
endmacro ()

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

