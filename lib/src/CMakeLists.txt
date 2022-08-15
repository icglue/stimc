# general setup
add_library (stimc SHARED)

set (
    STIMC_HEADERS
    stimc.h
    stimc++.h
)
set (
    STIMC_SOURCES
    stimc.c
    stimc_init.c
)
set (
    STIMC_SOURCES_BOOST
    stimc_thread_boost.cpp
)

set (
    UNCRUSTIFY_FILES_C
    ${STIMC_SOURCES}
    stimc.h
    stimc_thread.inl
)
set (
    UNCRUSTIFY_FILES_CXX
    ${STIMC_SOURCES_BOOST}
    stimc++.h
)

# pkgconfig vars
set (STIMC_PC_INCFLAGS)
set (STIMC_PC_LIBFLAGS)
set (STIMC_PC_REQUIRES)

include (${CMAKE_SOURCE_DIR}/scripts/cmake_simulator.cmake)

list (APPEND STIMC_PC_INCFLAGS "-I${INC_VPI_USER}")
include_directories (${INC_VPI_USER})

include (${CMAKE_SOURCE_DIR}/scripts/cmake_thread_impl.cmake)

# stimc config header
set (STIMC_DISABLE_CLEANUP           ${DISABLE_CLEANUP})
set (STIMC_VALVECTOR_MAX_STATIC      ${VALVECTOR_MAX_STATIC})
set (STIMC_THREAD_STACK_SIZE_DEFAULT ${THREAD_STACK_SIZE_DEFAULT})

configure_file (stimc_config.h.in stimc_config.h)

# library build
set (CMAKE_INCLUDE_CURRENT_DIR yes)

target_compile_features(stimc PUBLIC cxx_std_11)
target_compile_features(stimc PUBLIC c_std_11)

set_target_properties (
    stimc PROPERTIES

    OUTPUT_NAME   stimc
    VERSION       ${PROJECT_VERSION}
    SOVERSION     ${STIMC_VERSION_MAJOR}
    PUBLIC_HEADER "${STIMC_HEADERS}"
    SOURCES       "${STIMC_SOURCES}"
)

install (TARGETS stimc PUBLIC_HEADER)

# uncrustify
foreach (file ${UNCRUSTIFY_FILES_C})
    add_uncrustify_file (${file} C)
endforeach ()
foreach (file ${UNCRUSTIFY_FILES_CXX})
    add_uncrustify_file (${file} CPP)
endforeach ()

# package-config vars
set (STIMC_PC_INCFLAGS ${STIMC_PC_INCFLAGS} PARENT_SCOPE)
set (STIMC_PC_LIBFLAGS ${STIMC_PC_LIBFLAGS} PARENT_SCOPE)
set (STIMC_PC_REQUIRES ${STIMC_PC_REQUIRES} PARENT_SCOPE)
