#include "socc.h"
#include <vpi_user.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

const char *socc_get_caller_scope (void)
{
    vpiHandle taskref      = vpi_handle(vpiSysTfCall, NULL);
    assert (taskref);
    vpiHandle taskscope    = vpi_handle(vpiScope, taskref);
    assert (taskscope);
    const char *scope_name = vpi_get_str(vpiFullName, taskscope);
    assert (scope_name);

    fprintf (stderr, "DEBUG: scope of \"%s\"\n", scope_name);

    return scope_name;
}

void socc_module_init (struct socc_module *m)
{
    assert (m);
    const char *scope = socc_get_caller_scope ();

    m->scope = (char *) malloc (sizeof (char) * (strlen (scope) + 1));
    strcpy (m->scope, scope);
}

vpiHandle socc_pin_init (struct socc_module *m, const char *name)
{
    const char *scope = m->scope;

    size_t scope_len = strlen (scope);
    size_t name_len  = strlen (name);

    char *pin_name = (char *) malloc (sizeof (char) * (scope_len + name_len + 2));

    strcpy (pin_name, scope);
    pin_name[scope_len] = '.';
    strcpy (&(pin_name[scope_len+1]), name);

    fprintf (stderr, "DEBUG: pin_init of \"%s\"\n", pin_name);

    vpiHandle pin = vpi_handle_by_name(pin_name, NULL);

    free (pin_name);

    assert (pin);

    return pin;
}
