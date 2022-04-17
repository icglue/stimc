
set (
    STIMC_HEADERS
    stimc.h
    stimc++.h
)
set (
    STIMC_SOURCES
    stimc.c
    stimc_init.c
    stimc_thread_boost.cpp
)

add_library (stimc SHARED ${STIMC_SOURCES})

set_target_properties (
    stimc PROPERTIES

    OUTPUT_NAME   stimc
    VERSION       ${PROJECT_VERSION}
    SOVERSION     ${STIMC_VERSION_MAJOR}
    PUBLIC_HEADER stimc.h
)

