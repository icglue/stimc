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
 * @brief stimc++ (c++ stimc wrapper).
 */

#ifndef STIMCXX_H
#define STIMCXX_H

#include <stimc.h>
#include <utility>

/**
 * @brief stimc++ namespace.
 */
namespace stimcxx {
    class event_combination_all;
    class event_combination_any;

#ifdef STIMCXX_DISABLE_STACK_UNWIND
    constexpr bool enable_stack_unwind = false;
#else
    constexpr bool enable_stack_unwind = true;
#endif

    /**
     * @brief Dummy exception class to mark final stack unwinding.
     */
    class thread_finish_exception {
        public:
            thread_finish_exception  () noexcept = default;
            ~thread_finish_exception () noexcept = default;

            thread_finish_exception            (const thread_finish_exception &t) noexcept = default; /**< @brief Default copy */
            thread_finish_exception& operator= (const thread_finish_exception &t) noexcept = default; /**< @brief Default copy */
            thread_finish_exception            (thread_finish_exception &&t)      noexcept = default; /**< @brief Default move */
            thread_finish_exception& operator= (thread_finish_exception &&t)      noexcept = default; /**< @brief Default move */
    };

    /**
     * @brief Helper for common finish check on return from suspend.
     */
    static inline void thread_finish_check ()
    {
        if (enable_stack_unwind && stimc_thread_is_finished()) {
            throw thread_finish_exception ();
        }
    }

    /**
     * @brief Wrapper class for @ref stimc_event and related functionality.
     */
    class event {
        private:
            stimc_event _event; /**< @brief The actual @ref stimc_event. */
        public:
            /**
             * @brief Create @ref stimc_event.
             */
            event () noexcept :
                _event (stimc_event_create ())
            {}

            event            (const event &e) = delete; /**< @brief Do not copy/change internals */
            event& operator= (const event &e) = delete; /**< @brief Do not copy/change internals */

            event (event &&e) noexcept :
                _event (e._event)
            {
                e._event = nullptr;
            }

            event& operator= (event &&e) noexcept
            {
                stimc_event t = e._event;

                e._event     = this->_event;
                this->_event = t;

                return *this;
            }

            /**
             * @brief Free @ref stimc_event.
             */
            ~event () noexcept
            {
                if (_event != nullptr) stimc_event_free (this->_event);
            }

            /**
             * @brief Wait for event to be triggered.
             *
             * Inline wrapper for @ref stimc_wait_event.
             */
            void wait ()
            {
                stimc_wait_event (_event);
                thread_finish_check ();
            }

            /**
             * @brief Wait for event to be triggered or specified timeout.
             * @param time_seconds Amount of time in seconds for timeout.
             *
             * @return true in case of timeout.
             *
             * Inline wrapper for @ref stimc_wait_event_timeout_seconds.
             */
            bool wait (double time_seconds)
            {
                bool result = stimc_wait_event_timeout_seconds (_event, time_seconds);

                thread_finish_check ();

                return result;
            }

            /**
             * @brief Wait for event to be triggered or specified timeout.
             * @param time Amount of time in unit specified by @c exp for timeout.
             * @param exp Time unit (e.g. SC_US).
             *
             * @return true in case of timeout.
             *
             * Inline wrapper for @ref stimc_wait_event_timeout.
             */
            bool wait (uint64_t time, enum stimc_time_unit exp)
            {
                bool result = stimc_wait_event_timeout (_event, time, exp);

                thread_finish_check ();

                return result;
            }

            /**
             * @brief Trigger event.
             *
             * Inline wrapper for @ref stimc_trigger_event.
             */
            void trigger () noexcept
            {
                stimc_trigger_event (_event);
            }

            /**
             * @brief Combine two events for waiting on both of them
             *
             * @param rhs other event.
             *
             * @return Combined events.
             */
            event_combination_all operator& (const event &rhs) noexcept;

            /**
             * @brief Combine two events for waiting on any of them
             *
             * @param rhs other event.
             *
             * @return Combined events.
             */
            event_combination_any operator| (const event &rhs) noexcept;

            friend class event_combination;
    };

    /**
     * @brief Common base class for event combination via @ref stimc_event_combination
     */
    class event_combination {
        protected:
            stimc_event_combination _combination; /**< @brief The actual @ref stimc_event_combination. */

        protected:
            /**
             * @brief Constructor for initial creation based on operation of 2 events.
             *
             * @param e1 lhs event.
             * @param e2 rhs event.
             * @param any Whether the combination will represent waiting on any (true) event or all (false) events.
             */
            event_combination (const event &e1, const event &e2, bool any) noexcept :
                _combination (stimc_event_combination_create (any))
            {
                append (e1);
                append (e2);
            }

            /**
             * @brief Constructor for extension based on operation with additional event.
             *
             * @param ec lhs operation.
             * @param e rhs event.
             */
            event_combination (const event_combination &ec, const event &e) noexcept :
                _combination (stimc_event_combination_create (false))
            {
                stimc_event_combination_copy (_combination, ec._combination);
                append (e);
            }

            /**
             * @brief Extension helper.
             *
             * @param e event to extend with.
             */
            void append (const event &e) noexcept
            {
                stimc_event_combination_append (_combination, e._event);
            }

