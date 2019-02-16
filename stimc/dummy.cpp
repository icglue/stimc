#include "dummy.hpp"

dummy::dummy ():
    DATA_W     (*this, "DATA_W"),
    clk_i      (*this, "clk_i"),
    reset_n_i  (*this, "reset_n_i"),
    data_in_i  (*this, "data_in_i"),
    data_out_o (*this, "data_out_o")
{
    STIMCXX_REGISTER_STARTUP_THREAD (testcontrol);
    STIMCXX_REGISTER_METHOD (posedge, clk_i, clock);
    STIMCXX_REGISTER_METHOD (change, data_in_i, dinchange);

    fprintf (stderr, "DEBUG: dummy module \"%s\" has DATA_W %d\n", module_id(), DATA_W.value());
}

dummy::~dummy ()
{
}

void dummy::clock ()
{
    fprintf (stderr, "DEBUG: clkedge in %s at time %ldns\n", module_id(), time (SC_NS));
    clk_event.trigger ();
}

void dummy::dinchange ()
{
    if (data_in_i.is_xz()) {
        fprintf (stderr, "DEBUG: data_in changed at time %ldns to <undefined>\n", time (SC_NS));
    } else {
        fprintf (stderr, "DEBUG: data_in changed at time %luns to 0x%08lx\n", time (SC_NS), (uint64_t) data_in_i);
    }
}

void dummy::testcontrol ()
{
    fprintf (stderr, "DEBUG: testcontrol...\n");
    data_out_o = 0x0123456789abcdef;
    wait (1e-9);
    data_out_o = 0x89abcdef01234567;

    for (int i = 0; i < 100; i++) {
        wait (clk_event);
        if (i % 2) {
            data_out_o.bits(i,i) = 1;
        } else {
            if (i % 4) {
                data_out_o.set_z();
            } else {
                data_out_o.set_x();
            }
        }
    }
}

STIMCXX_INIT (dummy)

STIMC_EXPORT_START
STIMCXX_EXPORT (dummy)
STIMC_EXPORT_END
