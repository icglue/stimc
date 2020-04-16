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
 * @brief stimc vpi library entry point handling.
 *
 * You can use STIMCXX_EXPORT (modulename) within the cpp module definition file.
 *
 * Alternatively the following function
 *   stimc_vpi_init_register_func_t *stimc_module_register_list (void)
 * can be defined to return a NULL terminated array of function pointers to the
 * module register functions.
 *
 * To simplify this STIMC_INIT (modulename) within the cpp modulename definitions file
 * and a stimc-export.inl file containing a bunch of lines with
 * STIMC_EXPORT (modulename) could be used.
 *
 * You need to compile and link stim-export.c file.
 */

#include <stimc.h>

static struct stimc_vpi_init_register_s *stimc_vpi_init_list = NULL;

void stimc_vpi_init_register (struct stimc_vpi_init_register_s *entry)
{
    if (entry->next != NULL) return;

    entry->next         = stimc_vpi_init_list;
    stimc_vpi_init_list = entry;
}

__attribute__ ((weak)) stimc_vpi_init_register_func_t *stimc_module_register_list (void)
{
    return NULL;
}

/**
 * @briefindirect startup function for
 * simulators expecting a function name to call
 */
void stimc_vpi_init (void)
{
    for (struct stimc_vpi_init_register_s *l = stimc_vpi_init_list; l != NULL; l = l->next) {
        l->func ();
    }

    stimc_vpi_init_register_func_t *init_list = stimc_module_register_list ();
    if (init_list) {
        for (unsigned i = 0; init_list[i] != 0; i++) {
            init_list[i]();
        }
    }
}

/* vlog startup vec */
void (*vlog_startup_routines[])(void) = {
    stimc_vpi_init,
    0,
};

