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

# Current dir
include (GNUInstallDirs)
install (FILES README.md LICENSE.md TYPE DOC)

# Subdirs
add_subdirectory (lib)
