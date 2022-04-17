
set (
    LIBCO_SOURCES
    libco.c
)

set (
    LIBCO_HEADERS
    libco.h
)

add_library (libco_local OBJECT ${LIBCO_SOURCES})
set_property (
    TARGET libco_local
    PROPERTY POSITION_INDEPENDENT_CODE ON
)

