`timescale 1ns/1ps

module tb_ff ();

    reg reset_n;
    reg clk;

    localparam SR_W = 8;

    wire [SR_W-1:0] shiftreg;

    genvar i;
    generate
        for (i = 0; i < SR_W / 2; i = i+1) begin: GEN_FF_VLOG
            ff #(
                .RESET_VAL (i % 2)
            ) i_ff (
                .clk_i     (clk),
                .reset_n_i (reset_n),
                .data_i    (shiftreg[(SR_W+i-1)%SR_W]),
                .data_o    (shiftreg[i])
            );
            initial $display ("GEN: %d", i);

        end
        for (i = SR_W/2; i < SR_W; i = i+1) begin: GEN_FF_STIMC
            ff_sc #(
                .RESET_VAL (i % 2)
            ) i_ff (
                .clk_i     (clk),
                .reset_n_i (reset_n),
                .data_i    (shiftreg[(SR_W+i-1)%SR_W]),
                .data_o    (shiftreg[i])
            );
            initial $display ("GEN: %d", i);
        end
    endgenerate

    localparam CLKPERIOD = 2;
    initial begin
        clk     = 1'b0;
        reset_n = 1'b0;
        #(3.3*CLKPERIOD);
        reset_n = 1'b1;
        #(20*CLKPERIOD);
        $finish();
    end

    always #(CLKPERIOD/2.0) begin
        clk = ~clk;
    end

    always @(posedge clk) begin
        $display ("shiftreg: %08b", shiftreg);
    end

endmodule
