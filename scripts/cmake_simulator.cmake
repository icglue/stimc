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

    execute_process (
        COMMAND ${CDSROOT}
        OUTPUT_VARIABLE CDS_PATH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    cmake_path (APPEND CDS_PATH tools include OUTPUT_VARIABLE CDS_INC)

    find_path (
        INC_VPI_USER vpi_user.h REQUIRED
        HINTS ${CDS_INC}
    )
else ()
    message (FATAL_ERROR "unknown or invalid simulator specified: ${SIMULATOR}")
endif ()
