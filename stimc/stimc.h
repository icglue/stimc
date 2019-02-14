#ifndef __STIMC_H__
#define __STIMC_H__

#include <vpi_user.h>

struct stimc_module {
    char *scope;
};

void stimc_module_init (struct stimc_module *m);
vpiHandle stimc_pin_init (struct stimc_module *m, const char *name);

void stimc_register_startup_task (void (*task) (void *userdata), void *userdata);

void stimc_wait_time (double time);

#endif
