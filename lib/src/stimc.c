/*
 *  stimc is a lightweight verilog-vpi wrapper for stimuli generation.
 *  Copyright (C) 2019-2021  Andreas Dixius, Felix Neumärker
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
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
#include <stdbool.h>

#include <assert.h>

#ifdef __cplusplus
#include <atomic>
/**
 * @brief stimc thread fence (not sure if necessary, but used between suspend/resume of stimc threads)
 */
#define stimc_thread_fence(...) std::atomic_thread_fence (std::memory_order_acq_rel)
#else
#include <stdatomic.h>
/**
 * @brief stimc thread fence (not sure if necessary, but used between suspend/resume of stimc threads)
 */
#define stimc_thread_fence(...) __atomic_thread_fence (__ATOMIC_ACQ_REL)
#endif

#ifndef STIMC_THREAD_STACK_SIZE_DEFAULT
/* default stack size */
#define STIMC_THREAD_STACK_SIZE_DEFAULT 65536
#endif

#ifndef STIMC_VALVECTOR_MAX_STATIC
#define STIMC_VALVECTOR_MAX_STATIC 8
#endif

#ifdef STIMC_DISABLE_CLEANUP
#define STIMC_CLEANUP_ATTR __attribute__((unused))
#else
#define STIMC_CLEANUP_ATTR
#endif

/******************************************************************************************************/
/* version */
/******************************************************************************************************/
#ifndef STIMC_VERSION_MAJOR
#define STIMC_VERSION_MAJOR 1
#endif
#ifndef STIMC_VERSION_MINOR
#define STIMC_VERSION_MINOR 1
#endif
#define STIMC_VERSION_STR(s) #s

/* external constants for version information */
const unsigned stimc_version_major = STIMC_VERSION_MAJOR;
const unsigned stimc_version_minor = STIMC_VERSION_MINOR;
const char    *stimc_version       = STIMC_VERSION_STR (STIMC_VERSION_MAJOR) "." STIMC_VERSION_STR (STIMC_VERSION_MINOR);

/******************************************************************************************************/
/* internal header */
/******************************************************************************************************/

#define STIMC_USE_INTERNAL_HEADER
#include "stimc_thread.inl"
#undef STIMC_USE_INTERNAL_HEADER

/* modules and co */
static const char *stimc_get_caller_scope (void);
static vpiHandle   stimc_module_handle_init (stimc_module *m, const char *name);

static int stimc_module_register_init_cptf (PLI_BYTE8 *user_data);
static int stimc_module_register_init_cltf (PLI_BYTE8 *user_data);

/* threads */
enum stimc_thread_state {
    STIMC_THREAD_STATE_CREATED, /* newly created */
    STIMC_THREAD_STATE_RUNNING, /* actively running in thread function */
    STIMC_THREAD_STATE_STOPPED, /* suspended in thread function, keep stack */
    STIMC_THREAD_STATE_STOPPED_TO_FINISH, /* suspended in thread function, can be cleaned */
    STIMC_THREAD_STATE_FINISHED, /* left thread function, can be cleaned */
    STIMC_THREAD_STATE_CLEANUP, /* in cleanup procedure */
};
struct stimc_thread_s {
    /* thread implementation data */
    stimc_thread_impl thread;

    /* thread function + data to call initially */
    void  (*func) (void *data);
    void *data;

    /* data related to waiting for time/event */
    vpiHandle               call_handle;
    stimc_event_combination event_combination;
    bool                    timeout;

    /* thread status */
    enum stimc_thread_state state;
    bool                    resume_on_finish;

#ifndef STIMC_DISABLE_CLEANUP
    struct stimc_cleanup_entry_s *cleanup_queue;
    struct stimc_cleanup_entry_s *cleanup_self;
#endif
};

struct stimc_thread_queue_s {
    size_t                  max;
    size_t                  num;
    struct stimc_thread_s **threads;
};

static struct stimc_thread_s *stimc_thread_create (void (*threadfunc)(void *userdata), void *userdata, size_t stacksize);
static void                   stimc_thread_finish (struct stimc_thread_s *thread);

static void        stimc_thread_remove_event_handle (struct stimc_thread_s *thread, stimc_event event);
static inline bool stimc_thread_has_event_handle    (struct stimc_thread_s *thread);

static inline void stimc_thread_queue_init        (struct stimc_thread_queue_s *q);
static void        stimc_thread_queue_prepare     (struct stimc_thread_queue_s *q, size_t min_len);
static void        stimc_thread_queue_free        (struct stimc_thread_queue_s *q);
static size_t      stimc_thread_queue_enqueue     (struct stimc_thread_queue_s *q, struct stimc_thread_s *thread);
static void        stimc_thread_queue_enqueue_all (struct stimc_thread_queue_s *q, struct stimc_thread_queue_s *source);
static void        stimc_thread_queue_clear       (struct stimc_thread_queue_s *q);

static void stimc_main_queue_run_threads (void);

