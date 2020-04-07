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
 * @brief stimc thread boost implementation.
 */

/* only compile if boost implementation selectod */

#ifdef STIMC_THREAD_IMPL_BOOST
#define STIMC_THREAD_IMPL_BOOST2
#endif

#ifdef STIMC_THREAD_IMPL_BOOST1
#define STIMC_THREAD_IMPL_BOOST_ANY

#include <boost/range.hpp>
#include <boost/coroutine/asymmetric_coroutine.hpp>
#include <assert.h>

typedef boost::coroutines::coroutine<void> coro_t;
#endif

#ifdef STIMC_THREAD_IMPL_BOOST2
#define STIMC_THREAD_IMPL_BOOST_ANY

#include <boost/coroutine2/coroutine.hpp>

typedef boost::coroutines2::coroutine<void> coro_t;
#endif

#ifdef STIMC_THREAD_IMPL_BOOST_ANY
struct stimc_thread_impl_s {
    coro_t::pull_type *main;
    coro_t::push_type *thread;
    bool               exit;
};

typedef struct stimc_thread_impl_s *stimc_thread_impl;

static stimc_thread_impl global_thread_current = nullptr;


#ifdef STIMC_THREAD_IMPL_BOOST2
extern "C" stimc_thread_impl stimc_thread_impl_create (void (*func)(void *), void *data, size_t stacksize __attribute__((unused)))
#else
extern "C" stimc_thread_impl stimc_thread_impl_create (void (*func)(void *), void *data, size_t stacksize)
#endif
{
    struct stimc_thread_impl_s *thread_data = new struct stimc_thread_impl_s;

    thread_data->main   = nullptr;
    thread_data->thread = nullptr;
    thread_data->exit   = false;

    coro_t::push_type *thread = new coro_t::push_type (
        [func, data, thread_data](coro_t::pull_type& main){
            thread_data->main = &main;
            main ();
            func (data);
        }
#ifdef STIMC_THREAD_IMPL_BOOST1
        ,
        boost::coroutines::attributes (stacksize)
#endif
    );

    thread_data->thread = thread;

    (*thread)();

    return thread_data;
}

extern "C" void stimc_thread_impl_delete (stimc_thread_impl t)
{
    assert (global_thread_current == nullptr);

    delete t->thread;
    delete t;
}

extern "C" void stimc_thread_impl_run (stimc_thread_impl t)
{
    assert (global_thread_current == nullptr);

    global_thread_current = t;

    coro_t::push_type &thread = *(t->thread);

    thread ();

    global_thread_current = nullptr;

    if (t->exit) {
        stimc_thread_impl_delete (t);
    }
}

extern "C" void stimc_thread_impl_suspend (void)
{
    assert (global_thread_current != nullptr);

    coro_t::pull_type &main = *(global_thread_current->main);

    main ();
}

extern "C" void stimc_thread_impl_exit (void)
{
    assert (global_thread_current != nullptr);

    global_thread_current->exit = true;
    coro_t::pull_type &main = *(global_thread_current->main);

    main ();
}

#endif

