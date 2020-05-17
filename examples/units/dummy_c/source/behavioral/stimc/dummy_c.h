#ifndef __DUMMY_C_H__
#define __DUMMY_C_H__

#include "stimc.h"

struct dummy_c {
    stimc_module module;

    stimc_parameter DATA_W;

    stimc_port clk_i;
    stimc_port reset_n_i;
    stimc_port data_in_i;
    stimc_port data_out_o;

    stimc_event clk_event;
    stimc_event din_event;
};

struct dummy_c* dummy_c_create (void);
void dummy_c_free (void* userdata);

void dummy_c_testcontrol (void* userdata);
void dummy_c_testcontrol2 (void* userdata);
void dummy_c_clock (void* userdata);
void dummy_c_dinchange (void* userdata);

#endif

