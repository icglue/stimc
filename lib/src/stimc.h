/*
 *  stimc is a lightweight verilog-vpi wrapper for stimuli generation.
 *  Copyright (C) 2019-2022  Andreas Dixius, Felix Neumärker
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
 * @brief stimc core.
 */

#ifndef STIMC_H
#define STIMC_H

#include <vpi_user.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
/* *auto-indent-off* */
extern "C" {
/* *auto-indent-on* */
#endif

/******************************************************************************************************/
/* version */
/******************************************************************************************************/
extern const unsigned stimc_version_major; /**< @brief stimc major version number */
extern const unsigned stimc_version_minor; /**< @brief stimc minor version number */
extern const char    *stimc_version;       /**< @brief stimc version as string of form "<major>.<minor>" */

/******************************************************************************************************/
/* ports/nets/parameters */
/******************************************************************************************************/

/**
 * @brief Port/net base type.
 *
 * Currently both of them are accessed via the same type as vpi does not need to distinguish.
 */
struct stimc_net_s {
    vpiHandle net;              /**< @brief vpi handle for access to net/port object */

    struct stimc_nba_data_s *nba; /**< @brief Data for scheduled non-blocking assignments */
};
typedef struct stimc_net_s *stimc_net;  /**< @brief Net base type. */
typedef struct stimc_net_s *stimc_port; /**< @brief Port base type. */

/**
 * @brief Parameter base type.
 */
typedef vpiHandle stimc_parameter;


/******************************************************************************************************/
/* methods/threads */
/******************************************************************************************************/

/**
 * @brief Register a callback at posedge event on specified net/port.
 * @param methodfunc Callback function accepting a single pointer as argument.
 * @param userdata Data argument to be handed to methodfunc on call.
 * @param net @ref stimc_net to observe for event.
 *
 * The callback function will be called from simulator context and not in a separate thread.
 * This means it cannot make use of wait functions. In case you need to combine an event callback
 * with a thread you might use a separate thread waiting for an @ref stimc_event triggered by the
 * callback method.
 */
void stimc_register_posedge_method (void (*methodfunc)(void *userdata), void *userdata, stimc_net net);

/**
 * @brief Register a callback at negedge event on specified net/port.
 * @param methodfunc Callback function accepting a single pointer as argument.
 * @param userdata Data argument to be handed to methodfunc on call.
 * @param net @ref stimc_net to observe for event.
 *
 * The callback function will be called from simulator context and not in a separate thread.
 * This means it cannot make use of wait functions. In case you need to combine an event callback
 * with a thread you might use a separate thread waiting for an @ref stimc_event triggered by the
 * callback method.
 */
void stimc_register_negedge_method (void (*methodfunc)(void *userdata), void *userdata, stimc_net net);

/**
 * @brief Register a callback at value change event on specified net/port.
 * @param methodfunc Callback function accepting a single pointer as argument.
 * @param userdata Data argument to be handed to methodfunc on call.
 * @param net @ref stimc_net to observe for event.
 *
 * The callback function will be called from simulator context and not in a separate thread.
 * This means it cannot make use of wait functions. In case you need to combine an event callback
 * with a thread you might use a separate thread waiting for an @ref stimc_event triggered by the
 * callback method.
 */
void stimc_register_change_method (void (*methodfunc)(void *userdata), void *userdata, stimc_net net);

/**
 * @brief Register a function to be started as thread on simulation start.
 * @param threadfunc Callback function accepting a single pointer as argument.
 * @param userdata Data argument to be handed to threadfunc on call.
 * @param stacksize Size of the thread's stack. Can be 0 (will use a default size then).
 *
 * The callback function will be started as a stimc thread at simulation start.
 * As separate thread it can be suspended and resumed via the different wait functions.
 *
 * The stacksize must be large enough for the callstack of the created thread.
 * Not all thread backends will use this stacksize, some support growing stacks.
 *
 * @deprecated Is subsumed into @ref stimc_spawn_thread.
 */
void stimc_register_startup_thread (void (*threadfunc)(void *userdata), void *userdata, size_t stacksize)
    __attribute__((deprecated("use stimc_spawn_thread")));

/**
 * @brief Enqueue a function to be started as thread.
 * @param threadfunc Callback function accepting a single pointer as argument.
 * @param userdata Data argument to be handed to threadfunc on call.
 * @param stacksize Size of the thread's stack. Can be 0 (will use a default size then).
 *
 * The callback function will be started as a stimc thread together with other
 * threads within the current simulation time step but typically not directly
 * after spawning it (other threads might be run before and the spawning
 * function must either return or suspend in case it is run as a thread itself).
 * As separate thread it can be suspended and resumed via the different wait functions.
 *
 * The stacksize must be large enough for the callstack of the created thread.
 * Not all thread backends will use this stacksize, some support growing stacks.
 */
void stimc_spawn_thread (void (*threadfunc)(void *userdata), void *userdata, size_t stacksize);

/**
 * @brief Register a function to be called when the current thread is terminated.
 * @param cleanfunc Callback function accepting a single pointer as argument.
 * @param userdata Data argument to be handed to cleanfunc on call.
 *
 * The callback function will be run when the thread is removed (either because it finished
 * or because simulation finished). Make sure that it can be called at any point within the
 * thread where it is suspended and also after it finishes.
 * @ref stimc_register_thread_cleanup can only be called from within the thread.
 */
void stimc_register_thread_cleanup (void (*cleanfunc)(void *userdata), void *userdata);


/******************************************************************************************************/
/* thread suspension */
/******************************************************************************************************/

/**
 * @brief Halt current thread.
 *
 * This will suspend the current stimc thread permanently.
 */
void stimc_thread_halt (void);

/**
 * @brief Terminate current thread.
 *
 * This will suspend the current stimc thread and free it's resources.
 */
void stimc_thread_exit (void);

/**
 * @brief Configure current thread to resume operation on finish for cleanup.
 * @param resume true: resume thread from waiting when finished,
 *               false (default): keep thread suspendet when finished.
 *
 * By default a finished thread is just cleaned up according to the
 * coroutine library used. In case it should be resumed on finishing
 * for cleanup purposes, this can be configured to true.
 *
 * If used, any wait function or @ref stimc_thread_halt,
 * @ref stimc_thread_exit and @ref stimc_finish will return when finishing the
 * thread. This should then be checked with @ref stimc_thread_is_finished
 * to do the intended cleanup.
 */
void stimc_thread_resume_on_finish (bool resume);

/**
 * @brief Check if current thread is already finished.
 * @return true if thread is finished, false otherwise.
 *
 * This is intended to be used in combination with
 * @ref stimc_thread_resume_on_finish set to true to check for the
 * final return on wait functions and @ref stimc_thread_halt,
 * @ref stimc_thread_exit and @ref stimc_finish.
 */
bool stimc_thread_is_finished (void);


/******************************************************************************************************/
/* time/wait */
/******************************************************************************************************/

/**
 * @brief exponents for integer time (@ref stimc_wait_time, @ref stimc_time)
 */
enum stimc_time_unit {
    SC_FS = -15, /**< @brief Femto seconds exponent. */
    SC_PS = -12, /**< @brief Pico seconds exponent. */
    SC_NS =  -9, /**< @brief Nano seconds exponent. */
    SC_US =  -6, /**< @brief Micro seconds exponent. */
    SC_MS =  -3, /**< @brief Milli seconds exponent. */
    SC_S  =   0, /**< @brief Seconds exponent. */
};

/**
 * @brief Suspend thread for specified amount of simulation time.
 * @param time Amount of time in unit specified by @c exp.
 * @param exp Time unit (e.g. SC_US).
 *
 * Suspends the current thread (created with @ref stimc_register_startup_thread)
 * for the specified amount of simulation time.
 */
void stimc_wait_time (uint64_t time, enum stimc_time_unit exp);

/**
 * @brief Suspend thread for specified amount of simulation time.
 * @param time Amount of time in seconds.
 *
 * Suspends the current thread (created with @ref stimc_register_startup_thread)
 * for the specified amount of simulation time in seconds.
 */
void stimc_wait_time_seconds (double time);

/**
 * @brief Get the current simulation time in specified unit.
 * @param exp Time unit (e.g. @c SC_US).
 * @return Simulation time.
 */
uint64_t stimc_time (enum stimc_time_unit exp);

/**
 * @brief Get the current simulation time in seconds.
 * @return Simulation time.
 *
 * Time is returned as floating point number and, depending on
 * proceeded time and simulator unit, might be less precise than
 * @ref stimc_time.
 */
double stimc_time_seconds (void);


/******************************************************************************************************/
/* event/wait */
/******************************************************************************************************/

/**
 * @brief stimc event type.
 *
 * Events can be used to synchronize threads.
 * A thread can wait for an event to be triggered.
 * Triggering can occur everywhere (threads and normal methods).
 * An event must be created via @ref stimc_event_create.
 */
typedef struct stimc_event_s *stimc_event;

/**
 * @brief Create a new @ref stimc_event.
 * @return the newly created event.
 */
stimc_event stimc_event_create (void);

/**
 * @brief Free resources of a @ref stimc_event.
 * @param event The event to free.
 */
void stimc_event_free (stimc_event event);

/**
 * @brief stimc event combination type.
 *
 * Multiple @ref stimc_event can be combined into
 * one event combination to wait for all or any of them.
 * The type of combination (all/any) is specified on creation
 * via @ref stimc_event_combination_create.
 */
typedef struct stimc_event_combination_s *stimc_event_combination;

/**
 * @brief Create a new combination of events for waiting.
 *
 * @param any true: resume waiting thread if any of the combined events is triggered,
 *            false: resume waiting thread if all of the combined events are triggered.
 *
 * @return newly created combination.
 */
stimc_event_combination stimc_event_combination_create (bool any);

/**
 * @brief Free event combination.
 *
 * @param combination @ref stimc_event_combination to free.
 */
void stimc_event_combination_free (stimc_event_combination combination);

/**
 * @brief Append event to combination of events.
 *
 * @param combination event combination to append to.
 * @param event event to append.
 */
void stimc_event_combination_append (stimc_event_combination combination, stimc_event event);

/**
 * @brief Make a copy of an event combination into an existing combination.
 *
 * @param dst combination to copy to.
 * @param src combination to copy from.
 *
 * Events in the existing combinations are lost and the type of combination (all/any)
 * will be overwritten.
 */
void stimc_event_combination_copy (stimc_event_combination dst, stimc_event_combination src);

/**
 * @brief Suspend current thread until @c event is triggered.
 * @param event The event to wait on.
 *
 * This function can only be called from within a stimc_thread.
 * The thread will be suspended and resumed on the event being triggered.
 */
void stimc_wait_event (stimc_event event);

/**
 * @brief Suspend current thread until @c event is triggered or until specified timeout.
 * @param event The event to wait on.
 * @param time Amount of time in unit specified by @c exp for timeout.
 * @param exp Time unit (e.g. SC_US).
 *
 * @return true in case of timeout.
 */
bool stimc_wait_event_timeout (stimc_event event, uint64_t time, enum stimc_time_unit exp);

/**
 * @brief Suspend current thread until @c event is triggered or until specified timeout.
 * @param event The event to wait on.
 * @param time Amount of time in seconds for timeout.
 *
 * @return true in case of timeout.
 */
bool stimc_wait_event_timeout_seconds (stimc_event event, double time);

/**
 * @brief Suspend current thread until all/any events in @c combination are triggered.
 *
 * @param combination list of events in @ref stimc_event_combination.
 * @param consume if true, combination will be consumed (and freed) and must not be
 *        used afterwards.
 *
 * @c consume is useful in cases where event-combination should be freed
 * after wait anyway and will leak in case the thread is never resumed before
 * and of simulation.
 */
void stimc_wait_event_combination (stimc_event_combination combination, bool consume);

/**
 * @brief Suspend current thread until all/any events in @c combination are triggered or until specified timeout.
 *
 * @param combination list of events in @ref stimc_event_combination.
 * @param consume if true, combination will be consumed (and freed) and must not be
 *        used afterwards. Explanation in @ref stimc_wait_event_combination.
 * @param time Amount of time in unit specified by @c exp for timeout.
 * @param exp Time unit (e.g. SC_US).
 *
 * @return true in case of timeout.
 */
bool stimc_wait_event_combination_timeout (stimc_event_combination combination, bool consume, uint64_t time, enum stimc_time_unit exp);

/**
 * @brief Suspend current thread until all/any events in @c combination are triggered or until specified timeout.
 *
 * @param combination list of events in @ref stimc_event_combination.
 * @param consume if true, combination will be consumed (and freed) and must not be
 *        used afterwards. Explanation in @ref stimc_wait_event_combination.
 * @param time Amount of time in seconds for timeout.
 *
 * @return true in case of timeout.
 */
bool stimc_wait_event_combination_timeout_seconds (stimc_event_combination combination, bool consume, double time);

/**
 * @brief Trigger a @ref stimc_event.
 * @param event The event to trigger.
 */
void stimc_trigger_event (stimc_event event);

/**
 * @brief Check, whether last wait with timeout returned due to timeout.
 *
 * @return true if last timeout caused resume for current thread.
 *
 * This is similar to checking the return value of the wait functions with timeout.
 * Can only be called from a running thread.
 */
bool stimc_wait_timed_out (void);


/******************************************************************************************************/
/* sim control */
/******************************************************************************************************/

/**
 * @brief Finish simulation.
 *
 * This is equivalent to calling the @c $finish
 * verilog system task.
 */
void stimc_finish (void);


/******************************************************************************************************/
/* port/net/parameter access */
/******************************************************************************************************/

/**
 * @brief Assign port/net to value.
 * @param net The port/net to assign.
 * @param value The value as 32 bit integer.
 * @see @ref stimc_net_set_uint64, @ref stimc_net_set_bits_uint64.
 */
static inline void stimc_net_set_int32 (stimc_net net, int32_t value)
{
    s_vpi_value v;

    v.format        = vpiIntVal;
    v.value.integer = value;
    vpi_put_value (net->net, &v, NULL, vpiNoDelay);
}

/**
 * @brief Assign port/net to value non-blocking.
 * @param net The port/net to assign.
 * @param value The value as 32 bit integer.
 * @see @ref stimc_net_set_uint64_nonblock, @ref stimc_net_set_bits_uint64_nonblock.
 *
 * The assignment will occur similar to verilog non-blocking assignment after current
 * simulator cycle.
 */
void stimc_net_set_int32_nonblock (stimc_net net, int32_t value);

/**
 * @brief Get port/net value.
 * @param net The port/net to assign.
 * @return Value as 32 bit integer.
 * @see @ref stimc_net_get_uint64, @ref stimc_net_get_bits_uint64.
 */
static inline int32_t stimc_net_get_int32 (stimc_net net)
{
    s_vpi_value v;

    v.format = vpiIntVal;
    vpi_get_value (net->net, &v);

    return v.value.integer;
}

/**
 * @brief Get port/net size in bits.
 * @param net The port/net to assign.
 * @return Width of @c net in bits.
 */
static inline unsigned stimc_net_size (stimc_net net)
{
    return vpi_get (vpiSize, net->net);
}

/**
 * @brief Get parameter format (type).
 * @param parameter Parameter to get format of.
 * @return vpi format of specified parameter.
 *
 * Can be used to decide whether to use integer or double format
 * to lookup parameter values.
 */
static inline PLI_INT32 stimc_parameter_get_format (stimc_parameter parameter)
{
    s_vpi_value v;

    v.format = vpiObjTypeVal;
    vpi_get_value (parameter, &v);

    return v.format;
}

/**
 * @brief Get parameter value in 32 bit integer format.
 * @param parameter Parameter to get read.
 * @return Parameter value in 32 bit integer format.
 */
static inline uint32_t stimc_parameter_get_int32 (stimc_parameter parameter)
{
    s_vpi_value v;

    v.format = vpiIntVal;
    vpi_get_value (parameter, &v);

    return v.value.integer;
}

/**
 * @brief Get parameter value in floating point format.
 * @param parameter Parameter to get read.
 * @return Parameter value in floating point format.
 */
static inline double stimc_parameter_get_double (stimc_parameter parameter)
{
    s_vpi_value v;

    v.format = vpiRealVal;
    vpi_get_value (parameter, &v);

    return v.value.real;
}

/**
 * @brief Non-blocking z assignment.
 * @param net Port/net to assign.
 * @see @ref stimc_net_set_z.
 *
 * Sets port/net to high impedance state similar to using
 * a verilog non-blocking assignment.
 */
void stimc_net_set_z_nonblock (stimc_net net);

/**
 * @brief Non-blocking x assignment.
 * @param net Port/net to assign.
 * @see @ref stimc_net_set_x.
 *
 * Sets port/net to verilog x value similar to using
 * a verilog non-blocking assignment.
 */
void stimc_net_set_x_nonblock (stimc_net net);

/**
 * @brief Immediate z assignment.
 * @param net Port/net to assign.
 * @see @ref stimc_net_set_z_nonblock.
 *
 * Sets port/net to high impedance state similar to using
 * a verilog blocking assignment.
 */
void stimc_net_set_z (stimc_net net);

/**
 * @brief Immediate x assignment.
 * @param net Port/net to assign.
 * @see @ref stimc_net_set_x_nonblock.
 *
 * Sets port/net to verilog x value similar to using
 * a verilog blocking assignment.
 */
void stimc_net_set_x (stimc_net net);

/**
 * @brief Check for x/z value.
 * @param net Port/net to check.
 * @return true if port/net contains x or z values, false otherwise.
 */
bool stimc_net_is_xz (stimc_net net);

/**
 * @brief Immediate z assignment to sub bits.
 * @param net Port/net to assign.
 * @param msb MSB of bit range.
 * @param lsb LSB of bit range.
 * @see @ref stimc_net_set_bits_z_nonblock.
 *
 * Sets specified bit range of port/net to high impedance state
 * similar to using a verilog blocking assignment.
 */
void stimc_net_set_bits_z (stimc_net net, unsigned msb, unsigned lsb);

/**
 * @brief Immediate x assignment to sub bits.
 * @param net Port/net to assign.
 * @param msb MSB of bit range.
 * @param lsb LSB of bit range.
 * @see @ref stimc_net_set_bits_x_nonblock.
 *
 * Sets specified bit range of port/net to unknown state
 * similar to using a verilog blocking assignment.
 */
void stimc_net_set_bits_x (stimc_net net, unsigned msb, unsigned lsb);

/**
 * @brief Non-blocking z assignment to sub bits.
 * @param net Port/net to assign.
 * @param msb MSB of bit range.
 * @param lsb LSB of bit range.
 * @see @ref stimc_net_set_bits_z.
 *
 * Sets specified bit range of port/net to high impedance state
 * similar to using a verilog non-blocking assignment.
 */
void stimc_net_set_bits_z_nonblock (stimc_net net, unsigned msb, unsigned lsb);

/**
 * @brief Non-blocking x assignment to sub bits.
 * @param net Port/net to assign.
 * @param msb MSB of bit range.
 * @param lsb LSB of bit range.
 * @see @ref stimc_net_set_bits_x.
 *
 * Sets specified bit range of port/net to unknown state
 * similar to using a verilog non-blocking assignment.
 */
void stimc_net_set_bits_x_nonblock (stimc_net net, unsigned msb, unsigned lsb);

/**
 * @brief Check for x/z value in sub bits.
 * @param net Port/net to check.
 * @param msb MSB of bit range.
 * @param lsb LSB of bit range.
 * @return true if port/net contains x or z values in bit range, false otherwise.
 */
bool stimc_net_bits_are_xz (stimc_net net, unsigned msb, unsigned lsb);

/**
 * @brief Non-blocking sub bits assignment.
 * @param net Port/net to assign.
 * @param msb MSB of bit range.
 * @param lsb LSB of bit range.
 * @param value Value to assign as 64 bit integer.
 * @see @ref stimc_net_set_bits_uint64.
 *
 * Sets sub bit range of port/net to specified
 * value similar to using a verilog non-blocking assignment.
 */
void stimc_net_set_bits_uint64_nonblock (stimc_net net, unsigned msb, unsigned lsb, uint64_t value);

/**
 * @brief Immediate sub bits assignment.
 * @param net Port/net to assign.
 * @param msb MSB of bit range.
 * @param lsb LSB of bit range.
 * @param value Value to assign as 64 bit integer.
 * @see @ref stimc_net_set_bits_uint64_nonblock.
 *
 * Sets sub bit range of port/net to specified
 * value similar to using a verilog blocking assignment.
 */
void stimc_net_set_bits_uint64 (stimc_net net, unsigned msb, unsigned lsb, uint64_t value);

/**
 * @brief Sub bits read.
 * @param net Port/net to read.
 * @param msb MSB of bit range.
 * @param lsb LSB of bit range.
 * @return Value of specified bits as 64 bit integer.
 */
uint64_t stimc_net_get_bits_uint64 (stimc_net net, unsigned msb, unsigned lsb);

/**
 * @brief Non-blocking assignment.
 * @param net Port/net to assign.
 * @param value Value to assign as 64 bit integer.
 * @see @ref stimc_net_set_uint64.
 *
 * Sets port/net to specified value
 * similar to using a verilog non-blocking assignment.
 */
void stimc_net_set_uint64_nonblock (stimc_net net, uint64_t value);

/**
 * @brief Immediate assignment.
 * @param net Port/net to assign.
 * @param value Value to assign as 64 bit integer.
 * @see @ref stimc_net_set_uint64_nonblock.
 *
 * Sets port/net to specified value
 * similar to using a verilog blocking assignment.
 */
void stimc_net_set_uint64 (stimc_net net, uint64_t value);

/**
 * @brief Non-blocking real assignment.
 * @param net Port/net to assign.
 * @param value Value to assign as double.
 * @see @ref stimc_net_set_double.
 *
 * Sets port/net to specified real value
 * similar to using a verilog non-blocking assignment.
 */
void stimc_net_set_double_nonblock (stimc_net net, double value);

/**
 * @brief Immediate real assignment.
 * @param net Port/net to assign.
 * @param value Value to assign as double.
 * @see @ref stimc_net_set_double_nonblock.
 *
 * Sets port/net to specified real value
 * similar to using a verilog blocking assignment.
 */
static inline void stimc_net_set_double (stimc_net net, double value)
{
    s_vpi_value v;

    v.format     = vpiRealVal;
    v.value.real = value;
    vpi_put_value (net->net, &v, NULL, vpiNoDelay);
}

/**
 * @brief Value read.
 * @param net Port/net to read.
 * @return Value as 64 bit integer.
 */
uint64_t stimc_net_get_uint64 (stimc_net net);

/**
 * @brief Real value read.
 * @param net Port/net to read.
 * @return Value as double.
 */
static inline double stimc_net_get_double (stimc_net net)
{
    s_vpi_value v;

    v.format = vpiRealVal;
    vpi_get_value (net->net, &v);

    return v.value.real;
}


/******************************************************************************************************/
/* modules */
/******************************************************************************************************/

/**
 * @brief Module type.
 */
typedef struct stimc_module_s {
    vpiHandle mod; /**< @brief vpi handle for access. */
} stimc_module;

/**
 * @brief Module type initialization.
 * @param m Pointer to module to initialize
 * @param cleanfunc Function to call at end of simulation to clean module data.
 * @param cleandata Data pointer to hand to module cleanup function.
 *
 * Used from within the verilog module initialization
 * system task defined by @ref STIMC_INIT.
 * Do not call from outside as it uses the caller scope
 * to identify the instance.
 */
void stimc_module_init (stimc_module *m, void (*cleanfunc)(void *cleandata), void *cleandata);

/**
 * @brief Module type resource free.
 * @param m Pointer to module to free
 *
 * This only frees the resources of the module,
 * not the actual struct.
 */
void stimc_module_free (stimc_module *m);

/**
 * @brief Register vpi system task for module initialization.
 * @param name Module name
 * @param initfunc function to call on initialization.
 *
 * Will register the verilog system task
 * @c $stimc_name_init for module @c name.
 */
void stimc_module_register (const char *name, void (*initfunc)(void));

/**
 * @brief Port creation function.
 * @param m Pointer to module instance the port belongs to.
 * @param name Port name.
 * @return a newly created port handle for the specified port.
 */
stimc_port stimc_port_init (stimc_module *m, const char *name);

/**
 * @brief Port handle free function.
 * @param p port handle.
 */
void stimc_port_free (stimc_port p);

/**
 * @brief Parameter creation function.
 * @param m Pointer to module instance the parameter belongs to.
 * @param name Parameter name.
 * @return a newly created parameter handle for the specified parameter.
 */
stimc_parameter stimc_parameter_init (stimc_module *m, const char *name);

/**
 * @brief Parameter handle free function.
 * @param p parameter handle.
 */
void stimc_parameter_free (stimc_parameter p);

/**
 * @brief module initialization routine macro.
 *
 * Use to define the initialization code for a stimc module via
 * \code{.cpp}
 * STIMC_INIT (modulename)
 * {
 *     // body creating module resources
 * }
 * \endcode
 * will be indirectly used by @ref STIMC_EXPORT.
 */
#define STIMC_INIT(module) \
    static void _stimc_module_ ## module ## _init (void); \
\
    void _stimc_module_ ## module ## _register (void) \
    { \
        stimc_module_register (#module, _stimc_module_ ## module ## _init); \
    } \
\
    static void _stimc_module_ ## module ## _init (void)

typedef void (*stimc_vpi_init_register_func_t) (void);

struct stimc_vpi_init_register_s {
    stimc_vpi_init_register_func_t    func;
    struct stimc_vpi_init_register_s *next;
};

void stimc_vpi_init_register (struct stimc_vpi_init_register_s *entry);

/**
 * @brief stimc module initialization routine export macro.
 *
 * Used to define the initialization code for a stimc module via
 * @ref STIMC_INIT and register the necessary functions for loading
 * with the vpi library.
 *
 * Use similar to @ref STIMC_INIT :
 * \code{.cpp}
 * STIMC_EXPORT (modulename)
 * {
 *     // body creating module resources
 * }
 * \endcode
 */
#define STIMC_EXPORT(module) \
    void _stimc_module_ ## module ## _register (void); \
\
    static struct stimc_vpi_init_register_s _stimc_module_ ## module ## _vpi_init_s_ = {_stimc_module_ ## module ## _register, NULL}; \
\
    static void _stimc_module_ ## module ## _vpi_init_f_ (void) \
    { \
        stimc_vpi_init_register (&_stimc_module_ ## module ## _vpi_init_s_); \
    } \
    static stimc_vpi_init_register_func_t _stimc_module_ ## module ## _do_export_ __attribute__((__used__, section (".init_array"))) = _stimc_module_ ## module ## _vpi_init_f_; \
\
    STIMC_INIT (module)

#ifdef __cplusplus
/* *auto-indent-off* */
}
/* *auto-indent-on* */
#endif

#endif

