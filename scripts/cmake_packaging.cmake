# Packing
set (
    PACK_FORMAT "tar.xz"
    CACHE
    STRING "set pack format, one of zip, tar, tar.bz2, tar.gz, tar.xz, tar.zstd"
)
set_property (
    CACHE
    PACK_FORMAT
    PROPERTY STRINGS zip tar tar.bz2 tar.gz tar.xz tar.zstd
)

set (PACK_NAME   "stimc-${STIMC_VERSION}")
set (PACK_PREFIX "${PACK_NAME}/")
set (PACK_REF    HEAD)
set (PACK_FILE   "${PACK_NAME}.${PACK_FORMAT}")

foreach (ext IN ITEMS tar zip)
    add_custom_command (
        OUTPUT  ${PACK_NAME}.${ext}
        COMMAND git archive
            --format=${ext}
            --output=${CMAKE_CURRENT_BINARY_DIR}/${PACK_NAME}.${ext}
            --prefix=${PACK_PREFIX}scripts/
            --add-file=${CMAKE_CURRENT_BINARY_DIR}/scripts/cmake_patch_version_local.cmake
            --prefix=${PACK_PREFIX}
            ${PACK_REF}
        COMMENT "Create ${ext} archive"
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        VERBATIM
    )
endforeach ()

set (PACK_COMP_EXTENSIONS xz gz   bz2)
set (PACK_COMP_TOOLS      xz gzip bzip2)
foreach (ext tool IN ZIP_LISTS PACK_COMP_EXTENSIONS PACK_COMP_TOOLS)
    add_custom_command (
        OUTPUT  ${PACK_NAME}.tar.${ext}
        DEPENDS ${PACK_NAME}.tar
        COMMAND ${tool} ${PACK_NAME}.tar --force
        COMMENT "Create tar.${ext} archive"
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        VERBATIM
    )
endforeach ()
add_custom_command (
    OUTPUT  ${PACK_NAME}.tar.zstd
    DEPENDS ${PACK_NAME}.tar
    COMMAND zstd -10 --rm ${PACK_NAME}.tar -o ${PACK_NAME}.tar.zstd --force
    COMMENT "Create tar.zstd archive"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    VERBATIM
)

add_custom_target (
    pack
    DEPENDS ${PACK_NAME}.${PACK_FORMAT}
    COMMENT "Create archive"
)
