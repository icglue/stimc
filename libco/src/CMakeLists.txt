
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

#if (THREAD_IMPL STREQUAL "libco-local")
#    target_include_directories (stimc PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
#    target_link_libraries (stimc libco_local)
#endif ()

