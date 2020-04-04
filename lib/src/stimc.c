/*
 *  stimc is a lightweight verilog-vpi wrapper for stimuli generation.
 *  Copyright (C) 2019-2020  Andreas Dixius, Felix Neumärker
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

/**
 * @file
 * @brief stimc core.
 */

#include "stimc.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include <assert.h>

#include <pcl.h>

#ifndef STIMC_THREAD_STACK_SIZE
/* default stack size */
#define STIMC_THREAD_STACK_SIZE    65536
#endif

#ifndef STIMC_VALVECTOR_MAX_STATIC
#define STIMC_VALVECTOR_MAX_STATIC 8
#endif

/******************************************************************************************************/
/* internal header */
/******************************************************************************************************/

/* modules and co */
static const char *stimc_get_caller_scope (void);
static vpiHandle   stimc_module_handle_init (stimc_module *m, const char *name);

/* thread queues */
struct stimc_thread_queue_s {
    size_t       threads_len;
    size_t       threads_num;
    coroutine_t *threads;
};

struct stimc_event_s {
    struct stimc_thread_queue_s queue;
};

static void stimc_thread_queue_init (struct stimc_thread_queue_s *q);
static void stimc_thread_queue_free (struct stimc_thread_queue_s *q);
static void stimc_thread_queue_free_threads (struct stimc_thread_queue_s *q);
static void stimc_thread_queue_enqueue (struct stimc_thread_queue_s *q, coroutine_t thread);
static void stimc_thread_queue_enqueue_all (struct stimc_thread_queue_s *q, struct stimc_thread_queue_s *source);
static void stimc_thread_queue_clear (struct stimc_thread_queue_s *q);

static inline void stimc_main_queue_checksetup (void);
static void        stimc_main_queue_run_threads (void);

/* methods / callbacks */
struct stimc_callback_wrap {
    void  (*func) (void *data);
    void *data;
};

static inline void stimc_valuechange_method_callback_wrapper (struct t_cb_data *cb_data, int edge);
static PLI_INT32   stimc_posedge_method_callback_wrapper (struct t_cb_data *cb_data);
static PLI_INT32   stimc_negedge_method_callback_wrapper (struct t_cb_data *cb_data);
static PLI_INT32   stimc_change_method_callback_wrapper (struct t_cb_data *cb_data);
static void        stimc_register_valuechange_method (void (*methodfunc)(void *userdata), void *userdata, stimc_net net, int edge);
static PLI_INT32   stimc_thread_callback_wrapper (struct t_cb_data *cb_data);

/* thread suspend helper function */
static inline void stimc_suspend (void);

/* common wait function */
static void stimc_wait_time_int_exp (uint64_t time, int exp);

/* non-blocking assignment helpers */
enum nba_type {
    STIMC_NBA_UNUSED_LAST,
    STIMC_NBA_Z_ALL,
    STIMC_NBA_X_ALL,
    STIMC_NBA_VAL_ALL_INT32,
    STIMC_NBA_VAL_ALL_UINT64,
    STIMC_NBA_Z_BITS,
    STIMC_NBA_X_BITS,
    STIMC_NBA_VAL_BITS,
    STIMC_NBA_VAL_REAL,
};
struct nba_queue_entry {
    union {
        uint64_t value;
        double   real_value;
    };
    enum nba_type type;
    uint16_t      lsb;
    uint16_t      msb;
};
struct nba_data {
    vpiHandle               cb_handle;
    struct nba_queue_entry *queue;
    unsigned                queue_len;
    unsigned                queue_num;
};

static void      stimc_net_nba_queue_append (stimc_net net, struct nba_queue_entry *entry_new);
static PLI_INT32 stimc_net_nba_callback_wrapper (struct t_cb_data *cb_data);

/* common x/z setters */
static inline void stimc_net_set_xz (stimc_net net, int val);
static inline void stimc_net_set_bits_xz (stimc_net net, unsigned msb, unsigned lsb, int val);

/******************************************************************************************************/
/* global variables */
/******************************************************************************************************/
static coroutine_t stimc_current_thread = NULL;

static bool stimc_finish_pending = false;

