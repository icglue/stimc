option (ADDONS "Build and install addons" OFF)

set (
    LOGGING_HEADERS
    logging.h
)
set (
    LOGGING_SOURCES
    logging.c
)
set (
    ADDON_HEADERS
    stimc_sc_compat.h
)
set (
    ADDON_SOURCES
    stimc-export.c
)

set (
    UNCRUSTIFY_FILES_C
    ${LOGGING_SOURCES}
    ${LOGGING_HEADERS}
    ${ADDON_SOURCES}
)
set (
    UNCRUSTIFY_FILES_CXX
    ${ADDON_HEADERS}
)

if (ADDONS)
    add_library (logging-vpi SHARED)

    set (CMAKE_INCLUDE_CURRENT_DIR yes)

    include_directories (${INC_VPI_USER})
    target_compile_features(stimc PUBLIC c_std_11)

    set_target_properties (
        logging-vpi PROPERTIES

        OUTPUT_NAME   logging-vpi
        VERSION       ${PROJECT_VERSION}
        SOVERSION     ${STIMC_VERSION_MAJOR}
        PUBLIC_HEADER "${LOGGING_HEADERS}"
        SOURCES       "${LOGGING_SOURCES}"
    )

    set_target_properties (
        stimc PROPERTIES
        PUBLIC_HEADER "${ADDON_HEADERS}"
    )


    install (TARGETS logging-vpi PUBLIC_HEADER)
    install (TARGETS stimc       PUBLIC_HEADER)

    install (FILES "${ADDON_SOURCES}" TYPE DOC)

    foreach (file ${UNCRUSTIFY_FILES_C})
        add_uncrustify_file (${file} C)
    endforeach ()
    foreach (file ${UNCRUSTIFY_FILES_CXX})
        add_uncrustify_file (${file} CPP)
    endforeach ()
endif()
