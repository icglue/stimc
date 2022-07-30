cmake_minimum_required (VERSION 3.23)

# Project setup
set (STIMC_DESCRIPTION "a lightweight Verilog-vpi Wrapper for Stimuli Generation")

set (STIMC_VERSION_MAJOR 1)
set (STIMC_VERSION_MINOR 2)
set (STIMC_VERSION_PATCH 0)

set (STIMC_VERSION ${STIMC_VERSION_MAJOR}.${STIMC_VERSION_MINOR}.${STIMC_VERSION_PATCH})

project (
    stimc
    VERSION     ${STIMC_VERSION}
    DESCRIPTION ${STIMC_DESCRIPTION}
)

# Options
set (
    SIMULATOR icarus
    CACHE
    STRING "set simulator for vpi headers and test, one of: icarus, cvc, ncsim, xcelium (default=icarus)"
)
set_property (
    CACHE
    SIMULATOR
    PROPERTY STRINGS icarus cvc ncsim xcelium
)

set (
    THREAD_IMPL libco-local
    CACHE
    STRING "set thread implementation, one of: libco-local, libco, pcl, boost1, boost2 (default=libco-local)"
)
set_property (
    CACHE
    THREAD_IMPL
    PROPERTY STRINGS libco-local libco pcl boost1 boost2
)

set (
    WARN_LEVEL default
    CACHE
    STRING "set compiler warning level, one of: default, strict"
)
set_property (
    CACHE
    WARN_LEVEL
    PROPERTY STRINGS default strict
)

# Current dir
include (GNUInstallDirs)
install (FILES README.md LICENSE.md TYPE DOC)

# Subdirs
if (THREAD_IMPL STREQUAL "libco-local")
    add_subdirectory (libco)
endif ()
add_subdirectory (lib)
add_subdirectory (doxy)
add_subdirectory (examples)

# Tests
enable_testing ()

# Packing
set (
    PACK_FORMAT "tar.xz"
    CACHE
    STRING "set pack format, one of zip, tar, tar.bz2, tar.gz, tar.xz, tar.zstd"
)
set_property (
    CACHE
    PACK_FORMAT
    PROPERTY STRINGS zip tar tar.bz2 tar.gz tar.xz tar.zstd
)

set (PACK_NAME   "stimc-${STIMC_VERSION}")
set (PACK_PREFIX "${PACK_NAME}/")
set (PACK_REF    HEAD)
set (PACK_FILE   "${PACK_NAME}.${PACK_FORMAT}")

foreach (ext IN ITEMS tar zip)
    add_custom_command (
        OUTPUT  ${PACK_NAME}.${ext}
        COMMAND git archive --format=${ext} --prefix=${PACK_PREFIX} ${PACK_REF} --output=${CMAKE_CURRENT_BINARY_DIR}/${PACK_NAME}.${ext}
        COMMENT "Create ${ext} archive"
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        VERBATIM
    )
endforeach ()

set (PACK_COMP_EXTENSIONS xz gz   bz2)
set (PACK_COMP_TOOLS      xz gzip bzip2)
foreach (ext tool IN ZIP_LISTS PACK_COMP_EXTENSIONS PACK_COMP_TOOLS)
    add_custom_command (
        OUTPUT  ${PACK_NAME}.tar.${ext}
        DEPENDS ${PACK_NAME}.tar
        COMMAND ${tool} ${PACK_NAME}.tar --force
        COMMENT "Create tar.${ext} archive"
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        VERBATIM
    )
endforeach ()
add_custom_command (
    OUTPUT  ${PACK_NAME}.tar.zstd
    DEPENDS ${PACK_NAME}.tar
    COMMAND zstd -10 --rm ${PACK_NAME}.tar -o ${PACK_NAME}.tar.zstd --force
    COMMENT "Create tar.zstd archive"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    VERBATIM
)

add_custom_target (
    pack
    DEPENDS ${PACK_NAME}.${PACK_FORMAT}
    COMMENT "Create archive"
)
