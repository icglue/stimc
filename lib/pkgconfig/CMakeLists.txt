# include (CMakePackageConfigHelpers)

set(pcfile stimc.pc)

configure_file (
    ${pcfile}.in
    ${pcfile}
    @ONLY
)

install (
    FILES ${CMAKE_CURRENT_BINARY_DIR}/${pcfile}
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

# configure_package_config_file (
#     ${pcfile}.in
#     ${pcfile}
#     INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
# )
