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
 * @brief stimc thread boost implementation.
 */

#ifdef STIMC_THREAD_IMPL_BOOST
#define STIMC_THREAD_IMPL_BOOST2
#endif

/*******************************************************************************/
/* Boost version specific includes/aliases */
/*******************************************************************************/
#ifdef STIMC_THREAD_IMPL_BOOST1

#include <boost/range.hpp>
#include <boost/coroutine/asymmetric_coroutine.hpp>
#include <assert.h>

using coro_t = boost::coroutines::coroutine<void (*)(void)>;
#endif

#ifdef STIMC_THREAD_IMPL_BOOST2

#include <boost/coroutine2/coroutine.hpp>

using coro_t = boost::coroutines2::coroutine<void (*)(void)>;
#endif

/*******************************************************************************/
/* only compile something, if any boost version selected */
/*******************************************************************************/
#if defined(STIMC_THREAD_IMPL_BOOST1) || defined(STIMC_THREAD_IMPL_BOOST2)

#include <memory>

/* internal header */
#define STIMC_USE_INTERNAL_HEADER
#include "stimc_thread.inl"
#undef STIMC_USE_INTERNAL_HEADER

using stimc_thread_impl_func = void (*)(void);

struct stimc_thread_impl_s {
    std::unique_ptr<coro_t::push_type> thread;
    coro_t::pull_type                 *main;
    stimc_thread_impl_func             func;
    const size_t                       stacksize;

    stimc_thread_impl_s (stimc_thread_impl_func f, size_t s) :
        thread    (nullptr),
        main      (nullptr),
        func      (f),
        stacksize (s)
    {}

    stimc_thread_impl_s            (const stimc_thread_impl_s &t) = delete;
    stimc_thread_impl_s& operator= (const stimc_thread_impl_s &t) = delete;
    stimc_thread_impl_s            (stimc_thread_impl_s &&t)      = delete;
    stimc_thread_impl_s& operator= (stimc_thread_impl_s &&t)      = delete;

    ~stimc_thread_impl_s () = default;
};

using stimc_thread_impl_boost = stimc_thread_impl_s *;

static stimc_thread_impl_boost stimc_thread_impl_current = nullptr;


static void stimc_thread_impl_boost_wrap (coro_t::pull_type &main)
{
    assert (stimc_thread_impl_current);

    stimc_thread_impl_current->main = &main;

    stimc_thread_impl_func func = main.get ();

    assert (func);

    func ();
}

static inline void stimc_thread_impl_boost_init (stimc_thread_impl_boost t)
{
    if (t->thread != nullptr) return;

    t->thread.reset (new coro_t::push_type (
                         stimc_thread_impl_boost_wrap
#ifdef STIMC_THREAD_IMPL_BOOST1
                         ,
                         boost::coroutines::attributes (t->stacksize)
#endif
                         ));

    assert (t->thread != nullptr);
}

extern "C" stimc_thread_impl stimc_thread_impl_create (stimc_thread_impl_func func, size_t stacksize)
{
    stimc_thread_impl_boost t = new struct stimc_thread_impl_s (func, stacksize);

    return t;
}

extern "C" void stimc_thread_impl_run (stimc_thread_impl t_ext)
{
    stimc_thread_impl_boost t = static_cast<stimc_thread_impl_boost>(t_ext);

    assert (stimc_thread_impl_current == nullptr);

    stimc_thread_impl_boost_init (t);

    stimc_thread_impl_current = t;
    coro_t::push_type &thread = *(t->thread);

    thread (t->func);

    stimc_thread_impl_current = nullptr;
}

extern "C" void stimc_thread_impl_suspend (void)
{
    assert (stimc_thread_impl_current);

    coro_t::pull_type &main = *(stimc_thread_impl_current->main);

    main ();
}

extern "C" void stimc_thread_impl_delete (stimc_thread_impl t_ext)
{
    stimc_thread_impl_boost t = static_cast<stimc_thread_impl_boost>(t_ext);

    delete t;
}
#endif

