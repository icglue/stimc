#include <vpi_user.h>

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "stimc.h"

struct apb_emulator {
    struct stimc_module module;
    /* ports */
    vpiHandle apb_clk_i;
    vpiHandle apb_resetn_i;
    vpiHandle apb_clk_en_o;

    vpiHandle apb_addr_o;
    vpiHandle apb_sel_o;
    vpiHandle apb_enable_o;
    vpiHandle apb_write_o;
    vpiHandle apb_strb_o;
    vpiHandle apb_prot_o;
    vpiHandle apb_wdata_o;

    vpiHandle apb_ready_i;
    vpiHandle apb_rdata_i;
    vpiHandle apb_slverr_i;

    vpiHandle emulator_id_i;
    /* events */
    stimc_event clk_event;
    stimc_event reset_release_event;
};

struct apb_emulator* apb_emulator_create (void) {
    struct apb_emulator *apb_emulator = (struct apb_emulator*) malloc (sizeof (struct apb_emulator));

    stimc_module_init (&(apb_emulator->module));


    apb_emulator->apb_clk_i     = stimc_pin_init (&(apb_emulator->module), "apb_clk_i");
    apb_emulator->apb_resetn_i  = stimc_pin_init (&(apb_emulator->module), "apb_resetn_i");
    apb_emulator->apb_clk_en_o  = stimc_pin_init (&(apb_emulator->module), "apb_clk_en_o");

    apb_emulator->apb_addr_o    = stimc_pin_init (&(apb_emulator->module), "apb_addr_o");
    apb_emulator->apb_sel_o     = stimc_pin_init (&(apb_emulator->module), "apb_sel_o");
    apb_emulator->apb_enable_o  = stimc_pin_init (&(apb_emulator->module), "apb_enable_o");
    apb_emulator->apb_write_o   = stimc_pin_init (&(apb_emulator->module), "apb_write_o");
    apb_emulator->apb_strb_o    = stimc_pin_init (&(apb_emulator->module), "apb_strb_o");
    apb_emulator->apb_prot_o    = stimc_pin_init (&(apb_emulator->module), "apb_prot_o");
    apb_emulator->apb_wdata_o   = stimc_pin_init (&(apb_emulator->module), "apb_wdata_o");

    apb_emulator->apb_ready_i   = stimc_pin_init (&(apb_emulator->module), "apb_ready_i");
    apb_emulator->apb_rdata_i   = stimc_pin_init (&(apb_emulator->module), "apb_rdata_i");
    apb_emulator->apb_slverr_i  = stimc_pin_init (&(apb_emulator->module), "apb_slverr_i");

    apb_emulator->emulator_id_i = stimc_pin_init (&(apb_emulator->module), "emulator_id_i");

    apb_emulator->clk_event           = stimc_event_create ();
    apb_emulator->reset_release_event = stimc_event_create ();

    // init...
    stimc_net_set_uint32 (apb_emulator->apb_clk_en_o, 0);
    stimc_net_set_uint32 (apb_emulator->apb_sel_o,    0);
    stimc_net_set_uint32 (apb_emulator->apb_enable_o, 0);
    stimc_net_set_uint32 (apb_emulator->apb_write_o,  0);
    stimc_net_set_uint32 (apb_emulator->apb_prot_o,   0);
    stimc_net_set_uint32 (apb_emulator->apb_addr_o,   0);
    stimc_net_set_uint32 (apb_emulator->apb_wdata_o,  0);
    stimc_net_set_uint32 (apb_emulator->apb_strb_o,   0);

    return apb_emulator;
}