/* events */
struct stimc_event_s {
    struct stimc_thread_queue_s queue;
#ifndef STIMC_DISABLE_CLEANUP
    struct stimc_cleanup_entry_s *cleanup_self;
#endif
};

struct stimc_event_handle_s {
    stimc_event event;
    size_t      idx;
};

struct stimc_event_combination_s {
    bool                         any;
    size_t                       max;
    size_t                       num;
    struct stimc_event_handle_s *events;
};

static inline void stimc_event_combination_clear         (stimc_event_combination combination);
static inline void stimc_event_combination_prepare       (stimc_event_combination combination, size_t min_len);
static void        stimc_event_combination_append_handle (stimc_event_combination combination, stimc_event event, size_t idx);

static inline void stimc_event_remove_thread     (stimc_event event, size_t queue_idx);
static inline void stimc_event_enqueue_thread    (stimc_event event, struct stimc_thread_s *thread);
static void        stimc_event_thread_queue_free (stimc_event event);

/* methods / callbacks */
struct stimc_callback_wrap_s {
    void      (*func) (void *data);
    void     *data;
    vpiHandle cb_handle;
};

static inline void stimc_valuechange_method_callback_wrapper (struct t_cb_data *cb_data, int edge);
static PLI_INT32   stimc_posedge_method_callback_wrapper     (struct t_cb_data *cb_data);
static PLI_INT32   stimc_negedge_method_callback_wrapper     (struct t_cb_data *cb_data);
static PLI_INT32   stimc_change_method_callback_wrapper      (struct t_cb_data *cb_data);
static void        stimc_register_valuechange_method         (void (*methodfunc)(void *userdata), void *userdata, stimc_net net, int edge);
static PLI_INT32   stimc_thread_callback_wrapper             (struct t_cb_data *cb_data);
static void        stimc_thread_wrap                         (STIMC_THREAD_ARG_DECL);

/* thread helper function */
static inline void stimc_run     (struct stimc_thread_s *thread);
static inline void stimc_suspend (void);

/* common wait function */
static void stimc_wait_time_int_exp (uint64_t time, int exp);
static void stimc_event_combination_enqueue_thread (struct stimc_thread_s *thread, stimc_event_combination combination, bool consume);

/* non-blocking assignment helpers */
enum stimc_nba_type {
    STIMC_NBA_Z_ALL,
    STIMC_NBA_X_ALL,
    STIMC_NBA_VAL_ALL_INT32,
    STIMC_NBA_VAL_ALL_UINT64,
    STIMC_NBA_Z_BITS,
    STIMC_NBA_X_BITS,
    STIMC_NBA_VAL_BITS,
    STIMC_NBA_VAL_REAL,
};
struct stimc_nba_queue_entry_s {
    union {
        uint64_t value;
        double   real_value;
    };
    enum stimc_nba_type type;
    uint16_t            lsb;
    uint16_t            msb;
};
struct stimc_nba_data_s {
    vpiHandle                       cb_handle;
    struct stimc_nba_queue_entry_s *queue;
    size_t                          max;
    size_t                          num;
};

static void      stimc_net_nba_queue_append     (stimc_net net, struct stimc_nba_queue_entry_s *entry_new);
static PLI_INT32 stimc_net_nba_callback_wrapper (struct t_cb_data *cb_data);

/* common x/z setters */
static inline void stimc_net_set_xz      (stimc_net net, int val);
static inline void stimc_net_set_bits_xz (stimc_net net, unsigned msb, unsigned lsb, int val);

/* final cleanup */
#ifndef STIMC_DISABLE_CLEANUP
struct stimc_cleanup_entry_s {
    struct stimc_cleanup_entry_s *next;

    bool  cancel;
    void  (*cb) (void *data);
    void *data;
};

static void                                 stimc_cleanup_init         (void);
static void                                 stimc_cleanup_run          (struct stimc_cleanup_entry_s **queue);
static struct stimc_cleanup_entry_s *       stimc_cleanup_add          (void (*callback)(void *userdata), void *userdata);
static inline struct stimc_cleanup_entry_s *stimc_cleanup_add_internal (struct stimc_cleanup_entry_s *queue, void (*callback)(void *userdata), void *userdata);

struct stimc_cleanup_data_main_s {
    vpiHandle cb_finish;
    vpiHandle cb_reset;
};

static void      stimc_cleanup_internal (void *userdata);
static PLI_INT32 stimc_cleanup_callback (struct t_cb_data *cb_data);

static void stimc_cleanup_callback_wrap (void *userdata);
static void stimc_cleanup_thread        (void *userdata);
static void stimc_cleanup_event         (void *userdata);
#endif

/******************************************************************************************************/
/* global variables */
/******************************************************************************************************/
static struct stimc_thread_s *stimc_current_thread = NULL;

static bool stimc_finish_pending = false;

static struct stimc_thread_queue_s stimc_main_queue        = {0, 0, NULL};
static struct stimc_thread_queue_s stimc_main_queue_shadow = {0, 0, NULL};

