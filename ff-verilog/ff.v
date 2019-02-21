module ff (
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

    reg data;

    always @(posedge clk_i or negedge reset_n_i) begin
        if (reset_n_i == 1'b0) begin
            data <= RESET_VAL;
        end else begin
            data <= data_i;
        end
    end

    assign data_o = data;

endmodule
