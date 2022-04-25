option (BUILD_DOC "Build documentation" OFF)

find_package (Doxygen)

if (DOXYGEN_FOUND)
    set (doxyfile_in ${CMAKE_CURRENT_SOURCE_DIR}/lib.doxyfile.in)
    set (doxyfile    ${CMAKE_CURRENT_BINARY_DIR}/lib.doxyfile)

    configure_file (${doxyfile_in} ${doxyfile} @ONLY)

    add_custom_command (
        OUTPUT html man
        COMMAND ${DOXYGEN_EXECUTABLE} ${doxyfile}
        COMMENT "Create html and man"
        VERBATIM
    )

    add_custom_target (
        doc
        DEPENDS html man
        COMMENT "Create Documentation"
        VERBATIM
    )

    if (BUILD_DOC)
        add_custom_target (
            build_doc
            ALL
            DEPENDS doc
            VERBATIM
        )

        install (
            DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html
            TYPE DOC
        )
        install (
            DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/man
            TYPE MAN
        )
    endif ()
elseif (BUILD_DOC)
    message (FATAL_ERROR "doxygen not found")
endif ()
