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
 * @brief stimc++ (c++ stimc wrapper).
 */

#include "stimc++.h"

namespace stimcxx {
    module::module ()
    {
        stimc_module_init (&(this->_module), module::cleanup, this);
    }

    module::~module ()
    {
        stimc_module_free (&(this->_module));
    }

    void module::cleanup (void *m)
    {
        module *mod = (module *)m;

        delete mod;
    }

    module::port_base::port_base (module &m, const char *name)
    {
        this->_port = stimc_port_init (&(m._module), name);
    }

    module::port_base::~port_base ()
    {
        stimc_port_free (this->_port);
    }

    module::port::port (module &m, const char *name) :
        port_base (m, name)
    {}

    module::port_real::port_real (module &m, const char *name) :
        port_base (m, name)
    {}

    module::parameter::parameter (module &m, const char *name)
    {
        this->_parameter = stimc_parameter_init (&(m._module), name);

        if (stimc_parameter_get_format (this->_parameter) == vpiRealVal) {
            this->_value_d = stimc_parameter_get_double (this->_parameter);
            this->_value_i = this->_value_d;
        } else {
            this->_value_i = stimc_parameter_get_int32 (this->_parameter);
            this->_value_d = this->_value_i;
        }
    }

    module::parameter::~parameter ()
    {
        stimc_parameter_free (this->_parameter);
    }

    event::event ()
    {
        this->_event = stimc_event_create ();
    }

    event::~event ()
    {
        stimc_event_free (this->_event);
    }
};