static bool                        stimc_main_queue_setup  = false;
static struct stimc_thread_queue_s stimc_main_queue        = {0, 0, NULL};
static struct stimc_thread_queue_s stimc_main_queue_shadow = {0, 0, NULL};

/******************************************************************************************************/
/* implementation */
/******************************************************************************************************/
static const char *stimc_get_caller_scope (void)
{
    vpiHandle taskref = vpi_handle (vpiSysTfCall, NULL);

    assert (taskref);
    vpiHandle taskscope = vpi_handle (vpiScope, taskref);
    assert (taskscope);
    const char *scope_name = vpi_get_str (vpiFullName, taskscope);
    assert (scope_name);

    return scope_name;
}

static inline void stimc_valuechange_method_callback_wrapper (struct t_cb_data *cb_data, int edge)
{
    struct stimc_callback_wrap *wrap = (struct stimc_callback_wrap *)cb_data->user_data;

    /* correct edge? */
    if ((edge > 0) && (cb_data->value->value.scalar != vpi1)) {
        return;
    }
    if ((edge < 0) && (cb_data->value->value.scalar != vpi0)) {
        return;
    }

    wrap->func (wrap->data);

    stimc_main_queue_run_threads ();
}

static PLI_INT32 stimc_posedge_method_callback_wrapper (struct t_cb_data *cb_data)
{
    stimc_valuechange_method_callback_wrapper (cb_data, 1);
    return 0;
}
static PLI_INT32 stimc_negedge_method_callback_wrapper (struct t_cb_data *cb_data)
{
    stimc_valuechange_method_callback_wrapper (cb_data, -1);
    return 0;
}
static PLI_INT32 stimc_change_method_callback_wrapper (struct t_cb_data *cb_data)
{
    stimc_valuechange_method_callback_wrapper (cb_data, 0);
    return 0;
}

static void stimc_register_valuechange_method (void (*methodfunc)(void *userdata), void *userdata, stimc_net net, int edge)
{
    s_cb_data   data;
    s_vpi_time  data_time;
    s_vpi_value data_value;

    struct stimc_callback_wrap *wrap = (struct stimc_callback_wrap *)malloc (sizeof (struct stimc_callback_wrap));

    // TODO: free at end of simulation? (separate callback?)

    wrap->func = methodfunc;
    wrap->data = userdata;

    data.reason = cbValueChange;
    if (edge > 0) {
        /* posedge */
        data.cb_rtn = stimc_posedge_method_callback_wrapper;
    } else if (edge < 0) {
        /* negedge */
        data.cb_rtn = stimc_negedge_method_callback_wrapper;
    } else {
        /* value change */
        data.cb_rtn = stimc_change_method_callback_wrapper;
    }
    data.obj           = net->net;
    data.time          = &data_time;
    data.time->type    = vpiSuppressTime;
    data.time->high    = 0;
    data.time->low     = 0;
    data.time->real    = 0;
    data.value         = &data_value;
    data.value->format = vpiScalarVal;
    data.index         = 0;
    data.user_data     = (PLI_BYTE8 *)wrap;

    assert (vpi_register_cb (&data));
}

void stimc_register_posedge_method (void (*methodfunc)(void *userdata), void *userdata, stimc_net net)
{
    stimc_register_valuechange_method (methodfunc, userdata, net, 1);
}
void stimc_register_negedge_method (void (*methodfunc)(void *userdata), void *userdata, stimc_net net)
{
    stimc_register_valuechange_method (methodfunc, userdata, net, -1);
}
void stimc_register_change_method  (void (*methodfunc)(void *userdata), void *userdata, stimc_net net)
{
    stimc_register_valuechange_method (methodfunc, userdata, net, 0);
}


static PLI_INT32 stimc_thread_callback_wrapper (struct t_cb_data *cb_data)
{
    coroutine_t thread = (coroutine_t)cb_data->user_data;

    assert (thread);

    stimc_main_queue_checksetup ();
    stimc_thread_queue_enqueue (&stimc_main_queue, thread);

    stimc_main_queue_run_threads ();

    return 0;
}

