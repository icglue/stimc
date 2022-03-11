/*
 *  stimc is a lightweight verilog-vpi wrapper for stimuli generation.
 *  Copyright (C) 2019-2022  Andreas Dixius, Felix Neumärker
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
 * @brief stimc thread implementation.
 */

#ifndef STIMC_THREAD_INL
#define STIMC_THREAD_INL

#ifndef STIMC_USE_INTERNAL_HEADER
#error "stimc internal header should not be included outside of stimc core"
#endif

#include <stddef.h>

/*******************************************************************************/
/* implementation variants - default: pcl */
/*******************************************************************************/
#ifdef STIMC_THREAD_IMPL_PCL
#define STIMC_THREAD_IMPL
#endif

#ifdef STIMC_THREAD_IMPL_LIBCO
#define STIMC_THREAD_IMPL
#endif

#ifdef STIMC_THREAD_IMPL_BOOST
#define STIMC_THREAD_IMPL_BOOST2
#endif

#ifdef STIMC_THREAD_IMPL_BOOST1
#define STIMC_THREAD_IMPL
#endif

#ifdef STIMC_THREAD_IMPL_BOOST2
#define STIMC_THREAD_IMPL
#endif

#ifndef STIMC_THREAD_IMPL
#define STIMC_THREAD_IMPL_PCL
#endif


/*******************************************************************************/
/* implementation properties */
/*******************************************************************************/
#ifdef STIMC_THREAD_IMPL_PCL
#define STIMC_THREAD_IMPL_INLINE
#define STIMC_THREAD_ARG_PTR
#endif

#ifdef STIMC_THREAD_IMPL_LIBCO
#define STIMC_THREAD_IMPL_INLINE
#define STIMC_THREAD_ARG_NONE
#endif

#ifdef STIMC_THREAD_IMPL_BOOST1
#define STIMC_THREAD_IMPL_EXTERNAL
#define STIMC_THREAD_ARG_NONE
#endif

#ifdef STIMC_THREAD_IMPL_BOOST2
#define STIMC_THREAD_IMPL_EXTERNAL
#define STIMC_THREAD_ARG_NONE
#endif


#ifdef STIMC_THREAD_ARG_PTR
#define STIMC_THREAD_ARG_DEF  void *_unused_thread_arg_ __attribute__((unused))
#define STIMC_THREAD_ARG_DECL void *
#endif

#ifdef STIMC_THREAD_ARG_NONE
#define STIMC_THREAD_ARG_DEF  void
#define STIMC_THREAD_ARG_DECL void
#endif


/*******************************************************************************/
/* declaration */
/*******************************************************************************/
#ifdef STIMC_THREAD_IMPL_EXTERNAL
#ifdef __cplusplus
/* *auto-indent-off* */
extern "C" {
/* *auto-indent-on* */
#endif

#define STIMC_INTERNAL_ATTR __attribute__((visibility ("internal")))
typedef void *stimc_thread_impl;

stimc_thread_impl STIMC_INTERNAL_ATTR stimc_thread_impl_create (void (*func)(STIMC_THREAD_ARG_DECL), size_t stacksize);
void STIMC_INTERNAL_ATTR              stimc_thread_impl_run (stimc_thread_impl t);
void STIMC_INTERNAL_ATTR              stimc_thread_impl_suspend (void);
void STIMC_INTERNAL_ATTR              stimc_thread_impl_delete (stimc_thread_impl t);

#ifdef __cplusplus
/* *auto-indent-off* */
}
/* *auto-indent-on* */
#endif
#endif


/*******************************************************************************/
/* PCL + LIBCO threads - inline definition */
/*******************************************************************************/
#ifdef STIMC_THREAD_IMPL_PCL
#define STIMC_THREAD_IMPL_LIBCO_PCL

#include <pcl.h>
#include <limits.h>

#define STIMC_STACKSIZE_MAX (size_t)INT_MAX
typedef coroutine_t stimc_thread_impl;

#define STIMC_CO_CREATE(func, stacksize) co_create  (func, NULL, NULL, stacksize)
#define STIMC_CO_CURRENT()               co_current ()
#define STIMC_CO_SWITCH(thread)          co_call    (thread)
#define STIMC_CO_DELETE(thread)          co_delete  (thread)
#endif

#ifdef STIMC_THREAD_IMPL_LIBCO
#define STIMC_THREAD_IMPL_LIBCO_PCL
#include <libco.h>
#include <limits.h>

#define STIMC_STACKSIZE_MAX (size_t)UINT_MAX
typedef cothread_t stimc_thread_impl;

#define STIMC_CO_CREATE(func, stacksize) co_create (stacksize, func)
#define STIMC_CO_CURRENT()               co_active ()
#define STIMC_CO_SWITCH(thread)          co_switch (thread)
#define STIMC_CO_DELETE(thread)          co_delete (thread)
#endif

#ifdef STIMC_THREAD_IMPL_LIBCO_PCL
#include <assert.h>

static stimc_thread_impl stimc_thread_impl_main = NULL;

static inline stimc_thread_impl stimc_thread_impl_create (void (*func)(STIMC_THREAD_ARG_DECL), size_t stacksize)
{
    if (stacksize > STIMC_STACKSIZE_MAX) stacksize = STIMC_STACKSIZE_MAX;

    stimc_thread_impl t = STIMC_CO_CREATE (func, stacksize);

    assert (t);
    return t;
}

static inline void stimc_thread_impl_run (stimc_thread_impl t)
{
    stimc_thread_impl prev = stimc_thread_impl_main;

    stimc_thread_impl_main = STIMC_CO_CURRENT ();

    STIMC_CO_SWITCH (t);

    stimc_thread_impl_main = prev;
}

static inline void stimc_thread_impl_suspend (void)
{
    assert (stimc_thread_impl_main);

    STIMC_CO_SWITCH (stimc_thread_impl_main);
}

static inline void stimc_thread_impl_delete (stimc_thread_impl t)
{
    STIMC_CO_DELETE (t);
}
#endif

#endif