        public:
            /**
             * @brief Copy constructor.
             *
             * @param ec copy source.
             */
            event_combination (const event_combination &ec) noexcept :
                _combination (stimc_event_combination_create (false))
            {
                stimc_event_combination_copy (_combination, ec._combination);
            }

            /**
             * @brief Copy assignment operator.
             *
             * @param ec copy source.
             *
             * @return @c *this.
             */
            event_combination& operator= (const event_combination &ec) noexcept
            {
                stimc_event_combination_copy (_combination, ec._combination);
                return *this;
            }

            /**
             * @brief Move constructor.
             *
             * @param ec move source.
             */
            event_combination (event_combination &&ec) noexcept :
                _combination (ec._combination)
            {
                ec._combination = nullptr;
            }

            /**
             * @brief Move assignment operator.
             *
             * @param ec move source.
             *
             * @return @c *this.
             */
            event_combination& operator= (event_combination &&ec) noexcept
            {
                stimc_event_combination_free (_combination);
                _combination    = ec._combination;
                ec._combination = nullptr;

                return *this;
            }

            /**
             * @brief Destructor.
             *
             * Non-virtual, as the only derived classes @ref event_combination_all
             * and @ref event_combination_any do not add any resources to be
             * released.
             */
            ~event_combination () noexcept
            {
                if (_combination != nullptr) stimc_event_combination_free (_combination);
            }

            /**
             * @brief Wait for events of combination to be triggered based on type of combination.
             *
             * Inline wrapper for @ref stimc_wait_event_combination.
             */
            void wait () const &
            {
                stimc_wait_event_combination (_combination, false);
                thread_finish_check ();
            }

            /**
             * @brief Wait for events of combination to be triggered based on type of combination (rvalue version).
             *
             * Inline wrapper for @ref stimc_wait_event_combination.
             */
            void wait () &&
            {
                stimc_event_combination c = _combination;
                _combination = nullptr;

                stimc_wait_event_combination (c, true);
                thread_finish_check ();
            }

            /**
             * @brief Wait for events of combination to be triggered based on type of combination or specified timeout.
             * @param time_seconds Amount of time in seconds for timeout.
             *
             * @return true in case of timeout.
             *
             * Inline wrapper for @ref stimc_wait_event_combination_timeout_seconds.
             */
            bool wait (double time_seconds) const &
            {
                bool result = stimc_wait_event_combination_timeout_seconds (_combination, false, time_seconds);

                thread_finish_check ();

                return result;
            }

            /**
             * @brief Wait for events of combination to be triggered based on type of combination or specified timeout
             *        (rvalue version).
             * @param time_seconds Amount of time in seconds for timeout.
             *
             * @return true in case of timeout.
             *
             * Inline wrapper for @ref stimc_wait_event_combination_timeout_seconds.
             */
            bool wait (double time_seconds) &&
            {
                stimc_event_combination c = _combination;
                _combination = nullptr;

                bool result = stimc_wait_event_combination_timeout_seconds (c, true, time_seconds);

                thread_finish_check ();

                return result;
            }

            /**
             * @brief Wait for events of combination to be triggered based on type of combination or specified timeout.
             * @param time Amount of time in unit specified by @c exp for timeout.
             * @param exp Time unit (e.g. SC_US).
             *
             * @return true in case of timeout.
             *
             * Inline wrapper for @ref stimc_wait_event_combination_timeout.
             */
            bool wait (uint64_t time, enum stimc_time_unit exp) const &
            {
                bool result = stimc_wait_event_combination_timeout (_combination, false, time, exp);

                thread_finish_check ();

                return result;
            }

            /**
             * @brief Wait for events of combination to be triggered based on type of combination or specified timeout.
             *        (rvalue version).
             * @param time Amount of time in unit specified by @c exp for timeout.
             * @param exp Time unit (e.g. SC_US).
             *
             * @return true in case of timeout.
             *
             * Inline wrapper for @ref stimc_wait_event_combination_timeout.
             */
            bool wait (uint64_t time, enum stimc_time_unit exp) &&
            {
                stimc_event_combination c = _combination;
                _combination = nullptr;

                bool result = stimc_wait_event_combination_timeout (c, true, time, exp);

                thread_finish_check ();

                return result;
            }
    };

    /**
     * @brief Event combination class for waiting on all given events.
     */
    class event_combination_all : public event_combination {
        protected:
            /**
             * @brief Constructor for initial creation based on operation of 2 events.
             *
             * @param e1 lhs event.
             * @param e2 rhs event.
             */
            event_combination_all (const event &e1, const event &e2) noexcept : event_combination (e1, e2, false) {};

            /**
             * @brief Constructor for extension based on operation with additional event.
             *
             * @param ec lhs operation.
             * @param e rhs event.
             */
            event_combination_all (const event_combination_all &ec, const event &e) noexcept : event_combination (ec, e) {};

        public:
            event_combination_all            (const event_combination_all &ec) noexcept = default; /**< @brief default copy constructor */
            event_combination_all& operator= (const event_combination_all &ec) noexcept = default; /**< @brief default copy assignment */
            event_combination_all            (event_combination_all &&ec) noexcept      = default; /**< @brief default move constructor */
            event_combination_all& operator= (event_combination_all &&ec) noexcept      = default; /**< @brief default move assignment */

            ~event_combination_all () noexcept = default; /**< @brief default destructor */

            friend class event;

