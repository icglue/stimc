module dummy (
    clk_i,
    reset_n_i,
    data_in_i,
    data_out_o
);

    input         clk_i;
    input         reset_n_i;
    input  [31:0] data_in_i;
    output [31:0] data_out_o;

endmodule
