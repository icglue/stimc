#include <vpi_user.h>

#include <stdlib.h>
#include <string.h>

#include "stimc.h"

struct dummy {
    struct stimc_module module;
    /* ports */
    vpiHandle clk_i;
    vpiHandle reset_n_i;
    vpiHandle data_in_i;
    vpiHandle data_out_o;
};

struct dummy* dummy_create (void) {
    struct dummy *dummy = (struct dummy*) malloc (sizeof (struct dummy));

    stimc_module_init (&(dummy->module));

    dummy->clk_i      = stimc_pin_init (&(dummy->module), "clk_i");
    dummy->reset_n_i  = stimc_pin_init (&(dummy->module), "reset_n_i");
    dummy->data_in_i  = stimc_pin_init (&(dummy->module), "data_in_i");
    dummy->data_out_o = stimc_pin_init (&(dummy->module), "data_out_o");

    return dummy;
}

void dummy_testcontrol (void *userdata) {
    struct dummy *dummy = (struct dummy *) userdata;

    fprintf (stderr, "DEBUG: testcontrol...\n");

    /* write to output */
    s_vpi_value val;
    val.format = vpiIntVal;
    val.value.integer = 0x12345678;
    vpi_put_value (dummy->data_out_o, &val, NULL, vpiNoDelay);

    stimc_wait_time (1e-9);

    val.value.integer = 0x9abcdef0;
    vpi_put_value (dummy->data_out_o, &val, NULL, vpiNoDelay);
}

void dummy_clock (void *userdata) {
    struct dummy *dummy = (struct dummy *) userdata;
    fprintf (stderr, "DEBUG: clkedge in %s at time %e\n", dummy->module.id, stimc_time ());
}

/* init */
STIMC_INIT (dummy)
{
    struct dummy *dummy = dummy_create ();

    stimc_register_startup_thread (dummy_testcontrol, dummy);
    stimc_register_posedge_method (dummy_clock, dummy, dummy->clk_i);
}