#ifndef STIMC_DISABLE_CLEANUP
static struct stimc_cleanup_entry_s *stimc_cleanup_queue = NULL;
#endif

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
    struct stimc_callback_wrap_s *wrap = (struct stimc_callback_wrap_s *)cb_data->user_data;

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

    struct stimc_callback_wrap_s *wrap = (struct stimc_callback_wrap_s *)malloc (sizeof (struct stimc_callback_wrap_s));

    assert (wrap);

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

    wrap->cb_handle = vpi_register_cb (&data);
    assert (wrap->cb_handle);

#ifndef STIMC_DISABLE_CLEANUP
    stimc_cleanup_add (stimc_cleanup_callback_wrap, wrap);
#endif
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

static struct stimc_thread_s *stimc_thread_create (void (*threadfunc)(void *userdata), void *userdata, size_t stacksize)
{
    struct stimc_thread_s *thread = (struct stimc_thread_s *)malloc (sizeof (struct stimc_thread_s));

    assert (thread);

    if (stacksize == 0) stacksize = STIMC_THREAD_STACK_SIZE_DEFAULT;
    stimc_thread_impl ti = stimc_thread_impl_create (stimc_thread_wrap, stacksize);

    thread->thread = ti;
    thread->func   = threadfunc;
    thread->data   = userdata;

    thread->call_handle = NULL;
    thread->timeout     = false;

    thread->state            = STIMC_THREAD_STATE_CREATED;
    thread->resume_on_finish = false;

    thread->event_combination = stimc_event_combination_create (true);

#ifndef STIMC_DISABLE_CLEANUP
    thread->cleanup_queue = NULL;
    thread->cleanup_self  = stimc_cleanup_add (stimc_cleanup_thread, thread);
#endif

    return thread;
}

void stimc_register_thread_cleanup (void (*cleanfunc)(void *userdata) STIMC_CLEANUP_ATTR, void *userdata STIMC_CLEANUP_ATTR)
{
    assert (stimc_current_thread);

#ifndef STIMC_DISABLE_CLEANUP
    stimc_current_thread->cleanup_queue = stimc_cleanup_add_internal (stimc_current_thread->cleanup_queue, cleanfunc, userdata);
#endif
}

void stimc_thread_halt (void)
{
    assert (stimc_current_thread);

    if (stimc_current_thread->state < STIMC_THREAD_STATE_STOPPED) {
        stimc_current_thread->state = STIMC_THREAD_STATE_STOPPED;
    }

    stimc_suspend ();
}

void stimc_thread_exit (void)
{
    assert (stimc_current_thread);

    if (stimc_current_thread->state < STIMC_THREAD_STATE_STOPPED_TO_FINISH) {
        stimc_current_thread->state = STIMC_THREAD_STATE_STOPPED_TO_FINISH;
    }

    stimc_suspend ();
}

void stimc_thread_resume_on_finish (bool resume)
{
    assert (stimc_current_thread);

    stimc_current_thread->resume_on_finish = resume;
}

bool stimc_thread_is_finished (void)
{
    assert (stimc_current_thread);

    if (stimc_current_thread->state >= STIMC_THREAD_STATE_STOPPED) return true;

    return false;
}

static void stimc_thread_finish (struct stimc_thread_s *thread)
{
    assert (thread);

#ifndef STIMC_DISABLE_CLEANUP
    thread->cleanup_self->cancel = true;
#endif

    /* final resume ? */
    bool final_resume = false;
    if (thread->resume_on_finish
        && thread->state > STIMC_THREAD_STATE_CREATED
        && thread->state < STIMC_THREAD_STATE_FINISHED) {
        final_resume = true;
    }

    thread->state = STIMC_THREAD_STATE_CLEANUP;

    if (final_resume) {
        stimc_run (thread);
    }

    /* remove resume callbacks */
    if (thread->call_handle != NULL) {
        vpi_remove_cb (thread->call_handle);
    }
    for (size_t i = 0; i < thread->event_combination->num; i++) {
        struct stimc_event_handle_s *h = &(thread->event_combination->events[i]);
        stimc_event_remove_thread (h->event, h->idx);
        h->event = NULL;
    }

#ifndef STIMC_DISABLE_CLEANUP
    stimc_cleanup_run (&(thread->cleanup_queue));
#endif

    stimc_thread_impl ti = thread->thread;
    stimc_event_combination_free (thread->event_combination);
    free (thread);
    stimc_thread_impl_delete (ti);
}

static void stimc_thread_remove_event_handle (struct stimc_thread_s *thread, stimc_event event)
{
    for (size_t i = 0; i < thread->event_combination->num; i++) {
        struct stimc_event_handle_s *h = &(thread->event_combination->events[i]);

        if (h->event == event) {
            size_t idx_last = thread->event_combination->num - 1;

            if (i == idx_last) {
                thread->event_combination->num--;
            } else {
                thread->event_combination->events[i] = thread->event_combination->events[idx_last];
                thread->event_combination->num--;
            }

            break;
        }
    }
}

