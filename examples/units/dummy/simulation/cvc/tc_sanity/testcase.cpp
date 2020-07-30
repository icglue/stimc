#include <dummy.h>
#include <logging.h>
#include <tb_selfcheck.h>

using namespace stimcxx;

void dummy::testcontrol ()
{
    log_set_level (LOG_LEVEL_DEBUG);

    unsigned dw = DATA_W;
    unsigned errors = 0;

    log_info ("stimc start: full assignments stimc -> verilog");
    data_out_o = 0x0123456789abcdef;
    wait (clk_event);
    data_out_o <<= 0x89abcdef01234567;
    wait (1e-9);
    data_out_o = Z;
    wait (1, SC_NS);
    data_out_o = X;
    wait (0.1e-9);

    log_info ("stimc start: single-bit assignment stimc -> verilog");
    for (unsigned i = 0; i < dw; i++) {
        wait (clk_event);

        data_out_o((dw+i-2)%dw) <<= 0;
        data_out_o(i)           <<= 1;
        data_out_o((i+2)%dw)    <<= Z;
        data_out_o((i+4)%dw)    <<= X;
    }

    log_info ("stimc start: bit-range assignment stimc -> verilog");
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

    log_info ("stimc start: verilog -> stimc");
    wait (din_event);
    wait (din_event);
    if (data_in_i != X) {
        log_warn ("expected data_in_i == X");
        errors++;
    }
    wait (din_event);
    if (data_in_i != X) {
        log_warn ("expected data_in_i == X");
        errors++;
    }

    wait (din_event);
    if (data_in_i(10,0) != X) {
        log_warn ("data-in[10:0] contains x");
        errors++;
    }
    if (data_in_i(118,19) == X) {
        log_warn ("data-in[118:19] contains no x");
        errors++;
    }
    if (data_in_i(120,0) != X) {
        log_warn ("data-in[120:0] contains x");
        errors++;
    }
    if (data_in_i(127) != X) {
        log_warn ("data-in[127] contains x");
        errors++;
    }

    wait (din_event);
    if (data_in_i(127) != 1) {
        log_warn ("data-in[127] should be 1");
        errors++;
    }
    if (data_in_i(126,63) != 0xfdb97530eca86420) {
        log_warn ("data-in[126:63] should be 0xfdb97530eca86420 but is 0x%016lx", (uint64_t) data_in_i(126,63));
        errors++;
    }
    if (data_in_i(34,30) != 0x1e) {
        log_warn ("data-in[34:40] should be 0x1e but is 0x%02lx", (uint64_t) data_in_i(34,30));
        errors++;
    }

    wait (200, SC_NS);
    tb_final_check (1, errors, false);
}