            /**
             * @brief Combine combination with additional event.
             *
             * @param rhs event to combine with.
             *
             * @return Extended combination.
             */
            event_combination_all operator& (event &rhs) const & noexcept
            {
                event_combination_all ec (*this, rhs);

                return ec;
            }

            /**
             * @brief Combine combination with additional event (rvalue version).
             *
             * @param rhs event to combine with.
             *
             * @return Extended combination.
             */
            event_combination_all operator& (event &rhs) && noexcept
            {
                append (rhs);
                return std::move (*this);
            }
    };

    inline event_combination_all event::operator& (const event &rhs) noexcept
    {
        event_combination_all ec (*this, rhs);

        return ec;
    }

    /**
     * @brief Event combination class for waiting on any of the given events.
     */
    class event_combination_any : public event_combination {
        protected:
            /**
             * @brief Constructor for initial creation based on operation of 2 events.
             *
             * @param e1 lhs event.
             * @param e2 rhs event.
             */
            event_combination_any (const event &e1, const event &e2) noexcept : event_combination (e1, e2, true) {};

            /**
             * @brief Constructor for extension based on operation with additional event.
             *
             * @param ec lhs operation.
             * @param e rhs event.
             */
            event_combination_any (const event_combination_any &ec, const event &e) noexcept : event_combination (ec, e) {};

        public:
            event_combination_any            (const event_combination_any &ec) noexcept = default; /**< @brief default copy constructor */
            event_combination_any& operator= (const event_combination_any &ec) noexcept = default; /**< @brief default copy assignment */
            event_combination_any            (event_combination_any &&ec) noexcept      = default; /**< @brief default move constructor */
            event_combination_any& operator= (event_combination_any &&ec) noexcept      = default; /**< @brief default move assignment */

            ~event_combination_any () noexcept = default; /**< @brief default destructor */

            friend class event;

            /**
             * @brief Combine combination with additional event.
             *
             * @param rhs event to combine with.
             *
             * @return Extended combination.
             */
            event_combination_any operator| (event &rhs) const & noexcept
            {
                event_combination_any ec (*this, rhs);

                return ec;
            }

            /**
             * @brief Combine combination with additional event (rvalue version).
             *
             * @param rhs event to combine with.
             *
             * @return Extended combination.
             */
            event_combination_any operator| (event &rhs) && noexcept
            {
                append (rhs);
                return std::move (*this);
            }
    };

    inline event_combination_any event::operator| (const event &rhs) noexcept
    {
        event_combination_any ec (*this, rhs);

        return ec;
    }


    /**
     * @brief Convenience type to be able to assign x/z values.
     */
    enum class bit {
        X,      /**< @brief x value (verilog-x ~ unknown) */
        Z,      /**< @brief z value (verilog-z ~ high impedance) */
        not_XZ, /**< @brief not x/z for comparison */
    };

    static const bit X      = bit::X;      /**< @brief x value (verilog-x ~ unknown) */
    static const bit Z      = bit::Z;      /**< @brief z value (verilog-z ~ high impedance) */
    static const bit not_XZ = bit::not_XZ; /**< @brief not x/z for comparison */


    /**
     * @brief Wrapper class for @ref stimc_module and related functionality.
     */
    class module {
        private:
            stimc_module _module; /**< @brief The actual @ref stimc_module. */

            /**
             * @brief Cleanup callback for end of simulation - will delete the module.
             * @param m Pointer to module to delete.
             *
             * Will be handed to @ref stimc_module_init as cleanup callback
             * in constructor.
             */
            static void cleanup (void *m) noexcept
            {
                module *mod = (module *)m;

                delete mod;
            }

        protected:
            /**
             * @brief Init module data of @ref stimc_module.
             *
             * Meant as base class - not to be constructed directly.
             */
            module () noexcept :
                _module {nullptr}
            {
                stimc_module_init (&(this->_module), module::cleanup, this);
            }

        public:
            module            (const module &m) = delete; /**< @brief Do not copy/change internals */
            module& operator= (const module &m) = delete; /**< @brief Do not copy/change internals */
            module            (module &&m)      = delete; /**< @brief Do not move/change internals */
            module& operator= (module &&m)      = delete; /**< @brief Do not move/change internals */

            /**
             * @brief Free module resources.
             */
            virtual ~module ()
            {
                stimc_module_free (&(this->_module));
            }

            /**
             * @brief Get hierarchical identifier for instance of module.
             * @return The verilog hierarchy of module instance.
             */
            const char *module_id () noexcept
            {
                return _module.id;
            }

        protected:
            /**
             * @brief Wrapper base class for @ref stimc_port.
             * @see @ref port and @ref port_real for implementations to be used.
             */
            class port_base {
                protected:
                    stimc_port _port; /**< @brief The actual @ref stimc_port. */

                public:
                    /**
                     * @brief Port constructor for @ref stimc_port.
                     * @param m Parent module of port.
                     * @param name Name of the port.
                     */
                    port_base (module &m, const char *name) noexcept :
                        _port (stimc_port_init (&(m._module), name))
                    {}

                    port_base            (const port_base &p) = delete; /**< @brief Do not copy/change internals */
                    port_base& operator= (const port_base &p) = delete; /**< @brief Do not copy/change internals */
                    port_base            (port_base &&p)      = delete; /**< @brief Do not move/change internals */
                    port_base& operator= (port_base &&p)      = delete; /**< @brief Do not move/change internals */