static inline bool stimc_thread_has_event_handle (struct stimc_thread_s *thread)
{
    return (thread->event_combination->num > 0);
}

static PLI_INT32 stimc_thread_callback_wrapper (struct t_cb_data *cb_data)
{
    struct stimc_thread_s *thread = (struct stimc_thread_s *)cb_data->user_data;

    assert (thread);

    stimc_thread_queue_enqueue (&stimc_main_queue, thread);

    if (thread->call_handle != NULL) {
        vpi_remove_cb (thread->call_handle);
        thread->call_handle = NULL;
    }
    for (size_t i = 0; i < thread->event_combination->num; i++) {
        struct stimc_event_handle_s *h = &(thread->event_combination->events[i]);
        stimc_event_remove_thread (h->event, h->idx);
        /* if event handle exists, this has to be a timeout callback */
        thread->timeout = true;
    }
    stimc_event_combination_clear (thread->event_combination);

    stimc_main_queue_run_threads ();

    return 0;
}

static void stimc_thread_wrap (STIMC_THREAD_ARG_DEF)
{
    struct stimc_thread_s *thread = stimc_current_thread;

    assert (thread);

    thread->state = STIMC_THREAD_STATE_RUNNING;
    thread->func (thread->data);

    if (thread->state < STIMC_THREAD_STATE_FINISHED) {
        thread->state = STIMC_THREAD_STATE_FINISHED;
    }
    stimc_suspend ();
}

void stimc_register_startup_thread (void (*threadfunc)(void *userdata), void *userdata, size_t stacksize)
{
    stimc_spawn_thread (threadfunc, userdata, stacksize);
}

void stimc_spawn_thread (void (*threadfunc)(void *userdata), void *userdata, size_t stacksize)
{
    struct stimc_thread_s *thread = stimc_thread_create (threadfunc, userdata, stacksize);

    assert (thread);

    stimc_thread_queue_enqueue (&stimc_main_queue, thread);
}

static inline void stimc_run (struct stimc_thread_s *thread)
{
    stimc_current_thread = thread;

    stimc_thread_fence ();
    stimc_thread_impl_run (thread->thread);
    stimc_thread_fence ();

    stimc_current_thread = NULL;

    if (thread->state >= STIMC_THREAD_STATE_STOPPED_TO_FINISH
        && thread->state < STIMC_THREAD_STATE_CLEANUP) {
        stimc_thread_finish (thread);
    }
}

static inline void stimc_suspend (void)
{
    stimc_thread_fence ();
    stimc_thread_impl_suspend ();
    stimc_thread_fence ();
}

