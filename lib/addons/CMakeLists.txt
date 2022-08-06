add_library (logging-vpi SHARED)

set (
    LOGGING_HEADERS
    logging.h
)
set (
    LOGGING_SOURCES
    logging.c
)

set (
    UNCRUSTIFY_FILES_C
    ${LOGGING_SOURCES}
    ${LOGGING_HEADERS}
    stimc-export.c
)
set (
    UNCRUSTIFY_FILES_CXX
    stimc_sc_compat.h
)

# TODO:
# - install addon headers
# - install addon sources (to shared?)

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

install (TARGETS logging-vpi PUBLIC_HEADER)

foreach (file ${UNCRUSTIFY_FILES_C})
    add_uncrustify_file (${file} C)
endforeach ()
foreach (file ${UNCRUSTIFY_FILES_CXX})
    add_uncrustify_file (${file} CPP)
endforeach ()
