#ifndef __DUMMY_H__
#define __DUMMY_H__

#include <stimc.h>

struct dummy {
    stimc_module module;
    /* parameters */
    stimc_parameter DATA_W;
    /* ports */
    stimc_port      clk_i;
    stimc_port      reset_n_i;
    stimc_port      data_in_i;
    stimc_port      data_out_o;
    /* events */
    stimc_event clk_event;
};

struct dummy* dummy_create (void);
void dummy_testcontrol (void *userdata);
void dummy_dinchange (void *userdata);
void dummy_clock (void *userdata);

#endif