static void stimc_wait_time_int_exp (uint64_t time, int exp)
{
    /* thread data ... */
    struct stimc_thread_s *thread = stimc_current_thread;

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

    vpiHandle wait_handle = vpi_register_cb (&data);

    assert (wait_handle);

    thread->call_handle = wait_handle;

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

    stimc_wait_time_int_exp (ltime, timeunit_raw);
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

static inline void stimc_thread_queue_init (struct stimc_thread_queue_s *q)
{
    q->max = 0;
    q->num = 0;

    q->threads = NULL;
}

static void stimc_thread_queue_prepare (struct stimc_thread_queue_s *q, size_t min_len)
{
    if (min_len <= q->max) return;

    if (q->max == 0) {
        q->max = 16;
    }
    while (min_len > q->max) {
        q->max *= 2;
        assert (q->max != 0);
    }

    q->threads = (struct stimc_thread_s **)realloc (q->threads, sizeof (struct stimc_thread_s *) * (q->max));
    assert (q->threads);
}

static void stimc_thread_queue_free (struct stimc_thread_queue_s *q)
{
    q->max = 0;
    q->num = 0;
    if (q->threads != NULL) free (q->threads);

    q->threads = NULL;
}

static size_t stimc_thread_queue_enqueue (struct stimc_thread_queue_s *q, struct stimc_thread_s *thread)
{
    stimc_thread_queue_prepare (q, q->num + 1);

    size_t result_idx = q->num;

    /* thread data ... */
    q->threads[q->num] = thread;
    q->num++;

    return result_idx;
}

static void stimc_thread_queue_enqueue_all (struct stimc_thread_queue_s *q, struct stimc_thread_queue_s *source)
{
    stimc_thread_queue_prepare (q, q->num + source->num);

    /* thread data ... */
    for (size_t i = 0; i < source->num; i++) {
        if (source->threads[i] != NULL) {
            q->threads[q->num] = source->threads[i];
            q->num++;
        }
    }
}

static void stimc_thread_queue_clear (struct stimc_thread_queue_s *q)
{
    q->num = 0;
}

static void stimc_main_queue_run_threads ()
{
    while (true) {
        if (stimc_main_queue.num == 0) break;

        stimc_thread_queue_enqueue_all (&stimc_main_queue_shadow, &stimc_main_queue);
        stimc_thread_queue_clear (&stimc_main_queue);

        /* execute threads... */
        assert (stimc_current_thread == NULL);

        for (size_t i = 0; i < stimc_main_queue_shadow.num; i++) {
            struct stimc_thread_s *thread = stimc_main_queue_shadow.threads[i];

            if (thread == NULL) continue;

            stimc_run (thread);

            if (stimc_finish_pending) {
                stimc_finish_pending = false;
                vpi_control (vpiFinish, 0);
                break;
            }
        }

        stimc_thread_queue_clear (&stimc_main_queue_shadow);
    }
}

stimc_event stimc_event_create (void)
{
    stimc_event event = (stimc_event)malloc (sizeof (struct stimc_event_s));

    assert (event);

    stimc_thread_queue_init (&event->queue);

#ifndef STIMC_DISABLE_CLEANUP
    /*
     * Important: Start without cleanup data initialized.
     *
     * It is possible to create global events via stimc++ that
     * lead to stimc_event_create calls at elaboration
     * time which then triggers stimc_cleanup_init.
     *
     * Some simulators see it as an error to register an
     * end-of-simulation callback during elaboration but
     * at the same time need stimc module init-code to be present
     * as otherwise stimc related verilog system tasks are undefined.
     *
     * This does not apply to modules or ports as they are
     * only initialised during simulation time via their
     * stimc system tasks.
     */
    event->cleanup_self = NULL;
#endif

    return event;
}

static void stimc_event_thread_queue_free (stimc_event event)
{
    /* remove handles */
    for (size_t i = 0; i < event->queue.num; i++) {
        struct stimc_thread_s *thread = event->queue.threads[i];

        if (thread == NULL) continue;

        stimc_thread_remove_event_handle (thread, event);
        if (stimc_thread_has_event_handle (thread)) continue;

        /* in case the thread can still be woken up by timeout,
         * it will be a timeout */
        if (thread->call_handle != NULL) {
            thread->timeout = true;
        }
    }

    stimc_thread_queue_free (&event->queue);
}

void stimc_event_free (stimc_event event)
{
    if (event == NULL) return;

    stimc_event_thread_queue_free (event);

#ifndef STIMC_DISABLE_CLEANUP
    if (event->cleanup_self != NULL) {
        event->cleanup_self->cancel = true;
    }
#endif

    free (event);
}

stimc_event_combination stimc_event_combination_create (bool any)
{
    stimc_event_combination combination = (stimc_event_combination)malloc (sizeof (struct stimc_event_combination_s));

    assert (combination);

    combination->any    = any;
    combination->max    = 0;
    combination->num    = 0;
    combination->events = NULL;

    return combination;
}

void stimc_event_combination_free (stimc_event_combination combination)
{
    if (combination == NULL) return;

    if (combination->events != NULL) free (combination->events);
    free (combination);
}

static inline void stimc_event_combination_clear (stimc_event_combination combination)
{
    combination->num = 0;
}

static inline void stimc_event_combination_prepare (stimc_event_combination combination, size_t min_len)
{
    if (min_len <= combination->max) return;

    if (combination->max == 0) {
        combination->max = 4;
    }
    while (min_len > combination->max) {
        combination->max *= 2;
        assert (combination->max != 0);
    }

    combination->events = (struct stimc_event_handle_s *)realloc (combination->events, sizeof (struct stimc_event_handle_s) * (combination->max));
    assert (combination->events);
}

static void stimc_event_combination_append_handle (stimc_event_combination combination, stimc_event event, size_t idx)
{
    assert (combination);
    stimc_event_combination_prepare (combination, combination->num + 1);

    combination->events[combination->num] = (struct stimc_event_handle_s) {
        .event = event,
        .idx   = idx
    };

    combination->num++;
}

void stimc_event_combination_append (stimc_event_combination combination, stimc_event event)
{
    stimc_event_combination_append_handle (combination, event, 0);
}

void stimc_event_combination_copy (stimc_event_combination dst, stimc_event_combination src)
{
    assert (dst);
    stimc_event_combination_clear (dst);
    if (src == NULL) return;

    stimc_event_combination_prepare (dst, src->max);
    dst->any = src->any;
    dst->num = src->num;

    for (size_t i = 0; i < src->num; i++) {
        dst->events[i] = src->events[i];
    }
}

static inline void stimc_event_remove_thread (stimc_event event, size_t queue_idx)
{
    assert (event);
    assert (event->queue.num > queue_idx);

    event->queue.threads[queue_idx] = NULL;
}

static inline void stimc_event_enqueue_thread (stimc_event event, struct stimc_thread_s *thread)
{
#ifndef STIMC_DISABLE_CLEANUP
    if (event->cleanup_self == NULL) {
        event->cleanup_self = stimc_cleanup_add (stimc_cleanup_event, event);
    }
#endif

    size_t event_idx = stimc_thread_queue_enqueue (&event->queue, thread);

    stimc_event_combination_append_handle (thread->event_combination, event, event_idx);
}

static void stimc_event_combination_enqueue_thread (struct stimc_thread_s *thread, stimc_event_combination combination, bool consume)
{
    assert (combination);

    stimc_event_combination_prepare (thread->event_combination, combination->max);
    stimc_event_combination_clear (thread->event_combination);

    thread->event_combination->any = combination->any;

    for (size_t i = 0; i < combination->num; i++) {
        stimc_event_enqueue_thread (combination->events[i].event, thread);
    }

    if (consume) stimc_event_combination_free (combination);
}

void stimc_wait_event (stimc_event event)
{
    struct stimc_thread_s *thread = stimc_current_thread;

    assert (thread);

    stimc_event_combination_clear (thread->event_combination);
    stimc_event_enqueue_thread (event, thread);

    /* thread handling ... */
    stimc_suspend ();
}

void stimc_wait_event_combination (const stimc_event_combination combination, bool consume)
{
    struct stimc_thread_s *thread = stimc_current_thread;

    assert (thread);

    stimc_event_combination_enqueue_thread (thread, combination, consume);

    /* thread handling ... */
    stimc_suspend ();
}

bool stimc_wait_event_timeout (stimc_event event, uint64_t time, enum stimc_time_unit exp)
{
    struct stimc_thread_s *thread = stimc_current_thread;

    assert (thread);

    thread->timeout = false;
    stimc_event_enqueue_thread (event, thread);

    stimc_wait_time (time, exp);

    return (thread->timeout);
}

bool stimc_wait_event_timeout_seconds (stimc_event event, double time)
{
    struct stimc_thread_s *thread = stimc_current_thread;

    assert (thread);

    thread->timeout = false;
    stimc_event_enqueue_thread (event, thread);

    stimc_wait_time_seconds (time);

    return (thread->timeout);
}

bool stimc_wait_event_combination_timeout (const stimc_event_combination combination, bool consume, uint64_t time, enum stimc_time_unit exp)
{
    struct stimc_thread_s *thread = stimc_current_thread;

    assert (thread);

    thread->timeout = false;
    stimc_event_combination_enqueue_thread (thread, combination, consume);

    stimc_wait_time (time, exp);

    return (thread->timeout);
}

bool stimc_wait_event_combination_timeout_seconds (const stimc_event_combination combination, bool consume, double time)
{
    struct stimc_thread_s *thread = stimc_current_thread;

    assert (thread);

    thread->timeout = false;
    stimc_event_combination_enqueue_thread (thread, combination, consume);

    stimc_wait_time_seconds (time);

    return (thread->timeout);
}

void stimc_trigger_event (stimc_event event)
{
    if (event->queue.num == 0) return;

    /* disable timeouts, remove handles */
    for (size_t i = 0; i < event->queue.num; i++) {
        struct stimc_thread_s *thread = event->queue.threads[i];

        if (thread == NULL) continue;

        stimc_thread_remove_event_handle (thread, event);

        /* was one of many to wait for? -> do not trigger */
        if ((!thread->event_combination->any) && stimc_thread_has_event_handle (thread)) {
            event->queue.threads[i] = NULL;
            continue;
        }

        /* triggered by this event -> remove others */
        for (size_t j = 0; j < thread->event_combination->num; j++) {
            struct stimc_event_handle_s *h = &(thread->event_combination->events[j]);
            stimc_event_remove_thread (h->event, h->idx);
        }
        stimc_event_combination_clear (thread->event_combination);

        /* active timeout? -> remove + result */
        if (thread->call_handle != NULL) {
            vpi_remove_cb (thread->call_handle);
            thread->call_handle = NULL;
            thread->timeout     = false;
        }
    }

    /* enqueue threads... */
    stimc_thread_queue_enqueue_all (&stimc_main_queue, &event->queue);

    stimc_thread_queue_clear (&event->queue);
}

bool stimc_wait_timed_out (void)
{
    assert (stimc_current_thread);

    return stimc_current_thread->timeout;
}

void stimc_finish (void)
{
    if (stimc_current_thread == NULL) {
        vpi_control (vpiFinish, 0);
    } else {
        stimc_finish_pending = true;
        if (stimc_current_thread->state < STIMC_THREAD_STATE_STOPPED_TO_FINISH) {
            stimc_current_thread->state = STIMC_THREAD_STATE_STOPPED_TO_FINISH;
        }
        stimc_suspend ();
    }
}

void stimc_module_init (stimc_module *m, void (*cleanfunc)(void *cleandata) STIMC_CLEANUP_ATTR, void *cleandata STIMC_CLEANUP_ATTR)
{
    assert (m);
    const char *scope = stimc_get_caller_scope ();

    m->id = (char *)malloc (sizeof (char) * (strlen (scope) + 1));
    assert (m->id);
    strcpy (m->id, scope);

#ifndef STIMC_DISABLE_CLEANUP
    if (cleanfunc != NULL) {
        stimc_cleanup_add (cleanfunc, cleandata);
    }
#endif
}

void stimc_module_free (stimc_module *m)
{
    free (m->id);
}

static int stimc_module_register_init_cptf (PLI_BYTE8 *user_data __attribute__((unused)))
{
    return 0;
}

static int stimc_module_register_init_cltf (PLI_BYTE8 *user_data)
{
    void (*initfunc)(void) = (void (*)(void)) (uintptr_t) user_data;

    initfunc ();

    stimc_main_queue_run_threads ();

    return 0;
}

void stimc_module_register (const char *name, void (*initfunc)(void))
{
    const char *task_prefix = "$stimc_";
    const char *task_suffix = "_init";

    size_t task_name_len = strlen (task_prefix) + strlen (name) + strlen (task_suffix);

    char *task_name = (char *)malloc (task_name_len + 1);

    assert (task_name);
    task_name[0] = '\0';

    strcat (task_name, task_prefix);
    strcat (task_name, name);
    strcat (task_name, task_suffix);

    s_vpi_systf_data tf_data;

    tf_data.type      = vpiSysTask;
    tf_data.tfname    = task_name;
    tf_data.calltf    = stimc_module_register_init_cltf;
    tf_data.compiletf = stimc_module_register_init_cptf;
    tf_data.sizetf    = 0;
    tf_data.user_data = (void *)(uintptr_t)initfunc;

    vpi_register_systf (&tf_data);

    free (task_name);
}

static vpiHandle stimc_module_handle_init (stimc_module *m, const char *name)
{
    const char *scope = m->id;

    size_t scope_len = strlen (scope);
    size_t name_len  = strlen (name);

    char *net_name = (char *)malloc (sizeof (char) * (scope_len + name_len + 2));

    assert (net_name);

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

    assert (result);

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

static void stimc_net_nba_queue_append (stimc_net net, struct stimc_nba_queue_entry_s *entry_new)
{
    /* init queue if necessary */
    struct stimc_nba_data_s *nba = net->nba;

    if (nba == NULL) {
        /* allocate */
        nba = (struct stimc_nba_data_s *)malloc (sizeof (struct stimc_nba_data_s));
        assert (nba);

        nba->queue = (struct stimc_nba_queue_entry_s *)malloc (4 * sizeof (struct stimc_nba_queue_entry_s));
        nba->max   = 4;
        nba->num   = 0;
        assert (nba->queue);

        nba->cb_handle = NULL;

        net->nba = nba;
    } else {
        /* resize if necessary */
        if (nba->num + 1 > nba->max) {
            nba->max  *= 2;
            nba->queue = (struct stimc_nba_queue_entry_s *)realloc (nba->queue, nba->max * sizeof (struct stimc_nba_queue_entry_s));
            assert (nba->queue);
        }
    }

    /* add new entry */
    nba->queue[nba->num] = *entry_new;
    nba->num++;

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

    for (size_t i = 0; i < net->nba->num; i++) {
        struct stimc_nba_queue_entry_s *e = &(net->nba->queue[i]);

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
        }
    }

    vpi_remove_cb (net->nba->cb_handle);

    net->nba->cb_handle = NULL;
    net->nba->num       = 0;

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

    assert (vec);
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
            /* prevent sign extension */
            result |= (((uint64_t)(unsigned)v.value.vector[j].aval & ~((uint64_t)(unsigned)v.value.vector[j].bval)) >> s0);
        } else {
            /* prevent sign extension */
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

    assert (vec);
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
        /* prevent sign extension */
        result |= (((uint64_t)(unsigned)v.value.vector[i].aval & ~((uint64_t)(unsigned)v.value.vector[i].bval)) << (32 * i));
    }

    return result;
}

