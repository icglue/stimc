`timescale 1ns/1ps

module tb_ff_real ();

    reg reset_n;
    reg clk;

    localparam SR_W = 8;

    wire real shiftreg [SR_W-1:0];

    genvar i;
    generate
        for (i = 0; i < SR_W / 2; i = i+1) begin: GEN_FF_VLOG
            real ff;
            always @(posedge clk or negedge reset_n) begin
                if (reset_n == 1'b0) begin
                    ff <= i * 1.3;
                end else begin
                    ff <= shiftreg[(SR_W+i-1)%SR_W];
                end
            end
            assign shiftreg[i] = ff;
        end
        for (i = SR_W/2; i < SR_W; i = i+1) begin: GEN_FF_STIMC
            ff_real #(
                .RESET_VAL (i * 1.3)
            ) i_ff (
                .clk_i     (clk),
                .reset_n_i (reset_n),
                .data_i    (shiftreg[(SR_W+i-1)%SR_W]),
                .data_o    (shiftreg[i])
            );
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
        $display ("shiftreg[0]: %3.3f", shiftreg[0]);
    end

endmodule
