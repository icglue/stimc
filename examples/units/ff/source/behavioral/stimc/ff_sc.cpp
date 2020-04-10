#include "ff_sc.h"

ff_sc::ff_sc () :
    STIMCXX_PARAMETER (RESET_VAL),

    STIMCXX_PORT (clk_i),
    STIMCXX_PORT (reset_n_i),
    STIMCXX_PORT (data_i),

    STIMCXX_PORT (data_o)
{
    /* init... */
    uint64_t rv = RESET_VAL;

    data_o <<= rv;

    STIMCXX_REGISTER_METHOD (posedge, clk_i, clock);
    STIMCXX_REGISTER_METHOD (posedge, reset_n_i, reset_release);
}

ff_sc::~ff_sc ()
{}

void ff_sc::clock ()
{
    if (reset_n_i != 0) {
        unsigned v = data_i;
        data_o <<= v;
    } else {
        uint64_t rv = RESET_VAL;
        data_o <<= rv;
    }
}

void ff_sc::reset_release ()
{}

#if !defined NO_STIMCXX_EXPORT
    STIMCXX_EXPORT (ff_sc)
#else
    STIMCXX_INIT (ff_sc)
#endif