void stimc_net_set_z_nonblock (stimc_net net)
{
    struct stimc_nba_queue_entry_s assign = {
        .type = STIMC_NBA_Z_ALL,
    };

    stimc_net_nba_queue_append (net, &assign);
}

void stimc_net_set_x_nonblock (stimc_net net)
{
    struct stimc_nba_queue_entry_s assign = {
        .type = STIMC_NBA_X_ALL,
    };

    stimc_net_nba_queue_append (net, &assign);
}

void stimc_net_set_bits_z_nonblock (stimc_net net, unsigned msb, unsigned lsb)
{
    struct stimc_nba_queue_entry_s assign = {
        .type = STIMC_NBA_Z_BITS,
        .msb  = msb,
        .lsb  = lsb,
    };

    stimc_net_nba_queue_append (net, &assign);
}

void stimc_net_set_bits_x_nonblock (stimc_net net, unsigned msb, unsigned lsb)
{
    struct stimc_nba_queue_entry_s assign = {
        .type = STIMC_NBA_X_BITS,
        .msb  = msb,
        .lsb  = lsb,
    };

    stimc_net_nba_queue_append (net, &assign);
}

void stimc_net_set_uint64_nonblock (stimc_net net, uint64_t value)
{
    struct stimc_nba_queue_entry_s assign = {
        .value = value,
        .type  = STIMC_NBA_VAL_ALL_UINT64,
    };

    stimc_net_nba_queue_append (net, &assign);
}

