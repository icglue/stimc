# check for git version
find_program (GIT git)

if (NOT ${GIT} STREQUAL "GIT-NOTFOUND")
    # check for git repository
    execute_process (
        COMMAND ${GIT} rev-parse --show-toplevel
        OUTPUT_VARIABLE STIMC_GIT_REPOSITORY_ROOT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE STIMC_GIT_REPOSITORY_ERROR
        ERROR_QUIET
    )

    if ((${STIMC_GIT_REPOSITORY_ERROR} EQUAL 0) AND ("${STIMC_GIT_REPOSITORY_ROOT}" STREQUAL "${CMAKE_SOURCE_DIR}"))
        execute_process (
            COMMAND ${GIT} rev-list --count "v${STIMC_VERSION_MAJOR}.${STIMC_VERSION_MINOR}..HEAD"
            OUTPUT_VARIABLE STIMC_VERSION_PATCH_GIT
            OUTPUT_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE STIMC_VERSION_GIT_ERROR
        )
        if (${STIMC_VERSION_GIT_ERROR} EQUAL 0)
            set (STIMC_VERSION_PATCH ${STIMC_VERSION_PATCH_GIT})

            configure_file (scripts/cmake_patch_version_local.cmake.in scripts/cmake_patch_version_local.cmake)
        endif ()
    endif()
endif ()

include (scripts/cmake_patch_version_local.cmake OPTIONAL)
