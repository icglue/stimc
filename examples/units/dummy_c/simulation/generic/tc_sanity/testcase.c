#include <dummy_c.h>
#include <logging.h>
#include <tb_selfcheck.h>

void dummy_c_testcontrol (void *userdata)
{
    struct dummy_c *dummy_c = (struct dummy_c*)userdata;

    unsigned dw = stimc_parameter_get_int32 (dummy_c->DATA_W);
    unsigned errors = 0;

    log_info ("stimc start: full assignments stimc -> verilog");
    stimc_net_set_uint64          (dummy_c->data_out_o, 0x0123456789abcdef);
    stimc_wait_event              (dummy_c->clk_event);
    stimc_net_set_uint64_nonblock (dummy_c->data_out_o, 0x89abcdef01234567);
    stimc_wait_time_seconds       (1e-9);
    stimc_net_set_z               (dummy_c->data_out_o);
    stimc_wait_time               (1, SC_NS);
    stimc_net_set_x               (dummy_c->data_out_o);
    stimc_wait_time_seconds       (0.1e-9);

    log_info ("stimc start: single-bit assignment stimc -> verilog");
    for (unsigned i = 0; i < dw; i++) {
        stimc_wait_event (dummy_c->clk_event);

        stimc_net_set_bits_uint64_nonblock (dummy_c->data_out_o, (dw+i-2)%dw, (dw+i-2)%dw, 0);
        stimc_net_set_bits_uint64_nonblock (dummy_c->data_out_o, i,           i,           1);
        stimc_net_set_bits_z_nonblock      (dummy_c->data_out_o, (i+2)%dw,    (i+2)%dw);
        stimc_net_set_bits_x_nonblock      (dummy_c->data_out_o, (i+4)%dw,    (i+4)%dw);
    }

    log_info ("stimc start: bit-range assignment stimc -> verilog");
    stimc_wait_event (dummy_c->clk_event);
    stimc_net_set_int32_nonblock (dummy_c->data_out_o, 0);
    for (unsigned i = 0; i < dw-64; i++) {
        stimc_wait_event (dummy_c->clk_event);

        if (i % 2 == 0) {
            stimc_net_set_bits_uint64_nonblock (dummy_c->data_out_o, i+63, i, 0xffffffff0000ffff);
        } else {
            stimc_net_set_bits_uint64_nonblock (dummy_c->data_out_o, i+63, i, 0xffffffffffff0000);
        }
    }

    log_info ("stimc start: verilog -> stimc");
    stimc_wait_event (dummy_c->din_event);
    stimc_wait_event (dummy_c->din_event);
    if (!stimc_net_is_xz (dummy_c->data_in_i)) {
        log_warn ("expected data_in_i == X");
        errors++;
    }
    stimc_wait_event (dummy_c->din_event);
    if (!stimc_net_is_xz (dummy_c->data_in_i)) {
        log_warn ("expected data_in_i == X");
        errors++;
    }

    stimc_wait_event (dummy_c->din_event);
    if (!stimc_net_bits_are_xz (dummy_c->data_in_i, 10, 0)) {
        log_warn ("data-in[10:0] contains x");
        errors++;
    }
    if (stimc_net_bits_are_xz (dummy_c->data_in_i, 118, 19)) {
        log_warn ("data-in[118:19] contains no x");
        errors++;
    }
    if (!stimc_net_bits_are_xz (dummy_c->data_in_i, 120, 0)) {
        log_warn ("data-in[120:0] contains x");
        errors++;
    }
    if (!stimc_net_bits_are_xz (dummy_c->data_in_i, 127, 127)) {
        log_warn ("data-in[127] contains x");
        errors++;
    }

    stimc_wait_event (dummy_c->din_event);
    if (stimc_net_get_bits_uint64 (dummy_c->data_in_i, 127, 127) != 1) {
        log_warn ("data-in[127] should be 1");
        errors++;
    }
    if (stimc_net_get_bits_uint64 (dummy_c->data_in_i, 126, 63) != 0xfdb97530eca86420) {
        log_warn ("data-in[126:63] should be 0xfdb97530eca86420 but is 0x%016lx", stimc_net_get_bits_uint64 (dummy_c->data_in_i, 126, 63));
        errors++;
    }
    if (stimc_net_get_bits_uint64 (dummy_c->data_in_i, 34, 30) != 0x1e) {
        log_warn ("data-in[34:40] should be 0x1e but is 0x%02lx", stimc_net_get_bits_uint64 (dummy_c->data_in_i, 34, 30));
        errors++;
    }

    stimc_wait_time (200, SC_NS);
    tb_final_check (1, errors, false);
}
