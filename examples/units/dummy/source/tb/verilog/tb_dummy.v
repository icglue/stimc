`timescale 1ns/1ps

module tb_dummy ();

    localparam DATA_W = 128;

    wire              clk_s;
    wire              reset_n_s;
    wire [DATA_W-1:0] data_in_s;
    wire [DATA_W-1:0] data_out_s;

    dummy #(
        .DATA_W (DATA_W)
    ) i_dummy (
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

    initial begin
        data_in = {DATA_W {1'b0}};
        #(2.5*CLKPERIOD);
        data_in = {DATA_W {1'bx}};
        #(CLKPERIOD);
        data_in = 'h12345678;
        #(CLKPERIOD);
        data_in = {DATA_W {1'bz}};
        #(10*CLKPERIOD);
        $finish ();
    end

endmodule
