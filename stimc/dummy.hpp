#ifndef __DUMMYXX_H__
#define __DUMMYXX_H__

#include "stimc++.h"

class dummy : public stimcxx_module {
    private:
        parameter DATA_W;

        port      clk_i;
        port      reset_n_i;
        port      data_in_i;
        port      data_out_o;

        stimcxx_event clk_event;

    public:
        dummy ();
        virtual ~dummy ();

};

/*
void dummy_testcontrol (void *userdata);
void dummy_dinchange (void *userdata);
void dummy_clock (void *userdata);
*/

#endif