bool apb_emulator_write (struct apb_emulator *emulator, uint32_t addr, uint8_t strb, uint32_t wdata)
{
    stimc_net_set_uint32 (emulator->apb_clk_en_o, 1);
    stimc_net_set_uint32 (emulator->apb_sel_o,    1);
    stimc_net_set_uint32 (emulator->apb_write_o,  1);
    stimc_net_set_uint32 (emulator->apb_addr_o,   addr);
    stimc_net_set_uint32 (emulator->apb_wdata_o,  wdata);
    stimc_net_set_uint32 (emulator->apb_strb_o,   strb);
    stimc_wait_event (emulator->clk_event);
    stimc_net_set_uint32 (emulator->apb_enable_o, 1);

    bool result = true;
    while (true) {
        stimc_wait_event (emulator->clk_event);
        if (stimc_net_get_uint32 (emulator->apb_ready_i) == 1) {
            if (stimc_net_get_uint32 (emulator->apb_slverr_i) == 1) {
                result = false;
            }
            break;
        }
    }

    stimc_net_set_uint32 (emulator->apb_clk_en_o, 0);
    stimc_net_set_uint32 (emulator->apb_sel_o,    0);
    stimc_net_set_uint32 (emulator->apb_enable_o, 0);
    stimc_net_set_uint32 (emulator->apb_write_o,  0);
    stimc_net_set_uint32 (emulator->apb_addr_o,   0);
    stimc_net_set_uint32 (emulator->apb_wdata_o,  0);
    stimc_net_set_uint32 (emulator->apb_strb_o,   0);

    return result;
}

bool apb_emulator_read (struct apb_emulator *emulator, uint32_t addr, uint32_t *rdata)
{
    stimc_net_set_uint32 (emulator->apb_clk_en_o, 1);
    stimc_net_set_uint32 (emulator->apb_sel_o,    1);
    stimc_net_set_uint32 (emulator->apb_write_o,  0);
    stimc_net_set_uint32 (emulator->apb_addr_o,   addr);
    stimc_net_set_uint32 (emulator->apb_wdata_o,  0);
    stimc_net_set_uint32 (emulator->apb_strb_o,   0);
    stimc_wait_event (emulator->clk_event);
    stimc_net_set_uint32 (emulator->apb_enable_o, 1);

    bool result = true;
    while (true) {
        stimc_wait_event (emulator->clk_event);
        if (stimc_net_get_uint32 (emulator->apb_ready_i) == 1) {
            if (stimc_net_get_uint32 (emulator->apb_slverr_i) == 1) {
                result = false;
            }
            break;
        }
    }

    stimc_net_set_uint32 (emulator->apb_clk_en_o, 0);
    stimc_net_set_uint32 (emulator->apb_sel_o,    0);
    stimc_net_set_uint32 (emulator->apb_enable_o, 0);
    stimc_net_set_uint32 (emulator->apb_write_o,  0);
    stimc_net_set_uint32 (emulator->apb_addr_o,   0);
    stimc_net_set_uint32 (emulator->apb_strb_o,   0);

    *rdata = stimc_net_get_uint32 (emulator->apb_rdata_i);

    return result;
}

void apb_emulator_testcontrol (void *userdata) {
    struct apb_emulator *apb_emulator = (struct apb_emulator *) userdata;

    // reset...
    stimc_wait_event (apb_emulator->reset_release_event);
    printf ("reset released\n");
    stimc_wait_event (apb_emulator->clk_event);
    stimc_wait_event (apb_emulator->clk_event);

    apb_emulator_write (apb_emulator, 0x1234, 0xf, 0xa5a5a5a5);
    stimc_wait_event (apb_emulator->clk_event);
    uint32_t rdata;
    apb_emulator_read  (apb_emulator, 0x5678, &rdata);
    stimc_wait_event (apb_emulator->clk_event);
}

void apb_emulator_clock (void *userdata) {
    struct apb_emulator *apb_emulator = (struct apb_emulator *) userdata;
    stimc_trigger_event (apb_emulator->clk_event);
}

void apb_emulator_reset_release (void *userdata) {
    struct apb_emulator *apb_emulator = (struct apb_emulator *) userdata;
    stimc_trigger_event (apb_emulator->reset_release_event);
}

/* init */
STIMC_INIT (apb_emulator)
{
    struct apb_emulator *apb_emulator = apb_emulator_create ();

    stimc_register_startup_thread (apb_emulator_testcontrol, apb_emulator);
    stimc_register_posedge_method (apb_emulator_clock, apb_emulator, apb_emulator->apb_clk_i);
    stimc_register_posedge_method (apb_emulator_reset_release, apb_emulator, apb_emulator->apb_resetn_i);
}

