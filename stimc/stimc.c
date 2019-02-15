#include "stimc.h"

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

    return scope_name;
}

struct stimc_method_wrap {
    void (*methodfunc) (void *userdata);
    void *userdata;
};

static inline void stimc_valuechange_method_callback_wrapper (struct t_cb_data *cb_data, int edge) {
    struct stimc_method_wrap *wrap = (struct stimc_method_wrap *) cb_data->user_data;

    /* correct edge? */
    if ((edge > 0) && (cb_data->value->value.scalar != vpi1)) {
        return;
    }
    if ((edge < 0) && (cb_data->value->value.scalar != vpi0)) {
        return;
    }

    wrap->methodfunc (wrap->userdata);
}

static PLI_INT32 stimc_posedge_method_callback_wrapper (struct t_cb_data *cb_data) {
    stimc_valuechange_method_callback_wrapper (cb_data, 1);
    return 0;
}
static PLI_INT32 stimc_negedge_method_callback_wrapper (struct t_cb_data *cb_data) {
    stimc_valuechange_method_callback_wrapper (cb_data, -1);
    return 0;
}
static PLI_INT32 stimc_change_method_callback_wrapper (struct t_cb_data *cb_data) {
    stimc_valuechange_method_callback_wrapper (cb_data, 0);
    return 0;
}

static void stimc_register_valuechange_method (void (*methodfunc) (void *userdata), void *userdata, vpiHandle net, int edge)
{
    s_cb_data   data;
    s_vpi_time  data_time;
    s_vpi_value data_value;

    struct stimc_method_wrap *wrap = (struct stimc_method_wrap *) malloc (sizeof (struct stimc_method_wrap));
    wrap->methodfunc = methodfunc;
    wrap->userdata   = userdata;

    data.reason        = cbValueChange;
    if (edge > 0) {
        /* posedge */
        data.cb_rtn    = stimc_posedge_method_callback_wrapper;
    } else if (edge < 0) {
        /* negedge */
        data.cb_rtn    = stimc_negedge_method_callback_wrapper;
    } else {
        /* value change */
        data.cb_rtn    = stimc_change_method_callback_wrapper;
    }
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
    stimc_register_valuechange_method (methodfunc, userdata, net, 1);
}
void stimc_register_negedge_method (void (*methodfunc) (void *userdata), void *userdata, vpiHandle net)
{
    stimc_register_valuechange_method (methodfunc, userdata, net, -1);
}
void stimc_register_change_method  (void (*methodfunc) (void *userdata), void *userdata, vpiHandle net)
{
    stimc_register_valuechange_method (methodfunc, userdata, net, 0);
}


coroutine_t stimc_current_thread = NULL;

static PLI_INT32 stimc_thread_callback_wrapper (struct t_cb_data *cb_data) {
    coroutine_t *thread = (coroutine_t) cb_data->user_data;
    assert (thread);

    stimc_current_thread = thread;
    co_call (thread);
    stimc_current_thread = NULL;

    return 0;
}

void stimc_register_startup_thread (void (*threadfunc) (void *userdata), void *userdata)
{
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
    co_resume ();
}

void stimc_wait_time (uint64_t time, int exp)
{
    /* thread data ... */
    coroutine_t *thread = stimc_current_thread;
    assert (thread);

    /* time ... */
    uint64_t ltime = time;
    int timeunit_raw = vpi_get (vpiTimeUnit, NULL);
    while (exp > timeunit_raw) {
        ltime *= 10;
        exp--;
    }
    while (exp < timeunit_raw) {
        ltime /= 10;
        exp++;
    }
    uint64_t ltime_h = ltime >> 32;
    uint64_t ltime_l = ltime & 0xffffffff;

    /* add callback ... */
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

    /* thread handling ... */
    stimc_suspend ();
}

void stimc_wait_time_seconds (double time)
{
    /* time ... */
    int timeunit_raw = vpi_get (vpiTimeUnit, NULL);
    double timeunit = timeunit_raw;
    time *= pow (10, -timeunit);
    uint64_t ltime = time;
    stimc_wait_time (ltime, timeunit_raw);
}

uint64_t stimc_time (int exp)
{
    /* get time */
    s_vpi_time time;
    time.type = vpiSimTime;
    vpi_get_time (NULL, &time);

    uint64_t ltime_h = time.high;
    uint64_t ltime_l = time.low;
    uint64_t ltime   = ((ltime_h << 32) | ltime_l);

    /* timeunit */
    int timeunit_raw = vpi_get (vpiTimeUnit, NULL);

    while (exp < timeunit_raw) {
        ltime *= 10;
        exp++;
    }
    while (exp > timeunit_raw) {
        ltime /= 10;
        exp--;
    }

    return ltime;
}

