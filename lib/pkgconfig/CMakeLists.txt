set (pcfile stimc.pc)

string (JOIN " " PC_REQUIRES  ${STIMC_PC_REQUIRES})
string (JOIN " " PC_PRIV_INCS ${STIMC_PC_INCFLAGS})
string (JOIN " " PC_PRIV_LIBS ${STIMC_PC_LIBFLAGS})

set (PC_REQUIRES  " ${PC_REQUIRES}")
set (PC_PRIV_INCS " ${PC_PRIV_INCS}")
set (PC_PRIV_LIBS " ${PC_PRIV_LIBS}")

configure_file (
    ${pcfile}.in
    ${pcfile}
    @ONLY
)

install (
    FILES ${CMAKE_CURRENT_BINARY_DIR}/${pcfile}
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

