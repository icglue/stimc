#ifndef __APB_EMULATOR_H__
#define __APB_EMULATOR_H__

#include <stimc.h>


struct apb_stim {
    stimc_module module;

    /* parameter */
    stimc_parameter ID;

    /* ports */
    stimc_port  apb_clk_i;
    stimc_port  apb_resetn_i;
    stimc_port  apb_clk_en_o;

    stimc_port  apb_addr_o;
    stimc_port  apb_sel_o;
    stimc_port  apb_enable_o;
    stimc_port  apb_write_o;
    stimc_port  apb_strb_o;
    stimc_port  apb_prot_o;
    stimc_port  apb_wdata_o;

    stimc_port  apb_ready_i;
    stimc_port  apb_rdata_i;
    stimc_port  apb_slverr_i;

    /* events */
    stimc_event clk_event;
    stimc_event reset_release_event;
};

struct apb_stim* apb_stim_create (void);
bool apb_stim_write (struct apb_stim *emulator, uint32_t addr, uint8_t strb, uint32_t wdata);
bool apb_stim_read (struct apb_stim *emulator, uint32_t addr, uint32_t *rdata);
void apb_stim_testcontrol (void *userdata);
void apb_stim_clock (void *userdata);
void apb_stim_reset_release (void *userdata);

#endif
