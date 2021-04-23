`timescale 1ns/1ps

module tb_iotest ();
    `include "tb_selfcheck.vh"

    parameter DATA_W = 8;

    wire [DATA_W-1:0] gpio_s;

    iotest #(
        .ID     (0),
        .WIDTH  (DATA_W)
    ) i_iotest_0 (
        .gpio_b (gpio_s)
    );

    iotest #(
        .ID     (1),
        .WIDTH  (DATA_W)
    ) i_iotest_1 (
        .gpio_b (gpio_s)
    );

    `include "testcase.vh"

endmodule
