#include <vpi_user.h>

#include <stdlib.h>
#include <string.h>

#include "socc.h"

struct dummy {
    struct socc_module module;
    /* ports */
    vpiHandle clk_i;
    vpiHandle reset_n_i;
    vpiHandle data_in_i;
    vpiHandle data_out_o;
};

struct dummy* dummy_create (void) {
    struct dummy *dummy = (struct dummy*) malloc (sizeof (struct dummy));

    socc_module_init (&(dummy->module));

    dummy->clk_i      = socc_pin_init (&(dummy->module), "clk_i");
    dummy->reset_n_i  = socc_pin_init (&(dummy->module), "reset_n_i");
    dummy->data_in_i  = socc_pin_init (&(dummy->module), "data_in_i");
    dummy->data_out_o = socc_pin_init (&(dummy->module), "data_out_o");

    return dummy;
}

void dummy_testcontrol (void *userdata) {
    struct dummy *dummy = (struct dummy *) userdata;

    /* TODO */
}




/* init */
static int socc_dummy_init_cptf (PLI_BYTE8* user_data)
{
    return 0;
}

static int socc_dummy_init_cltf (PLI_BYTE8* user_data)
{
    struct dummy *dummy = dummy_create ();

    return 0;
}

static void socc_dummy_register (void)
{
    s_vpi_systf_data tf_data;

    tf_data.type      = vpiSysTask;
    tf_data.tfname    = "$socc_dummy_init";
    tf_data.calltf    = socc_dummy_init_cltf;
    tf_data.compiletf = socc_dummy_init_cptf;
    tf_data.sizetf    = 0;
    tf_data.user_data = NULL;

    vpi_register_systf(&tf_data);
}

void (*vlog_startup_routines[])(void) = {
    socc_dummy_register,
    0
};
