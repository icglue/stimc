cmake_minimum_required (VERSION 3.17)

# Project setup
set (STIMC_DESCRIPTION "a Lightweight Verilog-VPI Wrapper for Stimuli Generation")

set (STIMC_VERSION_MAJOR 1)
set (STIMC_VERSION_MINOR 3)
set (STIMC_VERSION_PATCH 0)

include (${CMAKE_SOURCE_DIR}/scripts/cmake_patch_version.cmake)

set (STIMC_VERSION ${STIMC_VERSION_MAJOR}.${STIMC_VERSION_MINOR}.${STIMC_VERSION_PATCH})

project (
    stimc
    VERSION     ${STIMC_VERSION}
    DESCRIPTION ${STIMC_DESCRIPTION}
)

include (${CMAKE_SOURCE_DIR}/scripts/cmake_options.cmake)

# Current dir
include (GNUInstallDirs)

install (
    FILES
    README.md LICENSE.md
    TYPE DOC
)

# Subdirs
if (THREAD_IMPL STREQUAL "libco-local")
    add_subdirectory (libco)
endif ()
add_subdirectory (lib)
add_subdirectory (doxy)
add_subdirectory (examples)

# Tests
enable_testing ()

# Pack
include (${CMAKE_SOURCE_DIR}/scripts/cmake_packaging.cmake)
