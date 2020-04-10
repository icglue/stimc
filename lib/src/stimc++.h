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

#ifndef __STIMCXX_H__
#define __STIMCXX_H__

#include <stimc.h>

/**
 * @brief stimc++ namespace.
 */
namespace stimcxx {
    /**
     * @brief Wrapper class for @ref stimc_event and related functionality.
     */
    class event {
        private:
            stimc_event _event; /**< @brief The actual @ref stimc_event. */
        public:
            event ();
            virtual ~event ();

            /**
             * @brief Wait for event to be triggered.
             *
             * Inline wrapper for @ref stimc_wait_event.
             */
            void wait ()
            {
                stimc_wait_event (_event);
            }

            /**
             * @brief Trigger event.
             *
             * Inline wrapper for @ref stimc_trigger_event.
             */
            void trigger ()
            {
                stimc_trigger_event (_event);
            }
    };


    /**
     * @brief Convenience type to be able to assign x/z values.
     *
     * Not directly mapped to an enum to prevent ambiguity with
     * integer types.
     */
    class bit {
        protected:
            /**
             * @brief The actual representation of x/z/none of them.
             */
            enum val {
                val_X,      /**< @brief x value (verilog-x ~ unknown) */
                val_Z,      /**< @brief z value (verilog-z ~ high impedance) */
                val_not_XZ, /**< @brief not x/z for comparison */
            };

            enum val _v; /**< @brief The actual value */

            /**
             * @brief Non-public constructor to prevent ambiguity
             * @param v The represented value.
             */
            bit (val v) : _v (v) {}
        public:
            /**
             * @brief Comparison.
             * @param b bit to compare against.
             * @return true if equal.
             * @see @ref module::port::operator==
             */
            bool operator== (const bit &b) const
            {
                return (b._v == this->_v);
            }

            /**
             * @brief Comparison.
             * @param b bit to compare against.
             * @return true if non-equal.
             * @see @ref module::port::operator!=
             */
            bool operator!= (const bit &b) const
            {
                return (b._v != this->_v);
            }

            /**
             * @brief bit representing x value */
            static bit X ()      {return bit (val_X);}
            static bit Z ()      {return bit (val_Z);}
            static bit not_XZ () {return bit (val_not_XZ);}
    };

    static const bit X      = bit::X ();      /**< @brief x value (verilog-x ~ unknown) */
    static const bit Z      = bit::Z ();      /**< @brief z value (verilog-z ~ high impedance) */
    static const bit not_XZ = bit::not_XZ (); /**< @brief not x/z for comparison */


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
            static void cleanup (void *m);

        public:
            module ();
            virtual ~module ();

            /**
             * @brief Get hierarchical identifier for instance of module.
             * @return The verilog hierarchy of module instance.
             */
            const char *module_id ()
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
                     * @brief Port constructor.
                     * @param m Parent module of port.
                     * @param name Name of the port.
                     */
                    port_base (module &m, const char *name);

                    port_base            (const port_base &p) = delete; /**< @brief Do not copy/change internals */
                    port_base& operator= (const port_base &p) = delete; /**< @brief Do not copy/change internals */

                    ~port_base ();

                    /**
                     * @brief Register a callback method for posedge events at port.
                     * @brief callback Method to register.
                     * @brief p Data pointer for callback.
                     *
                     * Internal helper. For registering from inside a module
                     * use @ref STIMCXX_REGISTER_METHOD for convenience.
                     */
                    void register_posedge_method (void (*callback)(void *p), void *p)
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
                    void register_negedge_method (void (*callback)(void *p), void *p)
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
                    void register_change_method (void (*callback)(void *p), void *p)
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
                            subbits (port &p, int msb, int lsb) :
                                _lsb (lsb), _msb (msb), _p (p) {}

                            /**
                             * @brief Cast for reading from port bit range as uint64_t.
                             * @return Current value of represented port bit range as uint64_t.
                             */
                            operator uint64_t ()
                            {
                                return stimc_net_get_bits_uint64 (_p._port, _msb, _lsb);
                            }

                            /**
                             * @brief Check for x/z value in comparisons.
                             * @return @ref X if port contains x or z values, @ref not_XZ otherwise.
                             */
                            operator bit ()
                            {
                                if (stimc_net_bits_are_xz (_p._port, _msb, _lsb)) {
                                    return X;
                                } else {
                                    return not_XZ;
                                }
                            }

                            /**
                             * @brief Compare against x value.
                             * @param b Value to compare against (typically @ref X).
                             * @return true if equal, false otherwise.
                             * @see @ref operator bit().
                             */
                            bool operator== (const bit &b) {return ((bit)(*this) == b);}

