
# general setup
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

# vpi header
if (SIMULATOR STREQUAL icarus)
    find_program (ICARUSVPI iverilog-vpi REQUIRED)
    execute_process (
        COMMAND iverilog-vpi --cflags
        COMMAND tr " " "\n"
        COMMAND grep "^-I"
        COMMAND sed -e "s/^-I//"
        OUTPUT_VARIABLE ICARUS_INCDIR
    )
    find_path (
        VPI_HEADER_PATH
        vpi_user.h
        REQUIRED
        HINTS ${ICARUS_INCDIR}
        PATH_SUFFIXES iverilog
    )
else ()
    # TODO: other simulators
    message (FATAL_ERROR "unknown or invalid simulator specified: ${SIMULATOR}")
endif ()

# thread implementation
if (THREAD_IMPL STREQUAL libco-local)
    # TODO: define, include, add sources
elseif (THREAD_IMPL STREQUAL libco)
else ()
    message (FATAL_ERROR "unknown or invalid thread implementation: ${THREAD_IMPL}")
endif ()
# TODO: thread implementation (defines + inc + link?)

# library build
include_directories (${VPI_HEADER_PATH})
set (CMAKE_INCLUDE_CURRENT_DIR yes)

add_library (stimc SHARED ${STIMC_SOURCES})

target_compile_features(stimc PUBLIC cxx_std_11)
target_compile_features(stimc PUBLIC c_std_11)

set_target_properties (
    stimc PROPERTIES

    OUTPUT_NAME   stimc
    VERSION       ${PROJECT_VERSION}
    SOVERSION     ${STIMC_VERSION_MAJOR}
    PUBLIC_HEADER "${STIMC_HEADERS}"
)

install (TARGETS stimc PUBLIC_HEADER)