                    /**
                     * @brief Destructor.
                     *
                     * Non-virtual, as derived classes will be used as members
                     * without slicing in case of destructor call.
                     */
                    ~port_base () noexcept
                    {
                        stimc_port_free (this->_port);
                    }

                    /**
                     * @brief Register a callback method for posedge events at port.
                     * @brief callback Method to register.
                     * @brief p Data pointer for callback.
                     *
                     * Internal helper. For registering from inside a module
                     * use @ref STIMCXX_REGISTER_METHOD for convenience.
                     */
                    void register_posedge_method (void (*callback)(void *p), void *p) noexcept
                    {
                        stimc_register_posedge_method (callback, p, this->_port);
                    }

                    /**
                     * @brief Register a callback method for negedge events at port.
                     * @brief callback Method to register.
                     * @brief p Data pointer for callback.
                     *
                     * Internal helper. For registering from inside a module
                     * use @ref STIMCXX_REGISTER_METHOD for convenience.
                     */
                    void register_negedge_method (void (*callback)(void *p), void *p) noexcept
                    {
                        stimc_register_negedge_method (callback, p, this->_port);
                    }

                    /**
                     * @brief Register a callback method for negedge events at port.
                     * @brief callback Method to register.
                     * @brief p Data pointer for callback.
                     *
                     * Internal helper. For registering from inside a module
                     * use @ref STIMCXX_REGISTER_METHOD for convenience.
                     */
                    void register_change_method (void (*callback)(void *p), void *p) noexcept
                    {
                        stimc_register_change_method (callback, p, this->_port);
                    }
            };

        public:
            /**
             * @brief Wrapper class for @ref stimc_port.
             */
            class port : public port_base {
                public:
                    /**
                     * @brief Helper class for access to bit range of signal.
                     */
                    class subbits {
                        private:
                            int _lsb; /**< @brief Least significant bit of represented bit range. */
                            int _msb; /**< @brief Most significant bit of represented bit range. */
                            port &_p; /**< @brief Port accessed by bit range. */
                        public:
                            /**
                             * @brief Constructor for subbit range of port
                             * @param p Port accessed.
                             * @param msb Most significant bit.
                             * @param lsb Least significant bit.
                             */
                            subbits (port &p, int msb, int lsb) noexcept :
                                _lsb (lsb), _msb (msb), _p (p)
                            {}

                            subbits            (const subbits &sb) noexcept = default; /**< @brief default copy constructor */
                            subbits            (subbits &&sb) noexcept      = default; /**< @brief default move constructor */
                            subbits& operator= (const subbits &sb) = delete;           /**< @brief delete assignment - assignment to be used with values */
                            subbits& operator= (subbits &&sb)      = delete;           /**< @brief delete assignment - assignment to be used with values */

                            ~subbits () noexcept = default; /**< @brief default destructor */

                            /**
                             * @brief Cast for reading from port bit range as uint64_t.
                             * @return Current value of represented port bit range as uint64_t.
                             */
                            operator uint64_t () noexcept
                            {
                                return stimc_net_get_bits_uint64 (_p._port, _msb, _lsb);
                            }

                            /**
                             * @brief Check for x/z value in comparisons.
                             * @return @ref X if port contains x or z values, @ref not_XZ otherwise.
                             */
                            operator bit () noexcept
                            {
                                if (stimc_net_bits_are_xz (_p._port, _msb, _lsb)) {
                                    return bit::X;
                                } else {
                                    return bit::not_XZ;
                                }
                            }

                            /**
                             * @brief Immediate assignment operator to port bit range.
                             * @param value Value to assgin.
                             * @return reference to the bitrange.
                             *
                             * Sets represented bit range of port to specified
                             * value similar to using a verilog blocking assignment.
                             */
                            subbits& operator= (uint64_t value) noexcept
                            {
                                stimc_net_set_bits_uint64 (_p._port, _msb, _lsb, value);
                                return *this;
                            }

                            /**
                             * @brief Immediate x/z assignment.
                             * @see @ref stimc_net_set_bits_x and @ref stimc_net_set_bits_z.
                             *
                             * Sets represented bit range of port to unknown/high impedance
                             * state similar to using a verilog blocking assignment.
                             */
                            subbits& operator= (bit v) noexcept
                            {
                                if (v == bit::X) {
                                    stimc_net_set_bits_x (_p._port, _msb, _lsb);
                                } else if (v == bit::Z) {
                                    stimc_net_set_bits_z (_p._port, _msb, _lsb);
                                }
                                return *this;
                            }

                            /**
                             * @brief Non-blocking assignment operator to port bit range.
                             * @param value Value to assgin.
                             * @return reference to the bitrange.
                             *
                             * Sets represented bit range of port to specified
                             * value similar to using a verilog non-blocking assignment.
                             *
                             * Reuses the shift-assign operator for being optically similar
                             * to the verilog non-blocking assignment operator <=.
                             * This can cause confusion in cases an actual shift assignment is intended,
                             * but is convenient for the typical stimc scenario of just having
                             * a way of accessing/assigning values of a verilog simulation.
                             */
                            subbits& operator<<= (uint64_t value) noexcept
                            {
                                stimc_net_set_bits_uint64_nonblock (_p._port, _msb, _lsb, value);
                                return *this;
                            }

