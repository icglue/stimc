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
 * @brief stimc vpi library entry point template.
 *
 * You need to provide a "stimc-export.inl" include file
 * containing all stimc modules to be exported in lines of the form
 * STIMC_EXPORT (modulename)
 */

typedef void (*stimc_vpi_init_register_func_t) (void);

/* declaration */
#define STIMC_EXPORT(module) \
    void _stimc_module_ ## module ## _register (void);

#include "stimc-export.inl"

#undef STIMC_EXPORT
#define STIMC_EXPORT(module) \
    _stimc_module_ ## module ## _register,

/* indirected vlog startup vec */
static stimc_vpi_init_register_func_t stimc_module_init_list[] = {
#   include "stimc-export.inl"
    0,
};

stimc_vpi_init_register_func_t *stimc_module_register_list (void)
{
    return stimc_module_init_list;
}

