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
 * @brief stimc thread libco implementation.
 */

/* only compile if libco implementation selectod */

#ifdef STIMC_THREAD_IMPL_LIBCO

#include <libco.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>

struct stimc_thread_impl_s {
    cothread_t main;
    cothread_t thread;
    void       (*func) (void *data);
    void      *data;
};

typedef struct stimc_thread_impl_s *stimc_thread_impl;

static stimc_thread_impl global_thread_current = NULL;


static void stimc_thread_impl_libco_wrap (void)
{
    assert (global_thread_current);

    global_thread_current->func (global_thread_current->data);

    co_switch (global_thread_current->main);
}

stimc_thread_impl stimc_thread_impl_create (void (*func)(void *), void *data, size_t stacksize)
{
    struct stimc_thread_impl_s *thread_data = (struct stimc_thread_impl_s *)malloc (sizeof (struct stimc_thread_impl_s));

    thread_data->main   = co_active ();
    thread_data->thread = co_create (stacksize, stimc_thread_impl_libco_wrap);
    thread_data->func   = func;
    thread_data->data   = data;

    assert (thread_data->thread);

    return thread_data;
}

void stimc_thread_impl_delete (stimc_thread_impl t)
{
    assert (global_thread_current == NULL);

    co_delete (t->thread);
    free (t);
}

void stimc_thread_impl_run (stimc_thread_impl t)
{
    assert (global_thread_current == NULL);

    global_thread_current = t;

    co_switch (t->thread);

    global_thread_current = NULL;
}

void stimc_thread_impl_suspend (void)
{
    assert (global_thread_current);

    co_switch (global_thread_current->main);
}

void stimc_thread_impl_exit (void)
{
    assert (global_thread_current);

    co_switch (global_thread_current->main);
}

#endif

