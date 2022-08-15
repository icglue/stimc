set (
    LIBCO_SOURCES
    libco.c
)

set (
    LIBCO_HEADERS
    libco.h
)

add_library (libco_local OBJECT ${LIBCO_SOURCES})

set_target_properties (
    libco_local PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    PUBLIC_HEADER "${LIBCO_HEADERS}"
)
