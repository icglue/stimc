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

#include "stimc_sc_compat.h"

namespace stimcxx {
    static bool _last_wait_timeout = false;

    bool timed_out ()
    {
        return _last_wait_timeout;
    }

    void wait (uint64_t time, enum stimc_time_unit exp, event &e)
    {
        _last_wait_timeout = e.wait (time, exp);
    }

    void wait (uint64_t time, enum stimc_time_unit exp, event_combination &&ec)
    {
        _last_wait_timeout = std::move (ec).wait (time, exp);
    }

    void wait (uint64_t time, enum stimc_time_unit exp, const event_combination &ec)
    {
        _last_wait_timeout = ec.wait (time, exp);
    }

    const std::string sc_time::to_string () const
    {
        std::string buffer (std::to_string (to_double ()) + std::string(" ns"));

        return buffer;
    }
}

