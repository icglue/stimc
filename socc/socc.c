#include "socc.h"
#include <vpi_user.h>

const char *get_caller_scope ()
{
    vpiHandle taskref      = vpi_handle(vpiSysTfCall, NULL);
    vpiHandle taskscope    = vpi_handle(vpiScope, taskref);
    const char *scope_name = vpi_get_str(vpiFullName, taskscope);

    return scope_name;
}
