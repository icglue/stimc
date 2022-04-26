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

# vpi header
if (SIMULATOR STREQUAL icarus)
    find_program (ICARUSVPI iverilog-vpi REQUIRED)

    execute_process (
        COMMAND ${ICARUSVPI} --cflags
        COMMAND tr " " "\n"
        COMMAND grep "^-I"
        COMMAND sed -e "s/^-I//"
        OUTPUT_VARIABLE ICARUS_INCDIR
    )

    find_path (
        INC_VPI_USER vpi_user.h REQUIRED
        HINTS ${ICARUS_INCDIR}
        PATH_SUFFIXES iverilog
    )
elseif (SIMULATOR STREQUAL cvc)
    find_program (CVC cvc REQUIRED)

    cmake_path (GET CVC     PARENT_PATH CVC_BIN)
    cmake_path (GET CVC_BIN PARENT_PATH CVC_ROOT)
    cmake_path (APPEND CVC_BIN include OUTPUT_VARIABLE CVC_INC)

    find_path (
        INC_VPI_USER vpi_user.h REQUIRED
        HINTS ${CVC_INC}
        PATH_SUFFIXES cvc oss-cvc
    )
elseif (SIMULATOR STREQUAL xcelium OR SIMULATOR STREQUAL ncsim)
    find_program (CDSROOT NAMES xmroot ncroot REQUIRED)

    execute_process (COMMAND ${CDSROOT} OUTPUT_VARIABLE CDS_PATH)
    string (STRIP ${CDS_PATH} CDS_PATH)
    cmake_path (APPEND CDS_PATH tools include OUTPUT_VARIABLE CDS_INC)

    find_path (
        INC_VPI_USER vpi_user.h REQUIRED
        HINTS ${CDS_INC}
    )
else ()
    message (FATAL_ERROR "unknown or invalid simulator specified: ${SIMULATOR}")
endif ()

include_directories (${INC_VPI_USER})

# thread implementation
if (THREAD_IMPL STREQUAL libco-local)
    target_compile_definitions (stimc PRIVATE STIMC_THREAD_IMPL_LIBCO)
    target_include_directories (stimc PRIVATE "${CMAKE_SOURCE_DIR}/libco/src")
    target_link_libraries      (stimc libco_local)
elseif (THREAD_IMPL STREQUAL libco)
    find_package (PkgConfig)
    if (PKG_CONFIG_FOUND)
        pkg_check_modules (PC_LIBCO libco QUIET)
    endif ()

    find_library (LIB_LIBCO co      REQUIRED HINTS ${PC_LIBCO_LIBRARY_DIRS})
    find_path    (INC_LIBCO libco.h REQUIRED HINTS ${PC_LIBCO_INCLUDE_DIRS})

    target_compile_definitions (stimc PRIVATE STIMC_THREAD_IMPL_LIBCO)
    target_include_directories (stimc PRIVATE ${INC_LIBCO})
    target_link_libraries      (stimc PRIVATE ${LIB_LIBCO})
elseif (THREAD_IMPL STREQUAL pcl)
    find_package (PkgConfig)
    if (PKG_CONFIG_FOUND)
        pkg_check_modules (PC_PCL pcl QUIET)
    endif ()

    find_library (LIB_PCL pcl   REQUIRED HINTS ${PC_PCL_LIBRARY_DIRS})
    find_path    (INC_PCL pcl.h REQUIRED HINTS ${PC_PCL_INCLUDE_DIRS})

    target_compile_definitions (stimc PRIVATE STIMC_THREAD_IMPL_PCL)
    target_include_directories (stimc PRIVATE ${INC_PCL})
    target_link_libraries      (stimc PRIVATE ${LIB_PCL})
elseif (THREAD_IMPL STREQUAL boost1)
    find_path (INC_BOOST_CO boost/coroutine/asymmetric_coroutine.hpp REQUIRED)

    target_compile_definitions (stimc PRIVATE STIMC_THREAD_IMPL_BOOST1)
    target_include_directories (stimc PRIVATE ${INC_BOOST_CO})
    list (APPEND STIMC_SOURCES ${STIMC_SOURCES_BOOST})
elseif (THREAD_IMPL STREQUAL boost2)
    find_path (INC_BOOST_CO boost/coroutine2/coroutine.hpp REQUIRED)

    target_compile_definitions (stimc PRIVATE STIMC_THREAD_IMPL_BOOST2)
    target_include_directories (stimc PRIVATE ${INC_BOOST_CO})
    list (APPEND STIMC_SOURCES ${STIMC_SOURCES_BOOST})
else ()
    message (FATAL_ERROR "unknown or invalid thread implementation: ${THREAD_IMPL}")
endif ()

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
