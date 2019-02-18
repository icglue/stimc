
/* declaration */
#define STIMC_EXPORT(module) \
    void _stimc_module_ ## module ## _register (void);

#include "stimc-export.inl"

#undef STIMC_EXPORT
#define STIMC_EXPORT(module) \
    _stimc_module_ ## module ## _register,

/* vlog startup vec */
void (*vlog_startup_routines[])(void) = {
#   include "stimc-export.inl"
    0,
};



