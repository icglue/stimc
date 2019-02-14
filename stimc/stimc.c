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

struct stimc_method_wrap {
    void (*methodfunc) (void *userdata);
    void *userdata;
};

static void stimc_edge_method_callback_wrapper (struct t_cb_data *cb_data, bool edge) {
    struct stimc_method_wrap *wrap = (struct stimc_method_wrap *) cb_data->user_data;

    // correct edge?
    if (edge ? (cb_data->value->value.scalar == vpi1) : (cb_data->value->value.scalar == vpi0)) {
        wrap->methodfunc (wrap->userdata);
    }
}

static PLI_INT32 stimc_posedge_method_callback_wrapper (struct t_cb_data *cb_data) {
    stimc_edge_method_callback_wrapper (cb_data, true);
    return 0;
}
static PLI_INT32 stimc_negedge_method_callback_wrapper (struct t_cb_data *cb_data) {
    stimc_edge_method_callback_wrapper (cb_data, false);
    return 0;
}

static void stimc_register_edge_method (void (*methodfunc) (void *userdata), void *userdata, vpiHandle net, bool edge)
{
    s_cb_data   data;
    s_vpi_time  data_time;
    s_vpi_value data_value;

    struct stimc_method_wrap *wrap = (struct stimc_method_wrap *) malloc (sizeof (struct stimc_method_wrap));
    wrap->methodfunc = methodfunc;
    wrap->userdata   = userdata;

    data.reason        = cbValueChange;
    data.cb_rtn        = (edge ? stimc_posedge_method_callback_wrapper : stimc_negedge_method_callback_wrapper);
    data.obj           = net;
    data.time          = &data_time;
    data.time->type    = vpiSuppressTime;
    data.time->high    = 0;
    data.time->low     = 0;
    data.time->real    = 0;
    data.value         = &data_value;
    data.value->format = vpiScalarVal;
    data.index         = 0;
    data.user_data     = (PLI_BYTE8 *) wrap;

    assert (vpi_register_cb (&data));
}

void stimc_register_posedge_method (void (*methodfunc) (void *userdata), void *userdata, vpiHandle net)
{
    stimc_register_edge_method (methodfunc, userdata, net, true);
}
void stimc_register_negedge_method (void (*methodfunc) (void *userdata), void *userdata, vpiHandle net)
{
    stimc_register_edge_method (methodfunc, userdata, net, false);
}


coroutine_t stimc_current_thread = NULL;

static PLI_INT32 stimc_thread_callback_wrapper (struct t_cb_data *cb_data) {
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

void stimc_register_startup_thread (void (*threadfunc) (void *userdata), void *userdata)
{
    fprintf (stderr, "DEBUG: stimc_register_startup_thread\n");
    s_cb_data   data;
    s_vpi_time  data_time;
    s_vpi_value data_value;

    coroutine_t thread = co_create (threadfunc, userdata, NULL, SOCC_THREAD_STACK_SIZE);
    assert (thread);

    data.reason        = cbAfterDelay;
    data.cb_rtn        = stimc_thread_callback_wrapper;
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
    data.cb_rtn        = stimc_thread_callback_wrapper;
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

double stimc_time (void)
{
    // get time
    s_vpi_time time;
    time.type = vpiSimTime;
    vpi_get_time (NULL, &time);

    uint64_t ltime_h = time.high;
    uint64_t ltime_l = time.low;
    uint64_t ltime   = ((ltime_h << 32) | ltime_l);

    // timeunit
    int timeunit_raw = vpi_get (vpiTimeUnit, NULL);

    double dtime = ltime;
    double dunit = timeunit_raw;
    dtime *= pow (10, dunit);

    return dtime;
}

struct stimc_event_s {
    size_t threads_len;
    size_t threads_num;
    volatile bool active;
    coroutine_t *threads;
    coroutine_t *threads_shadow;
};

stimc_event stimc_event_create (void)
{
    stimc_event event = (stimc_event) malloc (sizeof (struct stimc_event_s));

    event->active         = false;
    event->threads_len    = 16;
    event->threads_num    = 0;
    event->threads        = (coroutine_t *) malloc (sizeof (coroutine_t) * (event->threads_len));
    event->threads_shadow = (coroutine_t *) malloc (sizeof (coroutine_t) * (event->threads_len));
    event->threads[0]     = NULL;

    return event;
}

void stimc_wait_event (stimc_event event)
{
    // size check
    if (event->threads_num + 1 >= event->threads_len) {
        assert (event->active == false);
        event->threads_len   *= 2;
        event->threads        = (coroutine_t *) realloc (event->threads,        event->threads_len);
        event->threads_shadow = (coroutine_t *) realloc (event->threads_shadow, event->threads_len);
    }

    // thread data ...
    coroutine_t *thread = stimc_current_thread;
    assert (thread);

    event->threads[event->threads_num] = thread;
    event->threads_num++;
    event->threads[event->threads_num] = NULL;

    // thread handling ...
    stimc_suspend ();
}

void stimc_trigger_event (stimc_event event)
{
    if (event->active) return;

    // copy threads to shadow...
    for (size_t i = 0; i <= event->threads_num; i++) {
        event->threads_shadow[i] = event->threads[i];
    }

    assert (event->threads_shadow [event->threads_num] == NULL);

    event->threads[0]  = NULL;
    event->threads_num = 0;

    // execute threads...
    for (size_t i = 0; event->threads_shadow[i] != NULL; i++) {
        coroutine_t *thread = event->threads_shadow[i];
        stimc_current_thread = thread;
        co_call (thread);
    }
}


void stimc_module_init (struct stimc_module *m)
{
    assert (m);
    const char *scope = stimc_get_caller_scope ();

    m->id = (char *) malloc (sizeof (char) * (strlen (scope) + 1));
    strcpy (m->id, scope);
}

vpiHandle stimc_pin_init (struct stimc_module *m, const char *name)
{
    const char *scope = m->id;

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

