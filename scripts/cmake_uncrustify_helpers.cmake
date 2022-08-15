set (UNCRUSTIFY_CONFIG ${CMAKE_SOURCE_DIR}/scripts/uncrustify.cfg)

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
