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
 * @brief stimc core.
 */

#ifndef __STIMC_H__
#define __STIMC_H__

#include <vpi_user.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
#include <atomic>
/**
 * @brief stimc thread fence (not sure if necessary, but used between suspend/resume of stimc threads)
 */
#define stimc_thread_fence(...) std::atomic_thread_fence (std::memory_order_acq_rel)
#else
#include <stdatomic.h>
/**
 * @brief stimc thread fence (not sure if necessary, but used between suspend/resume of stimc threads)
 */
#define stimc_thread_fence(...) __atomic_thread_fence (__ATOMIC_ACQ_REL)
#endif

#ifdef __cplusplus
extern "C" {
#endif

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

    struct stimc_nba_data *nba; /**< @brief Data for scheduled non-blocking assignments */
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
 */
void stimc_register_startup_thread (void (*threadfunc)(void *userdata), void *userdata, size_t stacksize);

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
 * @brief Suspend current thread until @c event is triggered.
 * @param event The event to wait on.
 *
 * This function can only be called from within a stimc_thread.
 * The thread will be suspended and resumed on the event being triggered.
 */
void stimc_wait_event (stimc_event event);

/**
 * @brief Trigger a @ref stimc_event.
 * @param event The event to trigger.
 */
void stimc_trigger_event (stimc_event event);


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
    char *id; /**< @brief Hierarchical identifier. */
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
 *     // body
 * }
 * \endcode
 * will be indirectly used by @ref STIMC_EXPORT.
 */
#define STIMC_INIT(module) \
    static void _stimc_module_ ## module ## _init (void); \
\
    static int _stimc_module_ ## module ## _init_cptf (PLI_BYTE8 * user_data __attribute__((unused))) \
    { \
        return 0; \
    } \
\
    static int _stimc_module_ ## module ## _init_cltf (PLI_BYTE8 * user_data __attribute__((unused))) \
    { \
        _stimc_module_ ## module ## _init (); \
\
        return 0; \
    } \
\
    void _stimc_module_ ## module ## _register (void) \
    { \
        s_vpi_systf_data tf_data; \
\
        tf_data.type      = vpiSysTask; \
        tf_data.tfname    = "$stimc_" #module "_init"; \
        tf_data.calltf    = _stimc_module_ ## module ## _init_cltf; \
        tf_data.compiletf = _stimc_module_ ## module ## _init_cptf; \
        tf_data.sizetf    = 0; \
        tf_data.user_data = NULL; \
\
        vpi_register_systf (&tf_data); \
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
 */
#define STIMC_EXPORT(module) \
    STIMC_INIT (module) \
    \
    static stimc_vpi_init_register_s _stimc_module_ ## module ## _vpi_init_s_ = {_stimc_module_ ## module ## _register, NULL}; \
    \
    static void _stimc_module_ ## module ## _vpi_init_f_ (void) \
    { \
        stimc_vpi_init_register (&_stimc_module_ ## module ## _vpi_init_s_); \
    } \
    static stimc_vpi_init_register_func_t _stimc_module_ ## module ## _do_export_ __attribute__((__used__, section (".init_array"))) = _stimc_module_ ## module ## _vpi_init_f_;

#ifdef __cplusplus
}
#endif

#endif