                            /**
                             * @brief Non-blocking x/z assignment.
                             * @see @ref stimc_net_set_bits_x_nonblock and @ref stimc_net_set_bits_z_nonblock.
                             *
                             * Sets represented bit range of port to unknown/high impedance
                             * state similar to using a verilog non-blocking assignment.
                             */
                            subbits& operator<<= (bit v) noexcept
                            {
                                if (v == bit::X) {
                                    stimc_net_set_bits_x_nonblock (_p._port, _msb, _lsb);
                                } else if (v == bit::Z) {
                                    stimc_net_set_bits_z_nonblock (_p._port, _msb, _lsb);
                                }
                                return *this;
                            }
                    };
                public:
                    /**
                     * @brief Port constructor.
                     * @param m Parent module of port.
                     * @param name Name of the port.
                     */
                    port (module &m, const char *name) noexcept :
                        port_base (m, name)
                    {}

                    port            (const port &p) = delete; /**< @brief Do not copy/change internals */
                    port& operator= (const port &p) = delete; /**< @brief Do not copy/change internals */
                    port            (port &&p)      = delete; /**< @brief Do not move/change internals */
                    port& operator= (port &&p)      = delete; /**< @brief Do not move/change internals */

                    ~port () noexcept = default; /**< @brief default constructor */

                    /**
                     * @brief Immediate assignment operator to port.
                     * @param value Value to assgin.
                     * @return reference to the port.
                     *
                     * Sets port to specified value similar
                     * to using a verilog blocking assignment.
                     */
                    port& operator= (uint64_t value) noexcept
                    {
                        stimc_net_set_uint64 (_port, value);
                        return *this;
                    }

                    /**
                     * @brief Immediate x/z assignment.
                     * @see @ref stimc_net_set_x and @ref stimc_net_set_z.
                     *
                     * Sets port to unknown/high impedance state similar
                     * to using a verilog blocking assignment.
                     */
                    port& operator= (bit v) noexcept
                    {
                        if (v == bit::X) {
                            stimc_net_set_x (_port);
                        } else if (v == bit::Z) {
                            stimc_net_set_z (_port);
                        }
                        return *this;
                    }

                    /**
                     * @brief Non-blocking assignment operator to port.
                     * @param value Value to assgin.
                     * @return reference to the port.
                     *
                     * Sets port to specified value similar
                     * to using a verilog non-blocking assignment.
                     *
                     * Reuses the shift-assign operator for being optically similar
                     * to the verilog non-blocking assignment operator <=.
                     * This can cause confusion in cases an actual shift assignment is intended,
                     * but is convenient for the typical stimc scenario of just having
                     * a way of accessing/assigning values of a verilog simulation.
                     */
                    port& operator<<= (uint64_t value) noexcept
                    {
                        stimc_net_set_uint64_nonblock (_port, value);
                        return *this;
                    }

                    /**
                     * @brief Non-blocking x/z assignment.
                     * @see @ref stimc_net_set_x_nonblock and @ref stimc_net_set_z_nonblock.
                     *
                     * Sets port to unknown/high impedance state similar
                     * to using a verilog non-blocking assignment.
                     */
                    port& operator<<= (bit v) noexcept
                    {
                        if (v == bit::X) {
                            stimc_net_set_x_nonblock (_port);
                        } else if (v == bit::Z) {
                            stimc_net_set_z_nonblock (_port);
                        }
                        return *this;
                    }

                    /**
                     * @brief Cast for reading from port as uint64_t.
                     * @return Current value of port as uint64_t.
                     *
                     * In cases uint64_t is not sufficient try using
                     * @ref subbits.
                     */
                    operator uint64_t () noexcept
                    {
                        return stimc_net_get_uint64 (_port);
                    }

                    /**
                     * @brief Check for x/z value in comparisons.
                     * @return @ref X if port contains x or z values, @ref not_XZ otherwise.
                     */
                    operator bit () noexcept
                    {
                        if (stimc_net_is_xz (_port)) {
                            return bit::X;
                        } else {
                            return bit::not_XZ;
                        }
                    }

                    /**
                     * @brief Optain a new bit range handle to the port.
                     * @param msb Most significant bit of bit range.
                     * @param lsb Least significant bit of bit range.
                     * @return the subbits handle.
                     */
                    subbits operator() (int msb, int lsb) noexcept
                    {
                        subbits b (*this, msb, lsb);

                        return b;
                    }

                    /**
                     * @brief Optain a new bit handle to the port.
                     * @param bit bit of handle.
                     * @return the subbits handle.
                     */
                    subbits operator() (int bit) noexcept
                    {
                        subbits b (*this, bit, bit);

                        return b;
                    }
            };

            /**
             * @brief Wrapper class for @ref stimc_port for real values.
             */
            class port_real : public port_base {
                public:
                    /**
                     * @brief Port constructor.
                     * @param m Parent module of port.
                     * @param name Name of the port.
                     */
                    port_real (module &m, const char *name) noexcept :
                        port_base (m, name)
                    {}

                    port_real            (const port_real &p) = delete; /**< @brief Do not copy/change internals */
                    port_real& operator= (const port_real &p) = delete; /**< @brief Do not copy/change internals */
                    port_real            (port_real &&p)      = delete; /**< @brief Do not move/change internals */
                    port_real& operator= (port_real &&p)      = delete; /**< @brief Do not move/change internals */

