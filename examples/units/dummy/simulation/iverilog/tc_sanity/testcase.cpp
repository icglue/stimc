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
    data_out_o(23,16)   <<= 0xff;
    data_out_o(7,0)     <<= 0xff;
    data_out_o(120) <<= 1;
    data_out_o(121)   = Z;
    data_out_o(122)   = X;
    data_out_o(123) <<= Z;
    data_out_o(124) <<= X;
    wait (10, SC_NS);
    data_out_o(100,2) <<= X;

    if (data_in_i(10,0)   == X) {fprintf (stderr, "pass: data-in[10:0] contains x\n");} else {fprintf (stderr, "fail: data-in[10:0] contains x\n");}
    if (data_in_i(118,19) != X) {fprintf (stderr, "pass: data-in[118:19] contains no x\n");} else {fprintf (stderr, "fail: data-in[118:19] contains no x\n");}
    if (data_in_i(120,0)   == X) {fprintf (stderr, "pass: data-in[120:0] contains x\n");} else {fprintf (stderr, "fail: data-in[120:0] contains x\n");}

}
