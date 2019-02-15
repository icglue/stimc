#ifndef __APB_EMULATOR_H__
#define __APB_EMULATOR_H__

#include <stimc.h>

struct apb_emulator {
    stimc_module module;
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

    stimc_port  emulator_id_i;
    /* events */
    stimc_event clk_event;
    stimc_event reset_release_event;
};

struct apb_emulator* apb_emulator_create (void);
bool apb_emulator_write (struct apb_emulator *emulator, uint32_t addr, uint8_t strb, uint32_t wdata);
bool apb_emulator_read (struct apb_emulator *emulator, uint32_t addr, uint32_t *rdata);
void apb_emulator_testcontrol (void *userdata);
void apb_emulator_clock (void *userdata);
void apb_emulator_reset_release (void *userdata);

#endif
