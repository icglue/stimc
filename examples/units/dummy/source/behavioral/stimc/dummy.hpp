#ifndef __DUMMY_HPP__
#define __DUMMY_HPP__

#include "stimc++.h"

class dummy : public stimcxx::module {
    private:
        parameter DATA_W;

        port clk_i;
        port reset_n_i;
        port data_in_i;
        port data_out_o;

        stimcxx::event clk_event;
        stimcxx::event din_event;

    public:
        dummy ();
        virtual ~dummy ();

        void testcontrol ();
        void clock ();
        void dinchange ();
};

#endif

