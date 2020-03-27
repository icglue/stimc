#ifndef __FF_REAL_H__
#define __FF_REAL_H__

#include "stimc++.h"

class ff_real : public stimcxx::module {
    private:
        parameter RESET_VAL;

        port      clk_i;
        port      reset_n_i;
        port_real data_i;

        port_real data_o;

    public:
        ff_real ();
        ~ff_real ();

        void clock ();
        void reset_release ();
};

#endif

