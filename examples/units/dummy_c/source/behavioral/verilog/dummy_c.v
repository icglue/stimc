module dummy_c (
    clk_i,
    reset_n_i,
    data_in_i,
    data_out_o
);

    parameter DATA_W = 32;

    input               clk_i;
    input               reset_n_i;
    input  [DATA_W-1:0] data_in_i;
    output [DATA_W-1:0] data_out_o;

    initial begin
        $stimc_dummy_c_init();
    end

endmodule
