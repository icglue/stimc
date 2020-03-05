#ifndef __DUMMY_HPP__
#define __DUMMY_HPP__

#include "stimc++.h"

class dummy : public stimcxx_module {
    private:
        parameter DATA_W;

        port clk_i;
        port reset_n_i;
        port data_in_i;
        port data_out_o;

        stimcxx_event clk_event;

    public:
        dummy ();
        virtual ~dummy ();

        void testcontrol ();
        void clock ();
        void dinchange ();
};

#endif

