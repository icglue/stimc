`timescale 1ns/1ps

module tb_dummy_c ();
    `include "tb_selfcheck.vh"

    parameter DATA_W = 128;

    wire              clk_s;
    wire              reset_n_s;
    wire [DATA_W-1:0] data_in_s;
    wire [DATA_W-1:0] data_out_s;

    dummy_c #(
        .DATA_W (DATA_W)
    ) i_dummy_c (
        .clk_i      (clk_s),
        .reset_n_i  (reset_n_s),
        .data_in_i  (data_in_s),
        .data_out_o (data_out_s)
    );


    localparam CLKPERIOD = 2.0;
    reg              clk;
    reg              reset_n;
    reg [DATA_W-1:0] data_in;

    assign clk_s     = clk;
    assign reset_n_s = reset_n;
    assign data_in_s = data_in;

    initial begin
        clk     = 1'b0;
        reset_n = 1'b0;
        #(3.3*CLKPERIOD);
        reset_n = 1'b1;
    end

    always #(CLKPERIOD / 2.0) begin
        clk = ~clk;
    end

    `include "testcase.vh"

endmodule
