#include "dummy_c.h"
#include <logging.h>
#include <stdlib.h>
#include <assert.h>

struct dummy_c* dummy_c_create (void)
{
    struct dummy_c *dummy_c = (struct dummy_c*) malloc (sizeof (struct dummy_c));
    assert (dummy_c);

    stimc_module_init (&(dummy_c->module), dummy_c_free, dummy_c);

    dummy_c->DATA_W     = stimc_parameter_init (&(dummy_c->module), "DATA_W");

    dummy_c->clk_i      = stimc_port_init (&(dummy_c->module), "clk_i");
    dummy_c->reset_n_i  = stimc_port_init (&(dummy_c->module), "reset_n_i");
    dummy_c->data_in_i  = stimc_port_init (&(dummy_c->module), "data_in_i");
    dummy_c->data_out_o = stimc_port_init (&(dummy_c->module), "data_out_o");

    dummy_c->clk_event  = stimc_event_create ();
    dummy_c->din_event  = stimc_event_create ();

    stimc_register_startup_thread (dummy_c_testcontrol, dummy_c, 0);
    stimc_register_startup_thread (dummy_c_testcontrol2, dummy_c, 0);
    stimc_register_posedge_method (dummy_c_clock, dummy_c, dummy_c->clk_i);
    stimc_register_change_method  (dummy_c_dinchange, dummy_c, dummy_c->data_in_i);

    log_debug ("dummy_c module \"%s\" has DATA_W %d", dummy_c->module.id, stimc_parameter_get_int32 (dummy_c->DATA_W));

    return dummy_c;
}

void dummy_c_free (void* userdata)
{
    struct dummy_c *dummy_c = (struct dummy_c *)userdata;

    stimc_event_free (dummy_c->din_event);
    stimc_event_free (dummy_c->clk_event);

    stimc_port_free (dummy_c->data_out_o);
    stimc_port_free (dummy_c->data_in_i);
    stimc_port_free (dummy_c->reset_n_i);
    stimc_port_free (dummy_c->clk_i);

    stimc_parameter_free (dummy_c->DATA_W);

    stimc_module_free (&(dummy_c->module));

    free (dummy_c);
}

void dummy_c_clock (void* userdata)
{
    struct dummy_c *dummy_c = (struct dummy_c *)userdata;

    stimc_trigger_event (dummy_c->clk_event);
}

void dummy_c_dinchange (void* userdata)
{
    struct dummy_c *dummy_c = (struct dummy_c *)userdata;

    if (stimc_net_is_xz (dummy_c->data_in_i)) {
        log_debug ("data_in changed at time %ldns to <undefined>", stimc_time (SC_NS));
    } else {
        log_debug ("data_in changed at time %luns to 0x%016lx", stimc_time (SC_NS), stimc_net_get_uint64 (dummy_c->data_in_i));
    }
    stimc_trigger_event (dummy_c->din_event);
}

void __attribute__((weak)) dummy_c_testcontrol (void *userdata __attribute__((unused)))
{}

void __attribute__((weak)) dummy_c_testcontrol2 (void *userdata __attribute__((unused)))
{}

#ifndef NO_STIMC_EXPORT
STIMC_EXPORT (dummy_c)
#else
STIMC_INIT (dummy_c)
#endif
{
    dummy_c_create ();
}

