#include "dummy.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct dummy* dummy_create (void)
{
    struct dummy *dummy = (struct dummy*) malloc (sizeof (struct dummy));

    stimc_module_init (&(dummy->module));

    dummy->DATA_W     = stimc_parameter_init (&(dummy->module), "DATA_W");

    dummy->clk_i      = stimc_port_init      (&(dummy->module), "clk_i");
    dummy->reset_n_i  = stimc_port_init      (&(dummy->module), "reset_n_i");
    dummy->data_in_i  = stimc_port_init      (&(dummy->module), "data_in_i");
    dummy->data_out_o = stimc_port_init      (&(dummy->module), "data_out_o");

    dummy->clk_event  = stimc_event_create ();

    return dummy;
}

void dummy_testcontrol (void *userdata)
{
    struct dummy *dummy = (struct dummy *) userdata;

    fprintf (stderr, "DEBUG: testcontrol...\n");

    /* write to output */
    stimc_net_set_uint64 (dummy->data_out_o, 0x0123456789abcdef);

    stimc_wait_time_seconds (1e-9);

    stimc_net_set_uint64 (dummy->data_out_o, 0x89abcdef01234567);

    for (int i = 0; i < 100; i++) {
        stimc_wait_event (dummy->clk_event);
        if (i % 2) {
            stimc_net_set_bits_uint64 (dummy->data_out_o, i, i, 1);
        } else {
            if (i % 4) {
                stimc_net_set_z (dummy->data_out_o);
            } else {
                stimc_net_set_x (dummy->data_out_o);
            }
        }
    }
}

void dummy_dinchange (void *userdata)
{
    struct dummy *dummy = (struct dummy *) userdata;
    if (stimc_net_is_xz (dummy->data_in_i)) {
        fprintf (stderr, "DEBUG: data_in changed at time %ldns to <undefined>\n", stimc_time (SC_NS));
    } else {
        fprintf (stderr, "DEBUG: data_in changed at time %luns to 0x%08lx\n", stimc_time (SC_NS), stimc_net_get_uint64 (dummy->data_in_i));
    }
}

void dummy_clock (void *userdata)
{
    struct dummy *dummy = (struct dummy *) userdata;
    fprintf (stderr, "DEBUG: clkedge in %s at time %ldns\n", dummy->module.id, stimc_time (SC_NS));

    stimc_trigger_event (dummy->clk_event);
}

/* init */
STIMC_INIT (dummy)
{
    struct dummy *dummy = dummy_create ();

    stimc_register_startup_thread (dummy_testcontrol, dummy);
    stimc_register_posedge_method (dummy_clock, dummy, dummy->clk_i);
    stimc_register_change_method  (dummy_dinchange, dummy, dummy->data_in_i);

    fprintf (stderr, "DEBUG: dummy module \"%s\" has DATA_W %d\n", dummy->module.id, stimc_parameter_get_int32 (dummy->DATA_W));
}

