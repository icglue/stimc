#include "apb_stim.h"

#include <stdlib.h>
#include <string.h>


struct apb_stim *apb_stim_create (void)
{
    struct apb_stim *apb_stim = (struct apb_stim *)malloc (sizeof (struct apb_stim));

    stimc_module_init (&(apb_stim->module));

    apb_stim->ID = stimc_parameter_init (&(apb_stim->module), "ID");

    apb_stim->apb_clk_i    = stimc_port_init (&(apb_stim->module), "apb_clk_i");
    apb_stim->apb_resetn_i = stimc_port_init (&(apb_stim->module), "apb_resetn_i");
    apb_stim->apb_clk_en_o = stimc_port_init (&(apb_stim->module), "apb_clk_en_o");

    apb_stim->apb_addr_o   = stimc_port_init (&(apb_stim->module), "apb_addr_o");
    apb_stim->apb_sel_o    = stimc_port_init (&(apb_stim->module), "apb_sel_o");
    apb_stim->apb_enable_o = stimc_port_init (&(apb_stim->module), "apb_enable_o");
    apb_stim->apb_write_o  = stimc_port_init (&(apb_stim->module), "apb_write_o");
    apb_stim->apb_strb_o   = stimc_port_init (&(apb_stim->module), "apb_strb_o");
    apb_stim->apb_prot_o   = stimc_port_init (&(apb_stim->module), "apb_prot_o");
    apb_stim->apb_wdata_o  = stimc_port_init (&(apb_stim->module), "apb_wdata_o");

    apb_stim->apb_ready_i  = stimc_port_init (&(apb_stim->module), "apb_ready_i");
    apb_stim->apb_rdata_i  = stimc_port_init (&(apb_stim->module), "apb_rdata_i");
    apb_stim->apb_slverr_i = stimc_port_init (&(apb_stim->module), "apb_slverr_i");

    apb_stim->clk_event           = stimc_event_create ();
    apb_stim->reset_release_event = stimc_event_create ();

    /* init... */
    stimc_net_set_int32_nonblock (apb_stim->apb_clk_en_o, 0);
    stimc_net_set_int32_nonblock (apb_stim->apb_sel_o,    0);
    stimc_net_set_int32_nonblock (apb_stim->apb_enable_o, 0);
    stimc_net_set_int32_nonblock (apb_stim->apb_write_o,  0);
    stimc_net_set_int32_nonblock (apb_stim->apb_prot_o,   0);
    stimc_net_set_int32_nonblock (apb_stim->apb_addr_o,   0);
    stimc_net_set_int32_nonblock (apb_stim->apb_wdata_o,  0);
    stimc_net_set_int32_nonblock (apb_stim->apb_strb_o,   0);

    return apb_stim;
}

bool apb_stim_write (struct apb_stim *emulator, uint32_t addr, uint8_t strb, uint32_t wdata)
{
    stimc_net_set_int32_nonblock (emulator->apb_clk_en_o, 1);
    stimc_net_set_int32_nonblock (emulator->apb_sel_o,    1);
    stimc_net_set_int32_nonblock (emulator->apb_write_o,  1);
    stimc_net_set_int32_nonblock (emulator->apb_addr_o,   addr);
    stimc_net_set_int32_nonblock (emulator->apb_wdata_o,  wdata);
    stimc_net_set_int32_nonblock (emulator->apb_strb_o,   strb);
    stimc_wait_event (emulator->clk_event);
    stimc_net_set_int32_nonblock (emulator->apb_enable_o, 1);

    bool result = true;
    while (true) {
        stimc_wait_event (emulator->clk_event);
        if (stimc_net_get_int32 (emulator->apb_ready_i) == 1) {
            if (stimc_net_get_int32 (emulator->apb_slverr_i) == 1) {
                result = false;
            }
            break;
        }
    }

    stimc_net_set_int32_nonblock (emulator->apb_clk_en_o, 0);
    stimc_net_set_int32_nonblock (emulator->apb_sel_o,    0);
    stimc_net_set_int32_nonblock (emulator->apb_enable_o, 0);
    stimc_net_set_int32_nonblock (emulator->apb_write_o,  0);
    stimc_net_set_int32_nonblock (emulator->apb_addr_o,   0);
    stimc_net_set_int32_nonblock (emulator->apb_wdata_o,  0);
    stimc_net_set_int32_nonblock (emulator->apb_strb_o,   0);

    return result;
}

bool apb_stim_read (struct apb_stim *emulator, uint32_t addr, uint32_t *rdata)
{
    stimc_net_set_int32_nonblock (emulator->apb_clk_en_o, 1);
    stimc_net_set_int32_nonblock (emulator->apb_sel_o,    1);
    stimc_net_set_int32_nonblock (emulator->apb_write_o,  0);
    stimc_net_set_int32_nonblock (emulator->apb_addr_o,   addr);
    stimc_net_set_int32_nonblock (emulator->apb_wdata_o,  0);
    stimc_net_set_int32_nonblock (emulator->apb_strb_o,   0);
    stimc_wait_event (emulator->clk_event);
    stimc_net_set_int32_nonblock (emulator->apb_enable_o, 1);

    bool result = true;
    while (true) {
        stimc_wait_event (emulator->clk_event);
        if (stimc_net_get_int32 (emulator->apb_ready_i) == 1) {
            if (stimc_net_get_int32 (emulator->apb_slverr_i) == 1) {
                result = false;
            }
            break;
        }
    }

    stimc_net_set_int32_nonblock (emulator->apb_clk_en_o, 0);
    stimc_net_set_int32_nonblock (emulator->apb_sel_o,    0);
    stimc_net_set_int32_nonblock (emulator->apb_enable_o, 0);
    stimc_net_set_int32_nonblock (emulator->apb_write_o,  0);
    stimc_net_set_int32_nonblock (emulator->apb_addr_o,   0);
    stimc_net_set_int32_nonblock (emulator->apb_strb_o,   0);

    *rdata = stimc_net_get_int32 (emulator->apb_rdata_i);

    return result;
}

void apb_stim_testcontrol (void *userdata)
{
    struct apb_stim *apb_stim = (struct apb_stim *)userdata;

    /* reset... */
    stimc_wait_event (apb_stim->reset_release_event);
    printf ("reset released\n");
    stimc_wait_event (apb_stim->clk_event);
    stimc_wait_event (apb_stim->clk_event);

    apb_stim_write (apb_stim, 0x1234, 0xf, 0xa5a5a5a5);
    stimc_wait_event (apb_stim->clk_event);
    uint32_t rdata;
    apb_stim_read  (apb_stim, 0x5678, &rdata);
    stimc_wait_event (apb_stim->clk_event);
    stimc_finish ();
}

void apb_stim_clock (void *userdata)
{
    struct apb_stim *apb_stim = (struct apb_stim *)userdata;

    stimc_trigger_event (apb_stim->clk_event);
}

void apb_stim_reset_release (void *userdata)
{
    struct apb_stim *apb_stim = (struct apb_stim *)userdata;

    stimc_trigger_event (apb_stim->reset_release_event);
}

/* init */
STIMC_INIT (apb_stim)
{
    struct apb_stim *apb_stim = apb_stim_create ();

    stimc_register_startup_thread (apb_stim_testcontrol, apb_stim);
    stimc_register_posedge_method (apb_stim_clock, apb_stim, apb_stim->apb_clk_i);
    stimc_register_posedge_method (apb_stim_reset_release, apb_stim, apb_stim->apb_resetn_i);
}

