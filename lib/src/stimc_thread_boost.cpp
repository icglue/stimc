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
 * @brief stimc thread boost implementation.
 */

#ifdef STIMC_THREAD_IMPL_BOOST
#define STIMC_THREAD_IMPL_BOOST2
#endif

/*******************************************************************************/
/* Boost version specific includes/typedefs */
/*******************************************************************************/
#ifdef STIMC_THREAD_IMPL_BOOST1
#define STIMC_THREAD_IMPL_BOOST_ANY

#include <boost/range.hpp>
#include <boost/coroutine/asymmetric_coroutine.hpp>
#include <assert.h>

typedef boost::coroutines::coroutine<void (*)(void)> coro_t;
#endif

#ifdef STIMC_THREAD_IMPL_BOOST2
#define STIMC_THREAD_IMPL_BOOST_ANY

#include <boost/coroutine2/coroutine.hpp>

typedef boost::coroutines2::coroutine<void (*)(void)> coro_t;
#endif

/*******************************************************************************/
/* only compile something, if any boost version selected */
/*******************************************************************************/
#ifdef STIMC_THREAD_IMPL_BOOST_ANY
struct stimc_thread_impl_s {
    coro_t::push_type *thread;
    coro_t::pull_type *main;
};

typedef stimc_thread_impl_s *stimc_thread_impl;

static stimc_thread_impl stimc_thread_impl_current = nullptr;


static void stimc_thread_impl_boost_wrap (coro_t::pull_type &main)
{
    assert (stimc_thread_impl_current);

    stimc_thread_impl_current->main = &main;
    void (*func) (void)             = main.get ();

    assert (func);

    main ();

    func ();
}

#ifdef STIMC_THREAD_IMPL_BOOST2
#define STIMC_THREAD_IMPL_STACKSIZE_ATTR __attribute__((unused))
#else
#define STIMC_THREAD_IMPL_STACKSIZE_ATTR
#endif

extern "C" stimc_thread_impl stimc_thread_impl_create (void (*func)(void), size_t stacksize STIMC_THREAD_IMPL_STACKSIZE_ATTR)
{
    struct stimc_thread_impl_s *t = new struct stimc_thread_impl_s;

    t->main   = nullptr;
    t->thread = nullptr;

    coro_t::push_type *co = new coro_t::push_type (
        stimc_thread_impl_boost_wrap
#ifdef STIMC_THREAD_IMPL_BOOST1
        ,
        boost::coroutines::attributes (stacksize)
#endif
        );

    assert (co);
    t->thread = co;

    stimc_thread_impl prev = stimc_thread_impl_current;

    stimc_thread_impl_current = t;
    coro_t::push_type &thread = *co;

    thread (func);

    stimc_thread_impl_current = prev;

    return t;
}

extern "C" void stimc_thread_impl_run (stimc_thread_impl t)
{
    stimc_thread_impl prev = stimc_thread_impl_current;

    stimc_thread_impl_current = t;
    coro_t::push_type &thread = *(t->thread);

    thread (nullptr);

    stimc_thread_impl_current = prev;
}

extern "C" void stimc_thread_impl_suspend (void)
{
    assert (stimc_thread_impl_current);

    coro_t::pull_type &main = *(stimc_thread_impl_current->main);

    main ();
}

extern "C" void stimc_thread_impl_delete (stimc_thread_impl t)
{
    delete t->thread;
    delete t;
}
#endif

