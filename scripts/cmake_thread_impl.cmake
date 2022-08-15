# thread implementation
if (THREAD_IMPL STREQUAL libco-local)
    set (STIMC_THREAD_IMPL_LIBCO TRUE)

    target_include_directories (stimc PRIVATE "${CMAKE_SOURCE_DIR}/libco/src")
    target_link_libraries      (stimc libco_local)
elseif (THREAD_IMPL STREQUAL libco)
    find_package (PkgConfig)
    if (PKG_CONFIG_FOUND)
        pkg_check_modules (PC_LIBCO libco QUIET)
    endif ()

    find_library (LIB_LIBCO co      REQUIRED HINTS ${PC_LIBCO_LIBRARY_DIRS})
    find_path    (INC_LIBCO libco.h REQUIRED HINTS ${PC_LIBCO_INCLUDE_DIRS})

    set (STIMC_THREAD_IMPL_LIBCO TRUE)

    target_include_directories (stimc PRIVATE ${INC_LIBCO})
    target_link_libraries      (stimc PRIVATE ${LIB_LIBCO})

    if (PC_LIBCO_FOUND)
        list (APPEND STIMC_PC_REQUIRES libco)
    else ()
        cmake_path (GET LIB_LIBCO PARENT_PATH lib_libco_dir)
        list (APPEND STIMC_PC_LIBFLAGS "-L${lib_libco_dir} -lco")
    endif ()
elseif (THREAD_IMPL STREQUAL pcl)
    find_package (PkgConfig)
    if (PKG_CONFIG_FOUND)
        pkg_check_modules (PC_PCL pcl QUIET)
    endif ()

    find_library (LIB_PCL pcl   REQUIRED HINTS ${PC_PCL_LIBRARY_DIRS})
    find_path    (INC_PCL pcl.h REQUIRED HINTS ${PC_PCL_INCLUDE_DIRS})

    set (STIMC_THREAD_IMPL_PCL TRUE)

    target_include_directories (stimc PRIVATE ${INC_PCL})
    target_link_libraries      (stimc PRIVATE ${LIB_PCL})

    if (PC_PCL_FOUND)
        list (APPEND STIMC_PC_REQUIRES pcl)
    else ()
        cmake_path (GET LIB_PCL PARENT_PATH lib_pcl_dir)
        list (APPEND STIMC_PC_LIBFLAGS "-L${lib_pcl_dir} -lpcl")
    endif ()
elseif (THREAD_IMPL STREQUAL boost1)
    find_path    (INC_BOOST_CO boost/coroutine/asymmetric_coroutine.hpp REQUIRED)
    find_library (LIB_BOOST_CO boost_coroutine REQUIRED)

    set (STIMC_THREAD_IMPL_BOOST1 TRUE)

    target_include_directories (stimc PRIVATE ${INC_BOOST_CO})
    target_link_libraries      (stimc PRIVATE ${LIB_BOOST_CO})

    list (APPEND STIMC_SOURCES ${STIMC_SOURCES_BOOST})
elseif (THREAD_IMPL STREQUAL boost2)
    find_path    (INC_BOOST_CO boost/coroutine2/coroutine.hpp REQUIRED)
    find_library (LIB_BOOST_CO boost_context REQUIRED)

    set (STIMC_THREAD_IMPL_BOOST2 TRUE)

    target_include_directories (stimc PRIVATE ${INC_BOOST_CO})
    target_link_libraries      (stimc PRIVATE ${LIB_BOOST_CO})

    list (APPEND STIMC_SOURCES ${STIMC_SOURCES_BOOST})
else ()
    message (FATAL_ERROR "unknown or invalid thread implementation: ${THREAD_IMPL}")
endif ()
