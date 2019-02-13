#include "socc.h"
#include <vpi_user.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>

static const char *socc_get_caller_scope (void)
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


struct socc_callback_data {
    void (*task) (void *userdata);
    void *userdata;
};

static PLI_INT32 socc_callback_wrapper (struct t_cb_data *cb_data) {
    fprintf (stderr, "DEBUG: callback wrapper\n");
    struct socc_callback_data *scd = (struct socc_callback_data *) cb_data->user_data;

    scd->task (scd->userdata);

    return 0;
}

void socc_register_startup_task (void (*task) (void *userdata), void *userdata)
{
    fprintf (stderr, "DEBUG: socc_register_startup_task\n");
    s_cb_data   data;
    s_vpi_time  data_time;
    s_vpi_value data_value;

    struct socc_callback_data *scd = (struct socc_callback_data *) malloc (sizeof (struct socc_callback_data));
    scd->task     = task;
    scd->userdata = userdata;

    //data.reason        = cbStartOfSimulation;
    data.reason        = cbAfterDelay;
    data.cb_rtn        = socc_callback_wrapper;
    data.obj           = NULL;
    data.time          = &data_time;
    data.time->type    = vpiSimTime;
    data.time->high    = 0;
    data.time->low     = 0;
    data.time->real    = 0;
    data.value         = &data_value;
    data.value->format = vpiSuppressVal;
    data.index         = 0;
    data.user_data     = (PLI_BYTE8 *) scd;

    assert (vpi_register_cb (&data));
}

void socc_wait_time (double time)
{
    int timeunit_raw = vpi_get (vpiTimeUnit, NULL);
    double timeunit = timeunit_raw;
    time *= pow (10, -timeunit);
    fprintf (stderr, "DEBUG: waittime is %f * 10^%d s\n", time, timeunit_raw);
}
