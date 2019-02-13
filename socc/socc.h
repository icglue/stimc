#ifndef __SOCC_H__
#define __SOCC_H__

#include <vpi_user.h>

struct socc_module {
    char *scope;
};


const char *socc_get_caller_scope (void);

void socc_module_init (struct socc_module *m);
vpiHandle socc_pin_init (struct socc_module *m, const char *name);

#endif
