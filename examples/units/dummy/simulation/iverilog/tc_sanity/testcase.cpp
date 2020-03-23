#include "dummy.hpp"

using namespace stimcxx;

void dummy::testcontrol ()
{
    fprintf (stderr, "DEBUG: testcontrol...\n");
    data_out_o = 0x0123456789abcdef;
    wait (1e-9);
    data_out_o = 0x89abcdef01234567;

    for (int i = 0; i < 100; i++) {
        wait (clk_event);
        if (i % 2) {
            data_out_o(i, i) = 1;
        } else {
            if (i % 4) {
                data_out_o = Z;
            } else {
                data_out_o = X;
            }
        }
    }

    wait (10, SC_NS);
    data_out_o(31,24)   = 0;
    data_out_o(15,8)    = 0;
    wait (10, SC_NS);
    data_out_o(23,16) <<= 0xff;
    data_out_o(7,0)   <<= 0xff;
    wait (10, SC_NS);
}