                    ~port_real () noexcept = default; /**< @brief default constructor */

                    /**
                     * @brief Immediate assignment operator to port.
                     * @param value Value to assgin.
                     * @return reference to the port.
                     *
                     * Sets port to specified value similar
                     * to using a verilog blocking assignment.
                     */
                    port_real& operator= (double value) noexcept
                    {
                        stimc_net_set_double (_port, value);
                        return *this;
                    }

                    /**
                     * @brief Immediate x/z assignment.
                     * @see @ref stimc_net_set_x and @ref stimc_net_set_z.
                     *
                     * Sets port to unknown/high impedance state similar
                     * to using a verilog blocking assignment.
                     */
                    port_real& operator= (bit v) noexcept
                    {
                        if (v == bit::X) {
                            stimc_net_set_x (_port);
                        } else if (v == bit::Z) {
                            stimc_net_set_z (_port);
                        }
                        return *this;
                    }

                    /**
                     * @brief Non-blocking assignment operator to port.
                     * @param value Value to assgin.
                     * @return reference to the port.
                     *
                     * Sets port to specified value similar
                     * to using a verilog non-blocking assignment.
                     */
                    port_real& operator<<= (double value) noexcept
                    {
                        stimc_net_set_double_nonblock (_port, value);
                        return *this;
                    }

                    /**
                     * @brief Non-blocking x/z assignment.
                     * @see @ref stimc_net_set_x_nonblock and @ref stimc_net_set_z_nonblock.
                     *
                     * Sets port to unknown/high impedance state similar
                     * to using a verilog non-blocking assignment.
                     */
                    port_real& operator<<= (bit v) noexcept
                    {
                        if (v == bit::X) {
                            stimc_net_set_x_nonblock (_port);
                        } else if (v == bit::Z) {
                            stimc_net_set_z_nonblock (_port);
                        }
                        return *this;
                    }

                    /**
                     * @brief Cast for reading from port as uint64_t.
                     * @return Current value of port as uint64_t.
                     */
                    operator double () noexcept
                    {
                        return stimc_net_get_double (_port);
                    }

                    /**
                     * @brief Check for x/z value in comparisons.
                     * @return @ref X if port contains x or z values, @ref not_XZ otherwise.
                     */
                    operator bit () noexcept
                    {
                        if (stimc_net_is_xz (_port)) {
                            return bit::X;
                        } else {
                            return bit::not_XZ;
                        }
                    }
            };

            /**
             * @brief Wrapper class for @ref stimc_parameter.
             */
            class parameter {
                protected:
                    stimc_parameter _parameter; /**<@brief The wrapped @ref stimc_parameter. */
                    int32_t _value_i;           /**<@brief Cached integer value of parameter. */
                    double _value_d;            /**<@brief Cached double value of parameter. */
                public:
                    /**
                     * @brief Parameter constructor.
                     * @param m Parent module of parameter.
                     * @param name Name of the parameter.
                     */
                    parameter (module &m, const char *name) noexcept :
                        _parameter (stimc_parameter_init (&(m._module), name)),
                        _value_i (0),
                        _value_d (0.0)
                    {
                        if (stimc_parameter_get_format (this->_parameter) == vpiRealVal) {
                            this->_value_d = stimc_parameter_get_double (this->_parameter);
                            this->_value_i = this->_value_d;
                        } else {
                            this->_value_i = stimc_parameter_get_int32 (this->_parameter);
                            this->_value_d = this->_value_i;
                        }
                    }

                    parameter            (const parameter &p) = delete; /**< @brief Do not copy/change internals */
                    parameter& operator= (const parameter &p) = delete; /**< @brief Do not copy/change internals */
                    parameter            (parameter &&p)      = delete; /**< @brief Do not move/change internals */
                    parameter& operator= (parameter &&p)      = delete; /**< @brief Do not move/change internals */

                    /**
                     * @brief Simple destructor
                     */
                    ~parameter () noexcept
                    {
                        stimc_parameter_free (this->_parameter);
                    }

                    /**
                     * @brief Integer value of parameter.
                     * @return Integer value.
                     */
                    int value () noexcept
                    {
                        return _value_i;
                    }

                    /**
                     * @brief Double value of parameter.
                     * @return Double value.
                     */
                    double dvalue () noexcept
                    {
                        return _value_d;
                    }

                    /**
                     * @brief Integer value of parameter.
                     * @return Integer value.
                     */
                    operator uint64_t () noexcept
                    {
                        return _value_i;
                    }

                    /**
                     * @brief Integer value of parameter.
                     * @return Integer value.
                     */
                    operator uint32_t () noexcept
                    {
                        return _value_i;
                    }

                    /**
                     * @brief Double value of parameter.
                     * @return Double value.
                     */
                    operator double () noexcept
                    {
                        return _value_d;
                    }
            };
    };

    /**
     * @brief Inline wait wrapper.
     * @param time_seconds Amount of time in seconds.
     * Calls @ref stimc_wait_time_seconds.
     */
    static inline void wait (double time_seconds)
    {
        stimc_wait_time_seconds (time_seconds);
        thread_finish_check ();
    }

