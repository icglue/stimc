
include (CMakePackageConfigHelpers)

configure_package_config_file (
    stimc.pc.in
    stimc.pc
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
