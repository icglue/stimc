#include "dummy.hpp"

dummy::dummy ():
    DATA_W     (*this, "DATA_W"),
    clk_i      (*this, "clk_i"),
    reset_n_i  (*this, "reset_n_i"),
    data_in_i  (*this, "data_in_i"),
    data_out_o (*this, "data_out_o")
{
}

dummy::~dummy ()
{
}

STIMCXX_INIT (dummy)

STIMC_EXPORT_START
STIMCXX_EXPORT (dummy)
STIMC_EXPORT_END