    /**
     * @brief Inline wait wrapper.
     * @param time Amount of time in unit specified by @c exp.
     * @param exp Time unit (e.g. SC_US).
     * Calls @ref stimc_wait_time.
     */
    static inline void wait (uint64_t time, enum stimc_time_unit exp)
    {
        stimc_wait_time (time, exp);
        thread_finish_check ();
    }

    /**
     * @brief Inline wait wrapper.
     * @param e Event to wait for.
     * Calls @ref event::wait.
     */
    static inline void wait (event &e)
    {
        e.wait ();
    }

    /**
     * @brief Inline wait wrapper.
     * @param e Event to wait for.
     * @param time_seconds Amount of time in seconds for timeout.
     * @return true in case of timeout.
     * Calls @ref event::wait.
     */
    static inline bool wait (event &e, double time_seconds)
    {
        return e.wait (time_seconds);
    }

    /**
     * @brief Inline wait wrapper.
     * @param e Event to wait for.
     * @param time Amount of time in unit specified by @c exp for timeout.
     * @param exp Time unit (e.g. SC_US).
     * @return true in case of timeout.
     * Calls @ref event::wait.
     */
    static inline bool wait (event &e, uint64_t time, enum stimc_time_unit exp)
    {
        return e.wait (time, exp);
    }

    /**
     * @brief Inline wait wrapper, rvalue version.
     * @param ec Event combination to wait for.
     * Calls @ref event_combination::wait.
     */
    static inline void wait (event_combination &&ec)
    {
        std::move (ec).wait ();
    }

    /**
     * @brief Inline wait wrapper, rvalue version.
     * @param ec Event combination to wait for.
     * @param time_seconds Amount of time in seconds for timeout.
     * @return true in case of timeout.
     * Calls @ref event_combination::wait.
     */
    static inline bool wait (event_combination &&ec, double time_seconds)
    {
        return std::move (ec).wait (time_seconds);
    }

    /**
     * @brief Inline wait wrapper, rvalue version.
     * @param ec Event combination to wait for.
     * @param time Amount of time in unit specified by @c exp for timeout.
     * @param exp Time unit (e.g. SC_US).
     * @return true in case of timeout.
     * Calls @ref event_combination::wait.
     */
    static inline bool wait (event_combination &&ec, uint64_t time, enum stimc_time_unit exp)
    {
        return std::move (ec).wait (time, exp);
    }

    /**
     * @brief Inline wait wrapper.
     * @param ec Event combination to wait for.
     * Calls @ref event_combination::wait.
     */
    static inline void wait (const event_combination &ec)
    {
        ec.wait ();
    }

    /**
     * @brief Inline wait wrapper.
     * @param ec Event combination to wait for.
     * @param time_seconds Amount of time in seconds for timeout.
     * @return true in case of timeout.
     * Calls @ref event_combination::wait.
     */
    static inline bool wait (const event_combination &ec, double time_seconds)
    {
        return ec.wait (time_seconds);
    }

    /**
     * @brief Inline wait wrapper.
     * @param ec Event combination to wait for.
     * @param time Amount of time in unit specified by @c exp for timeout.
     * @param exp Time unit (e.g. SC_US).
     * @return true in case of timeout.
     * Calls @ref event_combination::wait.
     */
    static inline bool wait (const event_combination &ec, uint64_t time, enum stimc_time_unit exp)
    {
        return ec.wait (time, exp);
    }

    /**
     * @brief Inline halt wrapper.
     * Calls @ref stimc_thread_halt.
     */
    static inline void halt (void)
    {
        stimc_thread_halt ();
        thread_finish_check ();
    }

    /**
     * @brief Inline exit wrapper.
     * Calls @ref stimc_thread_exit.
     */
    static inline void exit (void)
    {
        stimc_thread_exit ();
        thread_finish_check ();
    }

    /**
     * @brief Inline simulation time wrapper.
     * @return Simulation time.
     * Calls @ref stimc_time_seconds.
     */
    static inline double time () noexcept
    {
        return stimc_time_seconds ();
    }

    /**
     * @brief Inline simulation time wrapper.
     * @param exp Time unit (e.g. @c SC_US).
     * Calls @ref stimc_time.
     * @return Simulation time.
     */
    static inline uint64_t time (enum stimc_time_unit exp) noexcept
    {
        return stimc_time (exp);
    }

    /**
     * @brief Inline finish wrapper.
     * Calls @ref stimc_finish.
     */
    static inline void finish ()
    {
        stimc_finish ();
        thread_finish_check ();
    }

    /**
     * @brief Helper base class for end-of-thread cleanup functionality.
     *
     * Important: Derived objects must be allocated by new,
     * not on the stack, as they will be deleted on end of simulation
     * and must persist even in case the function in which they are created
     * terminates.
     *
     * Usage: derive a class, create member variables for data
     * to clean up at end of thread and provide cleanup functionality
     * in derived class's destructor.
     *
     * Derived objects can only be created within a stimc thread,
     * as they will register their destruction in @ref cleanup_callback
     * as stimc end of thread cleanup callback via @ref stimc_register_thread_cleanup.
     *
     * Take care of scenarios where cleanup is not yet or no longer necessary
     * (e.g. data not yet created or already cleaned up by the thread).
     * E.g. set a pointer to NULL to indicate no cleanup is necessary.
     */
    class thread_cleanup {
        protected:
            /**
             * @brief The actual cleanup callback function.
             * @param cleanup_data casted pointer to @ref thread_cleanup derived object.
             *
             * Will delete the specified object.
             */
            static void cleanup_callback (void *cleanup_data)
            {
                thread_cleanup *cleanup = (thread_cleanup *)cleanup_data;

                delete cleanup;
            }

