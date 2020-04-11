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
 * @brief stimc thread implementation.
 */

#ifndef __STIMC_THREAD_H__
#define __STIMC_THREAD_H__

#ifndef __STIMC_INTERNAL_HEADER__
#error "stimc internal header should not be included outside of stimc core"
#endif


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
extern "C" {
#endif

typedef void *stimc_thread_impl;

stimc_thread_impl stimc_thread_impl_create (void (*func)(STIMC_THREAD_ARG_DECL), size_t stacksize);
void              stimc_thread_impl_run (stimc_thread_impl t);
void              stimc_thread_impl_suspend (void);
void              stimc_thread_impl_delete (stimc_thread_impl t);

#ifdef __cplusplus
}
#endif
#endif


/*******************************************************************************/
/* PCL threads - inline definition */
/*******************************************************************************/
#ifdef STIMC_THREAD_IMPL_PCL
#include <pcl.h>
#include <limits.h>

typedef coroutine_t stimc_thread_impl;

static inline stimc_thread_impl stimc_thread_impl_create (void (*func)(STIMC_THREAD_ARG_DECL), size_t stacksize)
{
    if (stacksize > (size_t)INT_MAX) stacksize = INT_MAX;

    coroutine_t t = co_create (func, NULL, NULL, stacksize);

    assert (t);
    return t;
}

static inline void stimc_thread_impl_run (stimc_thread_impl t)
{
    co_call (t);
}

static inline void stimc_thread_impl_suspend (void)
{
    co_resume ();
}

static inline void stimc_thread_impl_delete (stimc_thread_impl t)
{
    co_delete (t);
}
#endif


/*******************************************************************************/
/* LIBCO threads - inline definition */
/*******************************************************************************/
#ifdef STIMC_THREAD_IMPL_LIBCO
#include <libco.h>
#include <limits.h>

typedef cothread_t stimc_thread_impl;

static stimc_thread_impl stimc_thread_impl_main = NULL;

static inline stimc_thread_impl stimc_thread_impl_create (void (*func)(void), size_t stacksize)
{
    if (stacksize > (size_t)UINT_MAX) stacksize = UINT_MAX;

    cothread_t t = co_create (stacksize, func);

    assert (t);
    return t;
}

static inline void stimc_thread_impl_run (stimc_thread_impl t)
{
    cothread_t prev = stimc_thread_impl_main;

    stimc_thread_impl_main = co_active ();

    co_switch (t);

    stimc_thread_impl_main = prev;
}

static inline void stimc_thread_impl_suspend (void)
{
    assert (stimc_thread_impl_main);

    co_switch (stimc_thread_impl_main);
}

static inline void stimc_thread_impl_delete (stimc_thread_impl t)
{
    co_delete (t);
}
#endif


#endif