                            /**
                             * @brief Compare against x value.
                             * @param b Value to compare against (typically @ref X).
                             * @return true if equal, false otherwise.
                             * @see @ref operator bit().
                             */
                            bool operator!= (const bit &b) {return ((bit)(*this) != b);}

                            /**
                             * @brief Immediate assignment operator to port bit range.
                             * @param value Value to assgin.
                             * @return reference to the bitrange.
                             *
                             * Sets represented bit range of port to specified
                             * value similar to using a verilog blocking assignment.
                             */
                            subbits& operator= (uint64_t value)
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
                            subbits& operator= (bit v)
                            {
                                if (v == X) {
                                    stimc_net_set_bits_x (_p._port, _msb, _lsb);
                                } else if (v == Z) {
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
                            subbits& operator<<= (uint64_t value)
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
                            subbits& operator<<= (bit v)
                            {
                                if (v == X) {
                                    stimc_net_set_bits_x_nonblock (_p._port, _msb, _lsb);
                                } else if (v == Z) {
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
                    port (module &m, const char *name);

                    port            (const port &p) = delete; /**< @brief Do not copy/change internals */
                    port& operator= (const port &p) = delete; /**< @brief Do not copy/change internals */


                    /**
                     * @brief Immediate assignment operator to port.
                     * @param value Value to assgin.
                     * @return reference to the port.
                     *
                     * Sets port to specified value similar
                     * to using a verilog blocking assignment.
                     */
                    port& operator= (uint64_t value)
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
                    port& operator= (bit v)
                    {
                        if (v == X) {
                            stimc_net_set_x (_port);
                        } else if (v == Z) {
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
                    port& operator<<= (uint64_t value)
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
                    port& operator<<= (bit v)
                    {
                        if (v == X) {
                            stimc_net_set_x_nonblock (_port);
                        } else if (v == Z) {
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
                    operator uint64_t ()
                    {
                        return stimc_net_get_uint64 (_port);
                    }

                    /**
                     * @brief Check for x/z value in comparisons.
                     * @return @ref X if port contains x or z values, @ref not_XZ otherwise.
                     */
                    operator bit ()
                    {
                        if (stimc_net_is_xz (_port)) {
                            return X;
                        } else {
                            return not_XZ;
                        }
                    }

                    /**
                     * @brief Compare against x value.
                     * @param b Value to compare against (typically @ref X).
                     * @return true if equal, false otherwise.
                     * @see @ref operator bit().
                     */
                    bool operator== (const bit &b) {return ((bit)(*this) == b);}

                    /**
                     * @brief Compare against x value.
                     * @param b Value to compare against (typically @ref X).
                     * @return true if equal, false otherwise.
                     * @see @ref operator bit().
                     */
                    bool operator!= (const bit &b) {return ((bit)(*this) != b);}

                    /**
                     * @brief Optain a new bit range handle to the port.
                     * @param msb Most significant bit of bit range.
                     * @param lsb Least significant bit of bit range.
                     * @return the subbits handle.
                     */
                    subbits operator() (int msb, int lsb)
                    {
                        subbits b (*this, msb, lsb);

                        return b;
                    }

                    /**
                     * @brief Optain a new bit handle to the port.
                     * @param bit bit of handle.
                     * @return the subbits handle.
                     */
                    subbits operator() (int bit)
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
                    port_real (module &m, const char *name);

                    port_real            (const port_real &p) = delete; /**< @brief Do not copy/change internals */
                    port_real& operator= (const port_real &p) = delete; /**< @brief Do not copy/change internals */


                    /**
                     * @brief Immediate assignment operator to port.
                     * @param value Value to assgin.
                     * @return reference to the port.
                     *
                     * Sets port to specified value similar
                     * to using a verilog blocking assignment.
                     */
                    port_real& operator= (double value)
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
                    port_real& operator= (bit v)
                    {
                        if (v == X) {
                            stimc_net_set_x (_port);
                        } else if (v == Z) {
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
                    port_real& operator<<= (double value)
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
                    port_real& operator<<= (bit v)
                    {
                        if (v == X) {
                            stimc_net_set_x_nonblock (_port);
                        } else if (v == Z) {
                            stimc_net_set_z_nonblock (_port);
                        }
                        return *this;
                    }

                    /**
                     * @brief Cast for reading from port as uint64_t.
                     * @return Current value of port as uint64_t.
                     */
                    operator double ()
                    {
                        return stimc_net_get_double (_port);
                    }

                    /**
                     * @brief Check for x/z value in comparisons.
                     * @return @ref X if port contains x or z values, @ref not_XZ otherwise.
                     */
                    operator bit ()
                    {
                        if (stimc_net_is_xz (_port)) {
                            return X;
                        } else {
                            return not_XZ;
                        }
                    }

                    /**
                     * @brief Compare against x value.
                     * @param b Value to compare against (typically @ref X).
                     * @return true if equal, false otherwise.
                     * @see @ref operator bit().
                     */
                    bool operator== (const bit &b) {return ((bit)(*this) == b);}

                    /**
                     * @brief Compare against x value.
                     * @param b Value to compare against (typically @ref X).
                     * @return true if equal, false otherwise.
                     * @see @ref operator bit().
                     */
                    bool operator!= (const bit &b) {return ((bit)(*this) != b);}
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
                    parameter (module &m, const char *name);

                    parameter            (const parameter &p) = delete; /**< @brief Do not copy/change internals */
                    parameter& operator= (const parameter &p) = delete; /**< @brief Do not copy/change internals */

                    ~parameter ();

                    /**
                     * @brief Integer value of parameter.
                     * @return Integer value.
                     */
                    int value ()
                    {
                        return _value_i;
                    }

                    /**
                     * @brief Double value of parameter.
                     * @return Double value.
                     */
                    double dvalue ()
                    {
                        return _value_d;
                    }

                    /**
                     * @brief Integer value of parameter.
                     * @return Integer value.
                     */
                    operator uint64_t ()
                    {
                        return _value_i;
                    }

                    /**
                     * @brief Integer value of parameter.
                     * @return Integer value.
                     */
                    operator uint32_t ()
                    {
                        return _value_i;
                    }

                    /**
                     * @brief Double value of parameter.
                     * @return Double value.
                     */
                    operator double ()
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
     * @brief Inline simulation time wrapper.
     * @return Simulation time.
     * Calls @ref stimc_time_seconds.
     */
    static inline double time ()
    {
        return stimc_time_seconds ();
    }

    /**
     * @brief Inline simulation time wrapper.
     * @param exp Time unit (e.g. @c SC_US).
     * Calls @ref stimc_time.
     * @return Simulation time.
     */
    static inline uint64_t time (enum stimc_time_unit exp)
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
            thread_cleanup ();
            virtual ~thread_cleanup ();

            thread_cleanup            (const thread_cleanup &t) = delete; /**< @brief Do not copy/change internals */
            thread_cleanup& operator= (const thread_cleanup &t) = delete; /**< @brief Do not copy/change internals */
        protected:
            /**
             * @brief The actual cleanup callback function.
             * @param cleanup_data casted pointer to @ref thread_cleanup derived object.
             *
             * Will delete the specified object.
             */
            static void cleanup_callback (void *cleanup_data);
    };
};

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
                m->thread (); \
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
 * {
 *     // body
 * }
 * \endcode
 *
 * stimc++ pendant of @ref STIMC_INIT. @c modulename must be
 * the name of the child class of @ref stimcxx::module.
 */
#define STIMCXX_INIT(module) \
    static int _stimcxx_module_ ## module ## _init_cptf (PLI_BYTE8 * user_data __attribute__((unused))) \
    { \
        return 0; \
    } \
    \
    static int _stimcxx_module_ ## module ## _init_cltf (PLI_BYTE8 * user_data __attribute__((unused))) \
    { \
        module *m __attribute__((unused)); \
        m = new module (); \
    \
        return 0; \
    } \
    \
    extern "C" { \
    void _stimc_module_ ## module ## _register (void) \
    { \
        s_vpi_systf_data tf_data; \
        static char      tf_name[] = "$stimc_" #module "_init"; \
        \
        tf_data.type      = vpiSysTask; \
        tf_data.tfname    = tf_name; \
        tf_data.calltf    = _stimcxx_module_ ## module ## _init_cltf; \
        tf_data.compiletf = _stimcxx_module_ ## module ## _init_cptf; \
        tf_data.sizetf    = 0; \
        tf_data.user_data = NULL; \
        \
        vpi_register_systf (&tf_data); \
    } \
    }

#define STIMCXX_EXPORT(module) \
    STIMCXX_INIT (module) \
    void _stimc_export_module_ ## module (void) \
    { \
        stimc_register_module (&_stimc_module_ ## module ## _register); \
    } \
    void (*_stimc_do_export_module_ ## module)(void) __attribute__((__used__, section (".init_array"))) = _stimc_export_module_ ## module;
#endif

