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
 * @brief stimc++ compatibility functions for combined stimc/systemc testcases.
 *
 * This is not meant as a systemc replacement, as modules probably need
 * to be rewritten.
 * But in cases of compatible abstract interfaces most abstract
 * testcase functionality might be reduced to only use wait, time and
 * event functionality, which is provided here.
 */

#ifndef __STIMC_SC_COMPAT_H__
#define __STIMC_SC_COMPAT_H__

#include <stimc++.h>

/**
 * @brief stimc++ namespace.
 */
namespace stimcxx {
    class sc_event : public event {
        public:
            sc_event () = default;

            sc_event            (const sc_event &e) = delete; /**< @brief Do not copy/change internals */
            sc_event& operator= (const sc_event &e) = delete; /**< @brief Do not copy/change internals */
            sc_event            (sc_event &&e)      = delete; /**< @brief Do not move/change internals */
            sc_event& operator= (sc_event &&e)      = delete; /**< @brief Do not move/change internals */

            /**
             * @brief default destructor (= base destructor).
             *
             * Warning: This should be sufficient as in case of
             * slicing the right thing will happen (freeing resources
             * in base class). In case default functionality is not sufficient,
             * this cannot be guaranteed anymore, as base class destructor
             * is not virtual.
             */
            ~sc_event () = default;

            /**
             * @brief Trigger event.
             *
             * Inline wrapper for @ref trigger.
             */
            void notify ()
            {
                trigger ();
            }
    };

    /**
     * @brief Return, whether the most recent waiting on an event
     * with timeout ran into the timeout.
     *
     * Warning: this is only a workaround for "typical" cases (e.g.
     * wait with timeout + call to @ref timed_out in the same compilation
     * unit. In these cases it will work as a thread runs until it's next
     * call to a wait function in which case time-out information could
     * be seen as obsolete anyway.
     */
    bool timed_out ();

    /**
     * @brief Inline wait wrapper.
     * @param time Amount of time in unit specified by @c exp for timeout.
     * @param exp Time unit (e.g. SC_US).
     * @param e Event to wait for.
     * @return true in case of timeout.
     * Calls @ref event::wait.
     */
    void wait (uint64_t time, enum stimc_time_unit exp, event &e);

    /**
     * @brief Inline wait wrapper, rvalue version.
     * @param time Amount of time in unit specified by @c exp for timeout.
     * @param exp Time unit (e.g. SC_US).
     * @param ec Event combination to wait for.
     * @return true in case of timeout.
     * Calls @ref event_combination::wait.
     */
    void wait (uint64_t time, enum stimc_time_unit exp, event_combination &&ec);

    /**
     * @brief Inline wait wrapper.
     * @param time Amount of time in unit specified by @c exp for timeout.
     * @param exp Time unit (e.g. SC_US).
     * @param ec Event combination to wait for.
     * @return true in case of timeout.
     * Calls @ref event_combination::wait.
     */
    void wait (uint64_t time, enum stimc_time_unit exp, const event_combination &ec);
}

#endif