static void stimc_thread_wrap (void *userdata)
{
    struct stimc_callback_wrap *wrap = (struct stimc_callback_wrap *)userdata;

    wrap->func (wrap->data);

    free (wrap);

    co_exit ();
}

void stimc_register_startup_thread (void (*threadfunc)(void *userdata), void *userdata)
{
    s_cb_data   data;
    s_vpi_time  data_time;
    s_vpi_value data_value;

    struct stimc_callback_wrap *wrap = (struct stimc_callback_wrap *)malloc (sizeof (struct stimc_callback_wrap));

    wrap->func = threadfunc;
    wrap->data = userdata;

    coroutine_t thread = co_create (stimc_thread_wrap, wrap, NULL, STIMC_THREAD_STACK_SIZE);

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
    data.user_data     = (PLI_BYTE8 *)thread;

    assert (vpi_register_cb (&data));
}

static inline void stimc_suspend (void)
{
    stimc_thread_fence ();
    co_resume ();
    stimc_thread_fence ();
}

static void stimc_wait_time_int_exp (uint64_t time, int exp)
{
    /* thread data ... */
    coroutine_t thread = stimc_current_thread;

    assert (thread);

    /* time ... */
    uint64_t ltime        = time;
    int      timeunit_raw = vpi_get (vpiTimeUnit, NULL);
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
    data.user_data     = (PLI_BYTE8 *)thread;

    assert (vpi_register_cb (&data));

    // TODO: callback to delete thread + callback in case simulation terminates?

    /* thread handling ... */
    stimc_suspend ();
}

void stimc_wait_time (uint64_t time, enum stimc_time_unit exp)
{
    stimc_wait_time_int_exp (time, (int)exp);
}

void stimc_wait_time_seconds (double time)
{
    /* time ... */
    int    timeunit_raw = vpi_get (vpiTimeUnit, NULL);
    double timeunit     = timeunit_raw;

    time *= pow (10, -timeunit);
    uint64_t ltime = time;
    stimc_wait_time (ltime, timeunit_raw);
}

