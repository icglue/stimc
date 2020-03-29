#include "dummy.hpp"

using namespace stimcxx;

dummy::dummy () :
    STIMCXX_PARAMETER (DATA_W),
    STIMCXX_PORT (clk_i),
    STIMCXX_PORT (reset_n_i),
    STIMCXX_PORT (data_in_i),
    STIMCXX_PORT (data_out_o)
{
    STIMCXX_REGISTER_STARTUP_THREAD (testcontrol);
    STIMCXX_REGISTER_METHOD (posedge, clk_i, clock);
    STIMCXX_REGISTER_METHOD (change, data_in_i, dinchange);

    fprintf (stderr, "DEBUG: dummy module \"%s\" has DATA_W %d\n", module_id (), DATA_W.value ());
}

dummy::~dummy ()
{}

void dummy::clock ()
{
    //fprintf (stderr, "DEBUG: clkedge in %s at time %ldns\n", module_id (), time (SC_NS));
    clk_event.trigger ();
}

void dummy::dinchange ()
{
    if (data_in_i == X) {
        fprintf (stderr, "DEBUG: data_in changed at time %ldns to <undefined>\n", time (SC_NS));
    } else {
        fprintf (stderr, "DEBUG: data_in changed at time %luns to 0x%016lx\n", time (SC_NS), (uint64_t)data_in_i);
    }
    din_event.trigger ();
}

void __attribute__((weak)) dummy::testcontrol ()
{}

STIMCXX_INIT (dummy)

