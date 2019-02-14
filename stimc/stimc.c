#include "stimc.h"

#include <vpi_user.h>

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include <assert.h>

#include <pcl.h>

#ifndef SOCC_THREAD_STACK_SIZE
/* default stack size */
#define SOCC_THREAD_STACK_SIZE 65536
#endif

static const char *stimc_get_caller_scope (void)
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

void stimc_module_init (struct stimc_module *m)
{
    assert (m);
    const char *scope = stimc_get_caller_scope ();

    m->scope = (char *) malloc (sizeof (char) * (strlen (scope) + 1));
    strcpy (m->scope, scope);
}

vpiHandle stimc_pin_init (struct stimc_module *m, const char *name)
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

coroutine_t stimc_current_thread = NULL;

static PLI_INT32 stimc_callback_wrapper (struct t_cb_data *cb_data) {
    fprintf (stderr, "DEBUG: callback wrapper\n");
    coroutine_t *thread = (coroutine_t) cb_data->user_data;
    assert (thread);

    fprintf (stderr, "DEBUG: cbw - running thread\n");
    stimc_current_thread = thread;
    co_call (thread);
    stimc_current_thread = NULL;
    fprintf (stderr, "DEBUG: cbw - thread paused\n");

    return 0;
}

void stimc_register_startup_task (void (*task) (void *userdata), void *userdata)
{
    fprintf (stderr, "DEBUG: stimc_register_startup_task\n");
    s_cb_data   data;
    s_vpi_time  data_time;
    s_vpi_value data_value;

    coroutine_t thread = co_create (task, userdata, NULL, SOCC_THREAD_STACK_SIZE);
    assert (thread);

    data.reason        = cbAfterDelay;
    data.cb_rtn        = stimc_callback_wrapper;
    data.obj           = NULL;
    data.time          = &data_time;
    data.time->type    = vpiSimTime;
    data.time->high    = 0;
    data.time->low     = 0;
    data.time->real    = 0;
    data.value         = &data_value;
    data.value->format = vpiSuppressVal;
    data.index         = 0;
    data.user_data     = (PLI_BYTE8 *) thread;

    assert (vpi_register_cb (&data));
}

static void stimc_suspend (void)
{
    fprintf (stderr, "DEBUG: thread - suspending in wait\n");
    co_resume ();
    fprintf (stderr, "DEBUG: thread - returning from wait\n");
}

void stimc_wait_time (double time)
{
    // thread data ...
    coroutine_t *thread = stimc_current_thread;
    assert (thread);

    // time ...
    int timeunit_raw = vpi_get (vpiTimeUnit, NULL);
    double timeunit = timeunit_raw;
    time *= pow (10, -timeunit);
    fprintf (stderr, "DEBUG: waittime is %f * 10^%d s\n", time, timeunit_raw);
    uint64_t ltime = time;
    uint64_t ltime_h = ltime >> 32;
    uint64_t ltime_l = ltime & 0xffffffff;

    // add callback ...
    s_cb_data   data;
    s_vpi_time  data_time;
    s_vpi_value data_value;

    data.reason        = cbAfterDelay;
    data.cb_rtn        = stimc_callback_wrapper;
    data.obj           = NULL;
    data.time          = &data_time;
    data.time->type    = vpiSimTime;
    data.time->high    = ltime_h;
    data.time->low     = ltime_l;
    data.time->real    = time;
    data.value         = &data_value;
    data.value->format = vpiSuppressVal;
    data.index         = 0;
    data.user_data     = (PLI_BYTE8 *) thread;

    assert (vpi_register_cb (&data));

    // thread handling ...
    stimc_suspend ();
    fprintf (stderr, "DEBUG: wait done\n");
}
