# Options
set (
    SIMULATOR icarus
    CACHE
    STRING "set simulator for vpi headers and test, one of: icarus, cvc, ncsim, xcelium (default=icarus)"
)
set_property (
    CACHE
    SIMULATOR
    PROPERTY STRINGS icarus cvc ncsim xcelium
)

set (
    THREAD_IMPL libco-local
    CACHE
    STRING "set thread implementation, one of: libco-local, libco, pcl, boost1, boost2 (default=libco-local)"
)
set_property (
    CACHE
    THREAD_IMPL
    PROPERTY STRINGS libco-local libco pcl boost1 boost2
)

set (
    VALVECTOR_MAX_STATIC 0
    CACHE
    STRING "integer for maximum vpi on-stack vecval, 0 for default implementation"
)
set (
    THREAD_STACK_SIZE_DEFAULT 0
    CACHE
    STRING "default stack size for coroutines, 0 for default implementation"
)
set (
    DISABLE_CLEANUP FALSE
    CACHE
    BOOL "disable end-of-simulation resource cleanup"
)

set (
    WARN_LEVEL default
    CACHE
    STRING "set compiler warning level, one of: default, strict"
)
set_property (
    CACHE
    WARN_LEVEL
    PROPERTY STRINGS default strict
)