uint64_t stimc_time (enum stimc_time_unit exp)
{
    /* get time */
    s_vpi_time time;
    int        exp_int = (int)exp;

    time.type = vpiSimTime;
    vpi_get_time (NULL, &time);

    uint64_t ltime_h = time.high;
    uint64_t ltime_l = time.low;
    uint64_t ltime   = ((ltime_h << 32) | ltime_l);

    /* timeunit */
    int timeunit_raw = vpi_get (vpiTimeUnit, NULL);

    while (exp_int < timeunit_raw) {
        ltime *= 10;
        exp_int++;
    }
    while (exp_int > timeunit_raw) {
        ltime /= 10;
        exp_int--;
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

static void stimc_thread_queue_init (struct stimc_thread_queue_s *q)
{
    q->threads_len = 16;
    q->threads     = (coroutine_t *)malloc (sizeof (coroutine_t) * (q->threads_len));

    stimc_thread_queue_clear (q);
}

static void stimc_thread_queue_free (struct stimc_thread_queue_s *q)
{
    q->threads_len = 0;
    q->threads_num = 0;
    free (q->threads);

    q->threads = NULL;
}

static void stimc_thread_queue_free_threads (struct stimc_thread_queue_s *q)
{
    for (size_t i = 0; i < q->threads_num; i++) {
        coroutine_t c = q->threads[i];

        co_delete (c);
    }
}

static void stimc_thread_queue_enqueue (struct stimc_thread_queue_s *q, coroutine_t thread)
{
    /* size check */
    if (q->threads_num + 1 >= q->threads_len) {
        if (q->threads_len <= 0) q->threads_len = 1;

        q->threads_len *= 2;
        q->threads      = (coroutine_t *)realloc (q->threads, sizeof (coroutine_t) * (q->threads_len));
    }

    /* thread data ... */
    q->threads[q->threads_num] = thread;
    q->threads_num++;
    q->threads[q->threads_num] = NULL;
}

static void stimc_thread_queue_enqueue_all (struct stimc_thread_queue_s *q, struct stimc_thread_queue_s *source)
{
    /* size check */
    if (q->threads_num + source->threads_num >= q->threads_len) {
        if (q->threads_len <= 0) q->threads_len = 1;

        while (q->threads_num + source->threads_num + 1 >= q->threads_len) {
            q->threads_len *= 2;
        }
        q->threads = (coroutine_t *)realloc (q->threads, sizeof (coroutine_t) * q->threads_len);
    }

    /* thread data ... */
    for (size_t i = 0; i < source->threads_num; i++) {
        q->threads[q->threads_num] = source->threads[i];
        q->threads_num++;
    }

    q->threads[q->threads_num] = NULL;
}

static void stimc_thread_queue_clear (struct stimc_thread_queue_s *q)
{
    q->threads_num = 0;
    q->threads[0]  = NULL;
}

static inline void stimc_main_queue_checksetup ()
{
    if (stimc_main_queue_setup) return;

    // TODO: free at end of simulation?
    stimc_thread_queue_init (&stimc_main_queue);
    stimc_thread_queue_init (&stimc_main_queue_shadow);

    stimc_main_queue_setup = true;
}

static void stimc_main_queue_run_threads ()
{
    stimc_main_queue_checksetup ();

    while (true) {
        if (stimc_main_queue.threads_num == 0) break;

        stimc_thread_queue_enqueue_all (&stimc_main_queue_shadow, &stimc_main_queue);
        stimc_thread_queue_clear (&stimc_main_queue);

        /* execute threads... */
        assert (stimc_current_thread == NULL);

        for (size_t i = 0; stimc_main_queue_shadow.threads[i] != NULL; i++) {
            coroutine_t thread = stimc_main_queue_shadow.threads[i];
            stimc_current_thread = thread;

            stimc_thread_fence ();
            co_call (thread);
            stimc_thread_fence ();

            if (stimc_finish_pending) {
                stimc_finish_pending = false;
                vpi_control (vpiFinish, 0);
            }
        }

        stimc_current_thread = NULL;

        stimc_thread_queue_clear (&stimc_main_queue_shadow);
    }
}

stimc_event stimc_event_create (void)
{
    stimc_event event = (stimc_event)malloc (sizeof (struct stimc_event_s));

    stimc_thread_queue_init (&event->queue);

    return event;
}

void stimc_event_free (stimc_event event)
{
    stimc_thread_queue_free_threads (&event->queue);
    stimc_thread_queue_free (&event->queue);
    free (event);
}

void stimc_wait_event (stimc_event event)
{
    /* thread data ... */
    coroutine_t thread = stimc_current_thread;

    assert (thread);

    stimc_thread_queue_enqueue (&event->queue, thread);

    /* thread handling ... */
    stimc_suspend ();
}

void stimc_trigger_event (stimc_event event)
{
    if (event->queue.threads_num == 0) return;

    /* enqueue threads... */
    stimc_main_queue_checksetup ();
    stimc_thread_queue_enqueue_all (&stimc_main_queue, &event->queue);
    stimc_thread_queue_clear (&event->queue);
}

void stimc_finish (void)
{
    if (stimc_current_thread == NULL) {
        vpi_control (vpiFinish, 0);
    } else {
        stimc_finish_pending = true;
        co_exit ();
    }
}

void stimc_module_init (stimc_module *m)
{
    assert (m);
    const char *scope = stimc_get_caller_scope ();

    m->id = (char *)malloc (sizeof (char) * (strlen (scope) + 1));
    strcpy (m->id, scope);
}

void stimc_module_free (stimc_module *m)
{
    free (m->id);
}

static vpiHandle stimc_module_handle_init (stimc_module *m, const char *name)
{
    const char *scope = m->id;

    size_t scope_len = strlen (scope);
    size_t name_len  = strlen (name);

    char *net_name = (char *)malloc (sizeof (char) * (scope_len + name_len + 2));

    strcpy (net_name, scope);
    net_name[scope_len] = '.';
    strcpy (&(net_name[scope_len + 1]), name);

    vpiHandle net = vpi_handle_by_name (net_name, NULL);

    free (net_name);

    assert (net);

    return net;
}

stimc_port stimc_port_init (stimc_module *m, const char *name)
{
    vpiHandle handle = stimc_module_handle_init (m, name);

    stimc_port result = (stimc_port)malloc (sizeof (struct stimc_net_s));

    result->net = handle;
    result->nba = NULL;

    return result;
}

void stimc_port_free (stimc_port p)
{
    if (p->nba != NULL) {
        if (p->nba->cb_handle != NULL) {
            vpi_remove_cb (p->nba->cb_handle);
        }

        free (p->nba->queue);

        free (p->nba);
    }

    free (p);
}

stimc_parameter stimc_parameter_init (stimc_module *m, const char *name)
{
    return stimc_module_handle_init (m, name);
}

void stimc_parameter_free (stimc_parameter p __attribute__((unused)))
{
    /* nothing to do, yet*/
}

static void stimc_net_nba_queue_append (stimc_net net, struct nba_queue_entry *entry_new)
{
    /* init queue if necessary */
    struct nba_data *nba = net->nba;

    if (nba == NULL) {
        /* allocate */
        nba = (struct nba_data *)malloc (sizeof (struct nba_data));

        nba->queue         = (struct nba_queue_entry *)malloc (4 * sizeof (struct nba_queue_entry));
        nba->queue_len     = 4;
        nba->queue_num     = 0;
        nba->queue[0].type = STIMC_NBA_UNUSED_LAST;

        nba->cb_handle = NULL;

        net->nba = nba;
    } else {
        /* resize if necessary */
        if (nba->queue_num + 1 >= nba->queue_len) {
            nba->queue_len *= 2;
            nba->queue      = (struct nba_queue_entry *)realloc (nba->queue, nba->queue_len * sizeof (struct nba_queue_entry));
        }
    }

    /* add new entry */
    nba->queue[nba->queue_num] = *entry_new;
    nba->queue_num++;
    nba->queue[nba->queue_num].type = STIMC_NBA_UNUSED_LAST;

    /* add handler, if not yet created */
    if (nba->cb_handle != NULL) return;

    /* new callback */
    s_cb_data   cb_data;
    s_vpi_time  cb_data_time;
    s_vpi_value cb_data_value;

    cb_data.reason        = cbReadWriteSynch;
    cb_data.cb_rtn        = stimc_net_nba_callback_wrapper;
    cb_data.obj           = NULL;
    cb_data.time          = &cb_data_time;
    cb_data.time->type    = vpiSimTime;
    cb_data.time->high    = 0;
    cb_data.time->low     = 0;
    cb_data.time->real    = 0;
    cb_data.value         = &cb_data_value;
    cb_data.value->format = vpiSuppressVal;
    cb_data.index         = 0;
    cb_data.user_data     = (PLI_BYTE8 *)net;

    nba->cb_handle = vpi_register_cb (&cb_data);
    assert (nba->cb_handle);
}

static PLI_INT32 stimc_net_nba_callback_wrapper (struct t_cb_data *cb_data)
{
    stimc_net net = (stimc_net)cb_data->user_data;

    for (struct nba_queue_entry *e = net->nba->queue; e->type != STIMC_NBA_UNUSED_LAST; e++) {
        switch (e->type) {
            case STIMC_NBA_Z_ALL:
                stimc_net_set_z (net);
                break;
            case STIMC_NBA_X_ALL:
                stimc_net_set_x (net);
                break;
            case STIMC_NBA_VAL_ALL_INT32:
                stimc_net_set_int32 (net, e->value);
                break;
            case STIMC_NBA_VAL_ALL_UINT64:
                stimc_net_set_uint64 (net, e->value);
                break;
            case STIMC_NBA_Z_BITS:
                stimc_net_set_bits_z (net, e->msb, e->lsb);
                break;
            case STIMC_NBA_X_BITS:
                stimc_net_set_bits_x (net, e->msb, e->lsb);
                break;
            case STIMC_NBA_VAL_BITS:
                stimc_net_set_bits_uint64 (net, e->msb, e->lsb, e->value);
                break;
            case STIMC_NBA_VAL_REAL:
                stimc_net_set_double (net, e->real_value);
                break;
            case STIMC_NBA_UNUSED_LAST:
                /* prevent compiler warning warning */
                break;
        }
    }

    vpi_remove_cb (net->nba->cb_handle);

    net->nba->cb_handle     = NULL;
    net->nba->queue_num     = 0;
    net->nba->queue[0].type = STIMC_NBA_UNUSED_LAST;

    return 0;
}

static inline void stimc_net_set_xz (stimc_net net, int val)
{
    unsigned size = vpi_get (vpiSize, net->net);

    static s_vpi_value v;

    int32_t flags = vpiNoDelay;

    if (size == 1) {
        v.format       = vpiScalarVal;
        v.value.scalar = val;
        vpi_put_value (net->net, &v, NULL, flags);
        return;
    }

    unsigned vsize = ((size - 1) / 32) + 1;
    if (vsize <= STIMC_VALVECTOR_MAX_STATIC) {
        s_vpi_vecval vec[STIMC_VALVECTOR_MAX_STATIC];
        for (unsigned i = 0; i < vsize; i++) {
            vec[i].aval = (val == vpiZ ? 0x00000000 : 0xffffffff);
            vec[i].bval = 0xffffffff;
        }
        v.format       = vpiVectorVal;
        v.value.vector = &(vec[0]);
        vpi_put_value (net->net, &v, NULL, flags);
        return;
    }

    s_vpi_vecval *vec = (s_vpi_vecval *)malloc (vsize * sizeof (s_vpi_vecval));
    for (unsigned i = 0; i < vsize; i++) {
        vec[i].aval = (val == vpiZ ? 0x00000000 : 0xffffffff);
        vec[i].bval = 0xffffffff;
    }
    v.format       = vpiVectorVal;
    v.value.vector = vec;
    vpi_put_value (net->net, &v, NULL, flags);

    free (vec);
}

static inline void stimc_net_set_bits_xz (stimc_net net, unsigned msb, unsigned lsb, int val)
{
    if (msb < lsb) return;

    unsigned size = vpi_get (vpiSize, net->net);

    static s_vpi_value v;

    int32_t flags = vpiNoDelay;

    unsigned vsize = ((size - 1) / 32) + 1;

    unsigned jstart = lsb / 32;
    unsigned s0     = lsb % 32;
    unsigned jstop  = msb / 32;
    unsigned se     = msb % 32;

    v.format = vpiVectorVal;
    vpi_get_value (net->net, &v);

    for (unsigned j = jstart; (j < vsize) && (j <= jstop); j++) {
        uint32_t i_mask;

        if (j == jstop) {
            i_mask = (2 << se);
        } else {
            i_mask = 0;
        }

        if (j == jstart) {
            i_mask -= (1 << s0);
        } else {
            i_mask -= 1;
        }

        if (val == vpiZ) {
            v.value.vector[j].aval &= ~i_mask;
        } else {
            v.value.vector[j].aval |= i_mask;
        }
        v.value.vector[j].bval |= i_mask;
    }

    vpi_put_value (net->net, &v, NULL, flags);
}

void stimc_net_set_z (stimc_net net)
{
    stimc_net_set_xz (net, vpiZ);
}
void stimc_net_set_x (stimc_net net)
{
    stimc_net_set_xz (net, vpiX);
}

bool stimc_net_is_xz (stimc_net net)
{
    unsigned size = vpi_get (vpiSize, net->net);

    s_vpi_value v;

    if (size == 1) {
        v.format = vpiScalarVal;
        vpi_get_value (net->net, &v);
        if ((v.value.scalar == vpiX) || (v.value.scalar == vpiZ)) {
            return true;
        } else {
            return false;
        }
    }

    unsigned vsize = ((size - 1) / 32) + 1;
    v.format = vpiVectorVal;
    vpi_get_value (net->net, &v);
    for (unsigned i = 0; i < vsize; i++) {
        if (v.value.vector[i].bval != 0) return true;
    }
    return false;
}

void stimc_net_set_bits_z (stimc_net net, unsigned msb, unsigned lsb)
{
    stimc_net_set_bits_xz (net, msb, lsb, vpiZ);
}

void stimc_net_set_bits_x (stimc_net net, unsigned msb, unsigned lsb)
{
    stimc_net_set_bits_xz (net, msb, lsb, vpiX);
}

bool stimc_net_bits_are_xz (stimc_net net, unsigned msb, unsigned lsb)
{
    if (msb < lsb) return false;

    unsigned size = vpi_get (vpiSize, net->net);

    static s_vpi_value v;

    unsigned vsize = ((size - 1) / 32) + 1;

    unsigned jstart = lsb / 32;
    unsigned s0     = lsb % 32;
    unsigned jstop  = msb / 32;
    unsigned se     = msb % 32;

    v.format = vpiVectorVal;
    vpi_get_value (net->net, &v);

    for (unsigned j = jstart; (j < vsize) && (j <= jstop); j++) {
        uint32_t i_mask;

        if (j == jstop) {
            i_mask = (2 << se);
        } else {
            i_mask = 0;
        }

        if (j == jstart) {
            i_mask -= (1 << s0);
        } else {
            i_mask -= 1;
        }

        if ((v.value.vector[j].bval & i_mask) != 0) return true;
    }

    return false;
}

void stimc_net_set_bits_uint64 (stimc_net net, unsigned msb, unsigned lsb, uint64_t value)
{
    if (msb < lsb) return;

    unsigned size = vpi_get (vpiSize, net->net);

    static s_vpi_value v;

    int32_t flags = vpiNoDelay;

    unsigned vsize = ((size - 1) / 32) + 1;

    v.format = vpiVectorVal;
    vpi_get_value (net->net, &v);

    if ((msb - lsb) > 63) msb = lsb + 63;

    unsigned jstart = lsb / 32;
    unsigned s0     = lsb % 32;
    unsigned jstop  = msb / 32;

    uint64_t mask = (lsb == msb ? 1 : ((2 << (msb - lsb)) - 1));

    for (unsigned i = 0, j = jstart; (j < vsize) && (j <= jstop) && (i < 3); i++, j++) {
        uint64_t i_mask;
        uint64_t i_val;

        if (i == 0) {
            i_mask = mask << s0;
            i_val  = (value & mask) << s0;
        } else {
            i_mask = mask           >> (32 * i - s0);
            i_val  = (value & mask) >> (32 * i - s0);
        }

        v.value.vector[j].aval &= ~i_mask;
        v.value.vector[j].aval |=  i_val;
        v.value.vector[j].bval &= ~i_mask;
    }

    vpi_put_value (net->net, &v, NULL, flags);
}

uint64_t stimc_net_get_bits_uint64 (stimc_net net, unsigned msb, unsigned lsb)
{
    unsigned size = vpi_get (vpiSize, net->net);

    s_vpi_value v;

    unsigned vsize = ((size - 1) / 32) + 1;

    v.format = vpiVectorVal;
    vpi_get_value (net->net, &v);

    uint64_t result = 0;

    unsigned jstart = lsb / 32;
    unsigned s0     = lsb % 32;
    unsigned jstop  = msb / 32;

    for (unsigned i = 0, j = jstart; (j < vsize) && (j <= jstop) && (i < 3); i++, j++) {
        if (i == 0) {
            result |= (((uint64_t)(unsigned)v.value.vector[j].aval & ~((uint64_t)(unsigned)v.value.vector[j].bval)) >> s0);
        } else {
            result |= (((uint64_t)(unsigned)v.value.vector[j].aval & ~((uint64_t)(unsigned)v.value.vector[j].bval)) << (32 * i - s0));
        }
    }

    result &= ((uint64_t)2 << (msb - lsb)) - 1;

    return result;
}

void stimc_net_set_uint64 (stimc_net net, uint64_t value)
{
    unsigned size = vpi_get (vpiSize, net->net);

    static s_vpi_value v;

    int32_t flags = vpiNoDelay;

    if (size == 1) {
        v.format       = vpiScalarVal;
        v.value.scalar = (value ? vpi1 : vpi0);
        vpi_put_value (net->net, &v, NULL, flags);
        return;
    }

    unsigned vsize = ((size - 1) / 32) + 1;
    if (vsize <= STIMC_VALVECTOR_MAX_STATIC) {
        s_vpi_vecval vec[STIMC_VALVECTOR_MAX_STATIC];
        for (unsigned i = 0; (i < vsize) && (i < 2); i++) {
            vec[i].aval = (value >> (32 * i)) & 0xffffffff;
            vec[i].bval = 0;
        }
        for (unsigned i = 2; i < vsize; i++) {
            vec[i].aval = 0;
            vec[i].bval = 0;
        }
        v.format       = vpiVectorVal;
        v.value.vector = &(vec[0]);
        vpi_put_value (net->net, &v, NULL, flags);
        return;
    }

    s_vpi_vecval *vec = (s_vpi_vecval *)malloc (vsize * sizeof (s_vpi_vecval));
    for (unsigned i = 0; (i < vsize) && (i < 2); i++) {
        vec[i].aval = (value >> (32 * i)) & 0xffffffff;
        vec[i].bval = 0;
    }
    for (unsigned i = 2; i < vsize; i++) {
        vec[i].aval = 0;
        vec[i].bval = 0;
    }
    v.format       = vpiVectorVal;
    v.value.vector = vec;
    vpi_put_value (net->net, &v, NULL, flags);

    free (vec);
}

uint64_t stimc_net_get_uint64 (stimc_net net)
{
    unsigned size = vpi_get (vpiSize, net->net);

    s_vpi_value v;

    unsigned vsize = ((size - 1) / 32) + 1;

    v.format = vpiVectorVal;
    vpi_get_value (net->net, &v);

    uint64_t result = 0;
    for (unsigned i = 0; (i < vsize) && (i < 2); i++) {
        result |= (((uint64_t)(unsigned)v.value.vector[i].aval & ~((uint64_t)(unsigned)v.value.vector[i].bval)) << (32 * i));
    }

    return result;
}

void stimc_net_set_z_nonblock (stimc_net net)
{
    struct nba_queue_entry assign = {
        .type = STIMC_NBA_Z_ALL,
    };

    stimc_net_nba_queue_append (net, &assign);
}

void stimc_net_set_x_nonblock (stimc_net net)
{
    struct nba_queue_entry assign = {
        .type = STIMC_NBA_X_ALL,
    };

    stimc_net_nba_queue_append (net, &assign);
}

void stimc_net_set_bits_z_nonblock (stimc_net net, unsigned msb, unsigned lsb)
{
    struct nba_queue_entry assign = {
        .type = STIMC_NBA_Z_BITS,
        .msb  = msb,
        .lsb  = lsb,
    };

    stimc_net_nba_queue_append (net, &assign);
}

void stimc_net_set_bits_x_nonblock (stimc_net net, unsigned msb, unsigned lsb)
{
    struct nba_queue_entry assign = {
        .type = STIMC_NBA_X_BITS,
        .msb  = msb,
        .lsb  = lsb,
    };

    stimc_net_nba_queue_append (net, &assign);
}

void stimc_net_set_uint64_nonblock (stimc_net net, uint64_t value)
{
    struct nba_queue_entry assign = {
        .value = value,
        .type  = STIMC_NBA_VAL_ALL_UINT64,
    };

    stimc_net_nba_queue_append (net, &assign);
}

void stimc_net_set_bits_uint64_nonblock (stimc_net net, unsigned msb, unsigned lsb, uint64_t value)
{
    struct nba_queue_entry assign = {
        .value = value,
        .type  = STIMC_NBA_VAL_BITS,
        .msb   = msb,
        .lsb   = lsb,
    };

    stimc_net_nba_queue_append (net, &assign);
}

void stimc_net_set_int32_nonblock (stimc_net net, int32_t value)
{
    struct nba_queue_entry assign = {
        .value = value,
        .type  = STIMC_NBA_VAL_ALL_INT32,
    };

    stimc_net_nba_queue_append (net, &assign);
}

void stimc_net_set_double_nonblock (stimc_net net, double value)
{
    struct nba_queue_entry assign = {
        .real_value = value,
        .type       = STIMC_NBA_VAL_REAL,
    };

    stimc_net_nba_queue_append (net, &assign);
}

