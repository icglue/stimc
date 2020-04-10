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
 * You can you STIMCXX_EXPORT (modulename) within the cpp module definition file.
 *
 * Alternatively the follow function
 *   stimc_module_register_func_t *get_stimc_module_register_list (void)
 * can be defined to return a NULL terminate array of function pointer to the
 * module register function.
 *
 * To simplify this STIMC_INIT (modulename) within the cpp modulename definitions file
 * and a stimc-export.inl file containing a bunch of lines with
 * STIMC_EXPORT (modulename) could be used.
 *
 * You need to compile and link stim-export.c file.
 */

#include <stdlib.h>
#include <stdint.h>
#include <stimc.h>

#define STIMC_REGISTER_PAGE_SIZE 15

typedef uintptr_t stimc_register_page_t[STIMC_REGISTER_PAGE_SIZE + 1];

static stimc_register_page_t *stimc_module_table_ptr = NULL;

static uintptr_t *stimc_alloc_page (void)
{
    uintptr_t *newpage;

    newpage    = (uintptr_t *)malloc (sizeof (stimc_register_page_t));
    newpage[0] = 0;
    return newpage;
}

void stimc_register_module (stimc_module_register_func_t stimc_module_register_func)
{
    if (stimc_module_table_ptr == NULL) {
        stimc_module_table_ptr = (stimc_register_page_t *)stimc_alloc_page ();
    }

    int        funcidx = 0;
    uintptr_t *page    = (uintptr_t *)stimc_module_table_ptr;

    while (page[funcidx] != 0) {
        if (++funcidx == STIMC_REGISTER_PAGE_SIZE) {
            if (page[funcidx] == 0) {
                page[funcidx] = (uintptr_t)stimc_alloc_page ();
            }
            funcidx = 0;
            page    = (uintptr_t *)page[STIMC_REGISTER_PAGE_SIZE];
        }
    }

    page[funcidx++] = (uintptr_t)stimc_module_register_func;
    page[funcidx]   = 0;
}

__attribute__ ((weak)) stimc_module_register_func_t *get_stimc_module_register_list (void)
{
    return NULL;
}
/**
 * @briefindirect startup function for
 * simulators expecting a function name to call
 */
void stimc_vpi_init (void)
{
    int        funcidx = 0;
    uintptr_t *page    = (uintptr_t *)stimc_module_table_ptr;

    if (page) {
        while (page[funcidx] != 0) {
            ((stimc_module_register_func_t)page[funcidx])();

            if (++funcidx == STIMC_REGISTER_PAGE_SIZE) {
                page = (uintptr_t *)page[STIMC_REGISTER_PAGE_SIZE];
                if (page) {
                    funcidx = 0;
                } else {
                    break;
                }
            }
        }

    }

    stimc_module_register_func_t *stimc_module_register_list = get_stimc_module_register_list ();
    if (stimc_module_register_list) {
        for (unsigned i = 0; stimc_module_register_list[i] != 0; i++) {
            stimc_module_register_list[i]();
        }
    }
}

/* vlog startup vec */
void (*vlog_startup_routines[])(void) = {
    stimc_vpi_init,
    0,
};

