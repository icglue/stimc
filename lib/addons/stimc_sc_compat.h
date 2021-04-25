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
 * @brief stimc++ compatibility functions for combined stimc/systemc testcases.
 *
 * This is not meant as a systemc replacement, as modules probably need
 * to be rewritten.
 * But in cases of compatible abstract interfaces most abstract
 * testcase functionality might be reduced to only use wait, time and
 * event functionality, which is provided here.
 */

#ifndef STIMC_SC_COMPAT_H
#define STIMC_SC_COMPAT_H

#include <stimc++.h>
#include <string>

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
     */
    static inline bool timed_out ()
    {
        return stimc_wait_timed_out ();
    }

    /**
     * @brief Inline wait wrapper.
     * @param time Amount of time in unit specified by @c exp for timeout.
     * @param exp Time unit (e.g. SC_US).
     * @param e Event to wait for.
     * @return true in case of timeout.
     * Calls @ref event::wait.
     */
    static inline void wait (uint64_t time, enum stimc_time_unit exp, event &e)
    {
        e.wait (time, exp);
    }

    /**
     * @brief Inline wait wrapper, rvalue version.
     * @param time Amount of time in unit specified by @c exp for timeout.
     * @param exp Time unit (e.g. SC_US).
     * @param ec Event combination to wait for.
     * @return true in case of timeout.
     * Calls @ref event_combination::wait.
     */
    static inline void wait (uint64_t time, enum stimc_time_unit exp, event_combination &&ec)
    {
        std::move (ec).wait (time, exp);
    }

    /**
     * @brief Inline wait wrapper.
     * @param time Amount of time in unit specified by @c exp for timeout.
     * @param exp Time unit (e.g. SC_US).
     * @param ec Event combination to wait for.
     * @return true in case of timeout.
     * Calls @ref event_combination::wait.
     */
    static inline void wait (uint64_t time, enum stimc_time_unit exp, const event_combination &ec)
    {
        ec.wait (time, exp);
    }

    /**
     * @brief Inline wait forever wrapper.
     * Calls @ref stimc_thread_halt.
     */
    static inline void wait (void)
    {
        stimc_thread_halt ();
    }

    /**
     * @brief systemc sc_time minimal replacement.
     *
     * only sufficient for some use cases, mainly
     * getting the current simulation time and converting it to some
     * type (string, double).
     */
    class sc_time {
        private:
            uint64_t _time_ps;
        public:
            sc_time (uint64_t time_ps) : _time_ps (time_ps) {};
            ~sc_time () = default; /**< @brief Default sufficient */

            sc_time            (const sc_time &t) = default; /**< @brief Default sufficient */
            sc_time& operator= (const sc_time &t) = default; /**< @brief Default sufficient */
            sc_time            (sc_time &&t)      = default; /**< @brief Default sufficient */
            sc_time& operator= (sc_time &&t)      = default; /**< @brief Default sufficient */

            /**
             * @return time in nano seconds.
             */
            double to_double () const
            {
                return ((double)_time_ps) * 1e-3;
            }

            /**
             * @return time in seconds.
             */
            double to_seconds () const
            {
                return ((double)_time_ps) * 1e-12;
            }

            /**
             * @return time as string in nano seconds.
             */
            std::string to_string () const
            {
                std::string buffer (std::to_string (to_double ()) + std::string (" ns"));

                return buffer;
            }
    };

    /**
     * @brief sc_time_stamp replacement.
     * @return time in nano seconds as default.
     */
    static inline sc_time sc_time_stamp ()
    {
        sc_time time_stamp (stimc_time (SC_PS));

        return time_stamp;
    }
}

#endif

