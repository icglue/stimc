#ifndef __STIMC_H__
#define __STIMC_H__

#include <vpi_user.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* methods/threads */
void stimc_register_posedge_method (void (*methodfunc) (void *userdata), void *userdata, vpiHandle net);
void stimc_register_negedge_method (void (*methodfunc) (void *userdata), void *userdata, vpiHandle net);
void stimc_register_change_method  (void (*methodfunc) (void *userdata), void *userdata, vpiHandle net);

void stimc_register_startup_thread (void (*threadfunc) (void *userdata), void *userdata);

/* time/wait */
#define SC_FS -15
#define SC_PS -12
#define SC_NS -9
#define SC_US -6
#define SC_MS -3
#define SC_S  0
void stimc_wait_time (uint64_t time, int exp);
void stimc_wait_time_seconds (double time);

uint64_t stimc_time (int exp);
double   stimc_time_seconds (void);

/* event/wait */
typedef struct stimc_event_s* stimc_event;

stimc_event stimc_event_create (void);
void stimc_wait_event (stimc_event event);
void stimc_trigger_event (stimc_event event);

/* sim control */
void stimc_finish (void);

/* ports/parameters */
typedef vpiHandle stimc_port;
typedef vpiHandle stimc_parameter;

static inline void stimc_net_set_int32 (vpiHandle net, int32_t value)
{
    s_vpi_value v;
    v.format        = vpiIntVal;
    v.value.integer = value;
    vpi_put_value (net, &v, NULL, vpiNoDelay);
}

static inline int32_t stimc_net_get_int32 (vpiHandle net)
{
    s_vpi_value v;
    v.format = vpiIntVal;
    vpi_get_value (net, &v);

    return v.value.integer;
}

static inline unsigned stimc_net_size (vpiHandle net)
{
    return vpi_get (vpiSize, net);
}

static inline uint32_t stimc_parameter_get_int32 (stimc_parameter parameter)
{
    s_vpi_value v;
    v.format = vpiIntVal;
    vpi_get_value (parameter, &v);

    return v.value.integer;
}

void stimc_net_set_z (vpiHandle net);
void stimc_net_set_x (vpiHandle net);
bool stimc_net_is_xz (vpiHandle net);

void stimc_net_set_bits_uint64 (vpiHandle net, unsigned lsb, unsigned msb, uint64_t value);
uint64_t stimc_net_get_bits_uint64 (vpiHandle net, unsigned lsb, unsigned msb);
void stimc_net_set_uint64 (vpiHandle net, uint64_t value);
uint64_t stimc_net_get_uint64 (vpiHandle net);

/* modules */
typedef struct stimc_module_s {
    char *id;
} stimc_module;

void            stimc_module_init    (stimc_module *m);
stimc_port      stimc_port_init      (stimc_module *m, const char *name);
stimc_parameter stimc_parameter_init (stimc_module *m, const char *name);

/* module initialization routine macro
 *
 * calling STIMC_INIT (modulename)
 * {
 *   // body
 * }
 *
 * runs function body when called via $stimc_modulename_init in verilog shell.
 *
 * STIMC_EXPORT_START
 * STIMC_EXPORT (module1)
 * STIMC_EXPORT (module2)
 * STIMC_EXPORT_END
 *
 * makes module1 and module2 available (only once per vpi interface lib)
 */

#define STIMC_INIT(module) \
static void _stimc_module_ ## module ## _init (void);\
\
static int _stimc_module_ ## module ## _init_cptf (PLI_BYTE8* user_data __attribute__((unused)))\
{\
    return 0;\
}\
\
static int _stimc_module_ ## module ## _init_cltf (PLI_BYTE8* user_data __attribute__((unused)))\
{\
    _stimc_module_ ## module ## _init ();\
\
    return 0;\
}\
\
static void _stimc_module_ ## module ## _register (void)\
{\
    s_vpi_systf_data tf_data;\
\
    tf_data.type      = vpiSysTask;\
    tf_data.tfname    = "$stimc_" #module "_init";\
    tf_data.calltf    = _stimc_module_ ## module ## _init_cltf;\
    tf_data.compiletf = _stimc_module_ ## module ## _init_cptf;\
    tf_data.sizetf    = 0;\
    tf_data.user_data = NULL;\
\
    vpi_register_systf(&tf_data);\
}\
\
static void _stimc_module_ ## module ## _init (void)

#define STIMC_EXPORT(module)\
    _stimc_module_ ## module ## _register,

#define STIMC_EXPORT_START void (*vlog_startup_routines[])(void) = {
#define STIMC_EXPORT_END   0};

#ifdef __cplusplus
}
#endif

#endif
