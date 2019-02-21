module ff_sc (
    clk_i,
    reset_n_i,
    data_i,
    data_o
);

    parameter RESET_VAL = 1'b0;

    input  clk_i;
    input  reset_n_i;
    input  data_i;
    output data_o;

    initial begin
        $stimc_ff_sc_init();
    end
endmodule