void stimc_net_set_bits_uint64_nonblock (stimc_net net, unsigned msb, unsigned lsb, uint64_t value)
{
    struct stimc_nba_queue_entry_s assign = {
        .value = value,
        .type  = STIMC_NBA_VAL_BITS,
        .msb   = msb,
        .lsb   = lsb,
    };

    stimc_net_nba_queue_append (net, &assign);
}

void stimc_net_set_int32_nonblock (stimc_net net, int32_t value)
{
    struct stimc_nba_queue_entry_s assign = {
        .value = value,
        .type  = STIMC_NBA_VAL_ALL_INT32,
    };

    stimc_net_nba_queue_append (net, &assign);
}

void stimc_net_set_double_nonblock (stimc_net net, double value)
{
    struct stimc_nba_queue_entry_s assign = {
        .real_value = value,
        .type       = STIMC_NBA_VAL_REAL,
    };

    stimc_net_nba_queue_append (net, &assign);
}


#ifndef STIMC_DISABLE_CLEANUP
static void stimc_cleanup_init (void)
{
    struct stimc_cleanup_data_main_s *c_data = (struct stimc_cleanup_data_main_s *)malloc (sizeof (struct stimc_cleanup_data_main_s));

    assert (c_data);

    s_cb_data data;

    data.reason    = cbEndOfSimulation;
    data.cb_rtn    = stimc_cleanup_callback;
    data.obj       = NULL;
    data.time      = NULL;
    data.value     = NULL;
    data.index     = 0;
    data.user_data = NULL;

    c_data->cb_finish = vpi_register_cb (&data);
    assert (c_data->cb_finish);

    data.reason    = cbStartOfReset;
    data.cb_rtn    = stimc_cleanup_callback;
    data.obj       = NULL;
    data.time      = NULL;
    data.value     = NULL;
    data.index     = 0;
    data.user_data = NULL;

    c_data->cb_reset = vpi_register_cb (&data);
    /* might not be supported -> NULL ok */
    /* assert (c_data->cb_reset); */

    stimc_cleanup_queue = stimc_cleanup_add_internal (stimc_cleanup_queue, stimc_cleanup_internal, c_data);
}

