#ifndef __STIMC_H__
#define __STIMC_H__

#include <vpi_user.h>

void stimc_register_posedge_method (void (*methodfunc) (void *userdata), void *userdata, vpiHandle net);
void stimc_register_negedge_method (void (*methodfunc) (void *userdata), void *userdata, vpiHandle net);

void stimc_register_startup_thread (void (*threadfunc) (void *userdata), void *userdata);

void stimc_wait_time (double time);
double stimc_time (void);


typedef struct stimc_event_s* stimc_event;

stimc_event stimc_event_create (void);
void stimc_wait_event (stimc_event event);
void stimc_trigger_event (stimc_event event);


struct stimc_module {
    char *id;
};

void stimc_module_init (struct stimc_module *m);
vpiHandle stimc_pin_init (struct stimc_module *m, const char *name);

/* module initialization routine macro
 *
 * calling STIMC_INIT (modulename)
 * { // body }
 *
 * runs function body when called via $stimc_modulename_init in verilog shell.
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
void (*vlog_startup_routines[])(void) = {\
    _stimc_module_ ## module ## _register,\
    0\
};\
\
static void _stimc_module_ ## module ## _init (void)


#endif
