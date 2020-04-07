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

#ifndef __STIMC_THREAD_IMPL_H__
#define __STIMC_THREAD_IMPL_H__

#ifndef __STIMC_INTERNAL_HEADER__
#error "stimc internal header should not be included outside of stimc core"
#endif

/* implementation variants - default: pcl */
#ifdef STIMC_THREAD_IMPL_PCL
#define STIMC_THREAD_IMPL
#endif

#ifdef STIMC_THREAD_IMPL_BOOST
#define STIMC_THREAD_IMPL_BOOST2
#endif

#ifdef STIMC_THREAD_IMPL_BOOST1
#define STIMC_THREAD_IMPL
#define STIMC_THREAD_IMPL_EXTERNAL
#endif

#ifdef STIMC_THREAD_IMPL_BOOST2
#define STIMC_THREAD_IMPL
#define STIMC_THREAD_IMPL_EXTERNAL
#endif

#ifndef STIMC_THREAD_IMPL
#define STIMC_THREAD_IMPL_PCL
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* PCL threads */
#ifdef STIMC_THREAD_IMPL_PCL
#include <pcl.h>
#include <assert.h>

typedef coroutine_t stimc_thread_impl;

static inline stimc_thread_impl stimc_thread_impl_create (void (*func)(void *), void *data, size_t stacksize)
{
    coroutine_t co = co_create (func, data, NULL, stacksize);

    assert (co);
    return co;
}

static inline void stimc_thread_impl_run (stimc_thread_impl t)
{
    co_call (t);
}

static inline void stimc_thread_impl_suspend (void)
{
    co_resume ();
}

static inline void stimc_thread_impl_exit (void)
{
    co_exit ();
}

static inline void stimc_thread_impl_delete (stimc_thread_impl t)
{
    co_delete (t);
}

#endif

/* Boost threads */
#ifdef STIMC_THREAD_IMPL_EXTERNAL
typedef void *stimc_thread_impl;

stimc_thread_impl stimc_thread_impl_create (void (*func)(void *), void *data, size_t stacksize);
void              stimc_thread_impl_run (stimc_thread_impl t);
void              stimc_thread_impl_suspend (void);
void              stimc_thread_impl_exit (void);
void              stimc_thread_impl_delete (stimc_thread_impl t);
#endif

#ifdef __cplusplus
}
#endif

#endif

