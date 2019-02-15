#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <stimc.h>

struct dummy {
    stimc_module module;
    /* ports */
    stimc_port  clk_i;
    stimc_port  reset_n_i;
    stimc_port  data_in_i;
    stimc_port  data_out_o;
    /* events */
    stimc_event clk_event;
};

struct dummy* dummy_create (void) {
    struct dummy *dummy = (struct dummy*) malloc (sizeof (struct dummy));

    stimc_module_init (&(dummy->module));

    dummy->clk_i      = stimc_pin_init (&(dummy->module), "clk_i");
    dummy->reset_n_i  = stimc_pin_init (&(dummy->module), "reset_n_i");
    dummy->data_in_i  = stimc_pin_init (&(dummy->module), "data_in_i");
    dummy->data_out_o = stimc_pin_init (&(dummy->module), "data_out_o");

    dummy->clk_event  = stimc_event_create ();

    return dummy;
}

void dummy_testcontrol (void *userdata) {
    struct dummy *dummy = (struct dummy *) userdata;

    fprintf (stderr, "DEBUG: testcontrol...\n");

    /* write to output */
    stimc_net_set_uint32 (dummy->data_out_o, 0x12345678);

    stimc_wait_time_seconds (1e-9);

    stimc_net_set_uint32 (dummy->data_out_o, 0x9abcdef0);

    for (int i = 0; i < 100; i++) {
        stimc_wait_event (dummy->clk_event);
        stimc_net_set_uint32 (dummy->data_out_o, i);
    }
}

void dummy_clock (void *userdata) {
    struct dummy *dummy = (struct dummy *) userdata;
    fprintf (stderr, "DEBUG: clkedge in %s at time %e\n", dummy->module.id, stimc_time ());

    stimc_trigger_event (dummy->clk_event);
}

/* init */
STIMC_INIT (dummy)
{
    struct dummy *dummy = dummy_create ();

    stimc_register_startup_thread (dummy_testcontrol, dummy);
    stimc_register_posedge_method (dummy_clock, dummy, dummy->clk_i);
}

/* export */
STIMC_EXPORT_START
    STIMC_EXPORT (dummy)
STIMC_EXPORT_END
