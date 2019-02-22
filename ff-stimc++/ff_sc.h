#ifndef __FF_SC_H__
#define __FF_SC_H__

#include "stimc++.h"

class ff_sc : public stimcxx_module {
    private:
        parameter RESET_VAL;

        port clk_i;
        port reset_n_i;
        port data_i;

        port data_o;

    public:
        ff_sc ();
        virtual ~ff_sc ();

        void clock ();
        void reset_release ();
};

#endif

