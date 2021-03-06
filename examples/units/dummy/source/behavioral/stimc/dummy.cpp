#include "dummy.h"
#include "logging.h"

using namespace stimcxx;

dummy::dummy () :
    STIMCXX_PARAMETER (DATA_W),
    STIMCXX_PORT (clk_i),
    STIMCXX_PORT (reset_n_i),
    STIMCXX_PORT (data_in_i),
    STIMCXX_PORT (data_out_o),

    clk_event (),
    din_event ()
{
    STIMCXX_REGISTER_STARTUP_THREAD (testcontrol);
    STIMCXX_REGISTER_STARTUP_THREAD (testcontrol2);
    STIMCXX_REGISTER_METHOD (posedge, clk_i, clock);
    STIMCXX_REGISTER_METHOD (change, data_in_i, dinchange);

    log_debug ("dummy module \"%s\" has DATA_W %d", module_id (), DATA_W.value ());
}

dummy::~dummy ()
{}

void dummy::clock ()
{
    //log_debug ("clkedge in %s at time %ldns", module_id (), time (SC_NS));
    clk_event.trigger ();
}

void dummy::dinchange ()
{
    if (data_in_i == X) {
        log_debug ("data_in changed at time %ldns to <undefined>", time (SC_NS));
    } else {
        log_debug ("data_in changed at time %luns to 0x%016lx", time (SC_NS), (uint64_t)data_in_i);
    }
    din_event.trigger ();
}

void __attribute__((weak)) dummy::testcontrol ()
{}

void __attribute__((weak)) dummy::testcontrol2 ()
{}

#ifndef NO_STIMCXX_EXPORT
STIMCXX_EXPORT (dummy)
#else
STIMCXX_INIT (dummy)
#endif

