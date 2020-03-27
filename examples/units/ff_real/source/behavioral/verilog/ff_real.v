module ff_real (
    clk_i,
    reset_n_i,
    data_i,
    data_o
);

    parameter RESET_VAL = 0.0;

    input       clk_i;
    input       reset_n_i;
    input  real data_i;
    output real data_o;

    initial begin
        $stimc_ff_real_init();
    end
endmodule