static void stimc_cleanup_run (struct stimc_cleanup_entry_s **queue)
{
    assert (queue);

    struct stimc_cleanup_entry_s *q = *queue;

    /* call all cleanup callbacks */
    for (struct stimc_cleanup_entry_s *e = q; e != NULL; e = e->next) {
        if (e->cancel) continue;

        e->cb (e->data);
    }

    /* clean list */
    while (q != NULL) {
        struct stimc_cleanup_entry_s *e = q;
        q = e->next;
        free (e);
    }

    *queue = NULL;
}

static struct stimc_cleanup_entry_s *stimc_cleanup_add (void (*callback)(void *userdata), void *userdata)
{
    if (stimc_cleanup_queue == NULL) stimc_cleanup_init ();

    stimc_cleanup_queue = stimc_cleanup_add_internal (stimc_cleanup_queue, callback, userdata);
    return stimc_cleanup_queue;
}

static inline struct stimc_cleanup_entry_s *stimc_cleanup_add_internal (struct stimc_cleanup_entry_s *queue, void (*callback)(void *userdata), void *userdata)
{
    struct stimc_cleanup_entry_s *e = (struct stimc_cleanup_entry_s *)malloc (sizeof (struct stimc_cleanup_entry_s));

    assert (e);

    e->next   = queue;
    e->cancel = false;
    e->cb     = callback;
    e->data   = userdata;

    return e;
}

static void stimc_cleanup_internal (void *userdata)
{
    struct stimc_cleanup_data_main_s *data = (struct stimc_cleanup_data_main_s *)userdata;

    /* remove callbacks */
    if (data->cb_finish != NULL) vpi_remove_cb (data->cb_finish);
    if (data->cb_reset != NULL) vpi_remove_cb (data->cb_reset);
    free (data);

    /* thread queue */
    stimc_thread_queue_free (&stimc_main_queue);
    stimc_thread_queue_free (&stimc_main_queue_shadow);

    stimc_finish_pending = false;
}

static PLI_INT32 stimc_cleanup_callback (struct t_cb_data *cb_data __attribute__((unused)))
{
    assert (stimc_current_thread == NULL);

    stimc_cleanup_run (&stimc_cleanup_queue);

    return 0;
}

static void stimc_cleanup_callback_wrap (void *userdata)
{
    struct stimc_callback_wrap_s *wrap = (struct stimc_callback_wrap_s *)userdata;

    vpi_remove_cb (wrap->cb_handle);
    free (wrap);
}

static void stimc_cleanup_thread (void *userdata)
{
    struct stimc_thread_s *thread = (struct stimc_thread_s *)userdata;

    stimc_thread_finish (thread);
}

static void stimc_cleanup_event (void *userdata)
{
    struct stimc_event_s *event = (struct stimc_event_s *)userdata;

    stimc_event_thread_queue_free (event);
    event->cleanup_self = NULL;
}
#endif

