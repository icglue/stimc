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

# Current dir
include (GNUInstallDirs)
install (FILES README.md LICENSE.md TYPE DOC)

# Subdirs
if (THREAD_IMPL STREQUAL "libco-local")
    add_subdirectory (libco)
endif ()
add_subdirectory (lib)