        protected:
            thread_cleanup () noexcept
            {
                stimc_register_thread_cleanup (thread_cleanup::cleanup_callback, this);
            }

            virtual ~thread_cleanup () = default; /**< @brief Ensure derived destructor is called. */

            thread_cleanup            (const thread_cleanup &t) = delete; /**< @brief Do not copy/change internals */
            thread_cleanup& operator= (const thread_cleanup &t) = delete; /**< @brief Do not copy/change internals */
            thread_cleanup            (thread_cleanup &&t)      = delete; /**< @brief Do not move/change internals */
            thread_cleanup& operator= (thread_cleanup &&t)      = delete; /**< @brief Do not move/change internals */
    };
}

/**
 * @brief Parameter initialization for module constructor initializer list.
 * @param param Parameter to initialize.
 *
 * Will take care of proper constructor call for @ref stimcxx::module::parameter.
 */
#define STIMCXX_PARAMETER(param) \
    param (*this, #param)

/**
 * @brief Port initialization for module constructor initializer list.
 * @param port Port to initialize.
 *
 * Will take care of proper constructor call for @ref stimcxx::module::port.
 */
#define STIMCXX_PORT(port) \
    port (*this, #port)

/**
 * @brief Convenience wrapper for registering a method as startup thread with specified stacksize.
 * @param thread The method to register.
 * @param stacksize The size of the thread's stack.
 *
 * Wraps @ref stimc_register_startup_thread for registering
 * a method with no parameters as startup thread.
 */
#define STIMCXX_REGISTER_STARTUP_THREAD_STACKSIZE(thread, stacksize) \
    typedef decltype (this) _thisptype; \
    class _stimcxx_thread_init_ ## thread { \
        public: \
            static void callback (void *p) { \
                _thisptype m = (_thisptype)p; \
                stimc_thread_resume_on_finish (stimcxx::enable_stack_unwind); \
                try { \
                    m->thread (); \
                } catch (stimcxx::thread_finish_exception &e) {} \
            } \
    }; \
    stimc_register_startup_thread (_stimcxx_thread_init_ ## thread::callback, (void *)this, stacksize)

/**
 * @brief Convenience wrapper for registering a method as startup thread.
 * @param thread The method to register.
 *
 * Wraps @ref stimc_register_startup_thread for registering
 * a method with no parameters as startup thread.
 */
#define STIMCXX_REGISTER_STARTUP_THREAD(thread) \
    STIMCXX_REGISTER_STARTUP_THREAD_STACKSIZE (thread, 0)

/**
 * @brief Convenience wrapper for registering a method as event method.
 * @param event The event to wait for (one of posedge/negedge/change).
 * @param port The module port the event will be registered for.
 * @param func the method to call for the specified event.
 *
 * Wraps @ref stimc_register_posedge_method, @ref stimc_register_negedge_method
 * and @ref stimc_register_change_method for given port and function.
 */
#define STIMCXX_REGISTER_METHOD(event, port, func) \
    typedef decltype (this) _thisptype; \
    class _stimcxx_method_init_ ## event ## _ ## port ## _ ## func { \
        public: \
            static void callback (void *p) { \
                _thisptype m = (_thisptype)p; \
                m->func (); \
            } \
    }; \
    port.register_ ## event ## _method (_stimcxx_method_init_ ## event ## _ ## port ## _ ## func ::callback, (void *)this)


/**
 * @brief stimc++ module initialization routine macro.
 *
 * Use to define the initialization code for a stimc++ module via
 * \code{.cpp}
 * STIMCXX_INIT (modulename)
 * \endcode
 *
 * stimc++ pendant of @ref STIMC_INIT. @c modulename must be
 * the name of the child class of @ref stimcxx::module.
 * will be indirectly used by @ref STIMCXX_EXPORT.
 */
#define STIMCXX_INIT(module) \
    static void _stimcxx_module_ ## module ## _init () \
    { \
        module *m __attribute__((unused)); \
        m = new module (); \
    } \
    \
    extern "C" { \
    void _stimc_module_ ## module ## _register (void) \
    { \
        stimc_module_register (#module, _stimcxx_module_ ## module ## _init); \
    } \
    }

/**
 * @brief stimc++ module initialization routine export macro.
 *
 * Used to define the initialization code for a stimc++ module via
 * @ref STIMCXX_INIT and register the necessary functions for loading
 * with the vpi library.
 */
#define STIMCXX_EXPORT(module) \
    STIMCXX_INIT (module) \
    \
    static stimc_vpi_init_register_s _stimc_module_ ## module ## _vpi_init_s_ = {_stimc_module_ ## module ## _register, NULL}; \
    \
    static void _stimc_module_ ## module ## _vpi_init_f_ (void) \
    { \
        stimc_vpi_init_register (&_stimc_module_ ## module ## _vpi_init_s_); \
    } \
    static stimc_vpi_init_register_func_t _stimc_module_ ## module ## _do_export_ __attribute__((__used__, section (".init_array"))) = _stimc_module_ ## module ## _vpi_init_f_;

#endif

