set (
    UNCRUSTIFY_FILES_C
    logging.c
    logging.h
    stimc-export.c
)
set (
    UNCRUSTIFY_FILES_CXX
    stimc_sc_compat.h
)

foreach (file ${UNCRUSTIFY_FILES_C})
    add_uncrustify_file (${file} C)
endforeach ()
foreach (file ${UNCRUSTIFY_FILES_CXX})
    add_uncrustify_file (${file} CPP)
endforeach ()
