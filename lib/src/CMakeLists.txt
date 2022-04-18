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
    find_library (LIB_LIBCO co      REQUIRED)
    find_path    (INC_LIBCO libco.h REQUIRED)

    target_compile_definitions (stimc PRIVATE STIMC_THREAD_IMPL_LIBCO)
    target_include_directories (stimc PRIVATE ${INC_LIBCO})
    target_link_libraries      (stimc PRIVATE ${LIB_LIBCO})
elseif (THREAD_IMPL STREQUAL pcl)
    find_library (LIB_PCL pcl   REQUIRED)
    find_path    (INC_PCL pcl.h REQUIRED)

    target_compile_definitions (stimc PRIVATE STIMC_THREAD_IMPL_PCL)
    target_include_directories (stimc PRIVATE ${INC_PCL})
    target_link_libraries      (stimc PRIVATE ${LIB_PCL})
elseif (THREAD_IMPL STREQUAL boost1)
    find_path (INC_BOOST_CO boost/coroutine/asymmetric_coroutine.hpp REQUIRED)

    target_compile_definitions (stimc PRIVATE STIMC_THREAD_IMPL_BOOST1)
    target_include_directories (stimc PRIVATE ${INC_BOOST_CO})
    list (APPEND STIMC_SOURCES stimc_thread_boost.cpp)
elseif (THREAD_IMPL STREQUAL boost2)
    find_path (INC_BOOST_CO boost/coroutine2/coroutine.hpp REQUIRED)

    target_compile_definitions (stimc PRIVATE STIMC_THREAD_IMPL_BOOST2)
    target_include_directories (stimc PRIVATE ${INC_BOOST_CO})
    list (APPEND STIMC_SOURCES stimc_thread_boost.cpp)
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
