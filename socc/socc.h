#ifndef __SOCC_H__
#define __SOCC_H__

#include <vpi_user.h>

struct socc_module {
    char *scope;
};

void socc_module_init (struct socc_module *m);
vpiHandle socc_pin_init (struct socc_module *m, const char *name);

void socc_register_startup_task (void (*task) (void *userdata), void *userdata);

#endif
