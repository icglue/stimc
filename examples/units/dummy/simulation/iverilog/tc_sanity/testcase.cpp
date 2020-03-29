#include "dummy.hpp"
#include "tb_selfcheck.h"

using namespace stimcxx;

void dummy::testcontrol ()
{
    unsigned dw = DATA_W;
    unsigned errors = 0;

    fprintf (stderr, "INFO: stimc start: full assignments stimc -> verilog\n");
    data_out_o = 0x0123456789abcdef;
    wait (clk_event);
    data_out_o <<= 0x89abcdef01234567;
    wait (1e-9);
    data_out_o = Z;
    wait (1, SC_NS);
    data_out_o = X;
    wait (0.1e-9);

    fprintf (stderr, "INFO: stimc start: single-bit assignment stimc -> verilog\n");
    for (unsigned i = 0; i < dw; i++) {
        wait (clk_event);

        data_out_o((dw+i-2)%dw) <<= 0;
        data_out_o(i)           <<= 1;
        data_out_o((i+2)%dw)    <<= Z;
        data_out_o((i+4)%dw)    <<= X;
    }

    fprintf (stderr, "INFO: stimc start: bit-range assignment stimc -> verilog\n");
    wait (clk_event);
    data_out_o <<= 0;
    for (unsigned i = 0; i < dw-64; i++) {
        wait (clk_event);

        if (i % 2 == 0) {
            data_out_o(i+63,i) <<= 0xffffffff0000ffff;
        } else {
            data_out_o(i+63,i) <<= 0xffffffffffff0000;
        }
    }

    fprintf (stderr, "INFO: stimc start: verilog -> stimc\n");
    wait (din_event);
    wait (din_event);
    if (data_in_i != X) {
        fprintf (stderr, "fail: expected data_in_i == X\n");
        errors++;
    }
    wait (din_event);
    if (data_in_i != X) {
        fprintf (stderr, "fail: expected data_in_i == X\n");
        errors++;
    }

    wait (din_event);
    if (data_in_i(10,0) != X) {
        fprintf (stderr, "fail: data-in[10:0] contains x\n");
        errors++;
    }
    if (data_in_i(118,19) == X) {
        fprintf (stderr, "fail: data-in[118:19] contains no x\n");
        errors++;
    }
    if (data_in_i(120,0) != X) {
        fprintf (stderr, "fail: data-in[120:0] contains x\n");
        errors++;
    }
    if (data_in_i(127) != X) {
        fprintf (stderr, "fail: data-in[127] contains x\n");
        errors++;
    }

    wait (din_event);
    if (data_in_i(127) != 1) {
        fprintf (stderr, "fail: data-in[127] should be 1\n");
        errors++;
    }
    if (data_in_i(126,63) != 0xfdb97530eca86420) {
        fprintf (stderr, "fail: data-in[126:63] should be 0xfdb97530eca86420 but is 0x%016lx\n", (uint64_t) data_in_i(126,63));
        errors++;
    }
    if (data_in_i(34,30) != 0x1e) {
        fprintf (stderr, "fail: data-in[34:40] should be 0x1e but is 0x%02lx\n", (uint64_t) data_in_i(34,30));
        errors++;
    }

    wait (200, SC_NS);
    tb_final_check (1, errors, false);
}
