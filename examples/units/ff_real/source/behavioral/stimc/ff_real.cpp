#include "ff_real.h"

ff_real::ff_real () :
    STIMCXX_PARAMETER (RESET_VAL),

    STIMCXX_PORT (clk_i),
    STIMCXX_PORT (reset_n_i),
    STIMCXX_PORT (data_i),

    STIMCXX_PORT (data_o)
{
    /* init... */
    double rv = RESET_VAL;

    data_o <<= rv;

    STIMCXX_REGISTER_METHOD (posedge, clk_i, clock);
    STIMCXX_REGISTER_METHOD (posedge, reset_n_i, reset_release);
}

ff_real::~ff_real ()
{}

void ff_real::clock ()
{
    if (reset_n_i != 0) {
        double v = data_i;
        data_o <<= v;
    } else {
        double rv = RESET_VAL;
        data_o = rv;
    }
}

void ff_real::reset_release ()
{}

#ifndef NO_STIMCXX_EXPORT
STIMCXX_EXPORT (ff_real)
#else
STIMCXX_INIT (ff_real)
#endif