double stimc_time_seconds (void)
{
    /* get time */
    s_vpi_time time;
    time.type = vpiSimTime;
    vpi_get_time (NULL, &time);

    uint64_t ltime_h = time.high;
    uint64_t ltime_l = time.low;
    uint64_t ltime   = ((ltime_h << 32) | ltime_l);

    /* timeunit */
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
    /* size check */
    if (event->threads_num + 1 >= event->threads_len) {
        assert (event->active == false);
        event->threads_len   *= 2;
        event->threads        = (coroutine_t *) realloc (event->threads,        event->threads_len);
        event->threads_shadow = (coroutine_t *) realloc (event->threads_shadow, event->threads_len);
    }

    /* thread data ... */
    coroutine_t *thread = stimc_current_thread;
    assert (thread);

    event->threads[event->threads_num] = thread;
    event->threads_num++;
    event->threads[event->threads_num] = NULL;

    /* thread handling ... */
    stimc_suspend ();
}

void stimc_trigger_event (stimc_event event)
{
    if (event->active) return;
    if (event->threads_num == 0) return;

    /* copy threads to shadow... */
    for (size_t i = 0; i <= event->threads_num; i++) {
        event->threads_shadow[i] = event->threads[i];
    }

    assert (event->threads_shadow [event->threads_num] == NULL);

    event->threads[0]  = NULL;
    event->threads_num = 0;

    /* execute threads... */
    coroutine_t *old_thread = stimc_current_thread;
    for (size_t i = 0; event->threads_shadow[i] != NULL; i++) {
        coroutine_t *thread = event->threads_shadow[i];
        stimc_current_thread = thread;
        co_call (thread);
    }
    stimc_current_thread = old_thread;
}

void stimc_finish (void)
{
    vpi_control (vpiFinish, 0);
}

void stimc_module_init (stimc_module *m)
{
    assert (m);
    const char *scope = stimc_get_caller_scope ();

    m->id = (char *) malloc (sizeof (char) * (strlen (scope) + 1));
    strcpy (m->id, scope);
}

stimc_port stimc_port_init (stimc_module *m, const char *name)
{
    const char *scope = m->id;

    size_t scope_len = strlen (scope);
    size_t name_len  = strlen (name);

    char *pin_name = (char *) malloc (sizeof (char) * (scope_len + name_len + 2));

    strcpy (pin_name, scope);
    pin_name[scope_len] = '.';
    strcpy (&(pin_name[scope_len+1]), name);

    vpiHandle pin = vpi_handle_by_name(pin_name, NULL);

    free (pin_name);

    assert (pin);

    return pin;
}

static inline void stimc_net_set_xz (vpiHandle net, int val)
{
    unsigned size = vpi_get (vpiSize, net);

    s_vpi_value v;

    if (size == 1) {
        v.format       = vpiScalarVal;
        v.value.scalar = val;
        vpi_put_value (net, &v, NULL, vpiNoDelay);
        return;
    }

    unsigned vsize = ((size-1)/32)+1;
    if (vsize <= 8) {
        s_vpi_vecval vec[8];
        for (unsigned i = 0; i < vsize; i++) {
            vec[i].aval = (val == vpiZ ? 0x00000000 : 0xffffffff);
            vec[i].bval = 0xffffffff;
        }
        v.format       = vpiVectorVal;
        v.value.vector = &(vec[0]);
        vpi_put_value (net, &v, NULL, vpiNoDelay);
        return;
    }

    s_vpi_vecval *vec = (s_vpi_vecval*) malloc (vsize * sizeof (s_vpi_vecval));
    for (unsigned i = 0; i < vsize; i++) {
        vec[i].aval = (val == vpiZ ? 0x00000000 : 0xffffffff);
        vec[i].bval = 0xffffffff;
    }
    v.format       = vpiVectorVal;
    v.value.vector = vec;
    vpi_put_value (net, &v, NULL, vpiNoDelay);

    free (vec);
}

void stimc_net_set_z (vpiHandle net)
{
    stimc_net_set_xz (net, vpiZ);
}
void stimc_net_set_x (vpiHandle net)
{
    stimc_net_set_xz (net, vpiX);
}

bool stimc_net_is_xz (vpiHandle net)
{
    unsigned size = vpi_get (vpiSize, net);

    s_vpi_value v;

    if (size == 1) {
        v.format        = vpiScalarVal;
        vpi_get_value (net, &v);
        if ((v.value.scalar == vpiX) || (v.value.scalar == vpiZ)) {
            return true;
        } else {
            return false;
        }
    }

    unsigned vsize = ((size-1)/32)+1;
    v.format = vpiVectorVal;
    vpi_get_value (net, &v);
    for (unsigned i = 0; i < vsize; i++) {
        if (v.value.vector[i].bval != 0) return true;
    }
    return false;
}
