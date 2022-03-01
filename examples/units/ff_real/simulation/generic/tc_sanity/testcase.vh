
always @(posedge clk) begin
    $display ("shiftreg[0]: %5.3f", shiftreg[0]);
end

real shiftreg_last [SR_W-1:0];
integer j;
integer k;

initial begin
    @(posedge reset_n);

    for (k = 0; k < SR_W; k = k+1) begin
        shiftreg_last[k] = shiftreg[k];
    end

    for (j = 0; j < 20; j = j+1) begin
        for (k = 0; k < SR_W; k = k+1) begin
            shiftreg_last[k] <= shiftreg_last[(SR_W+k-1)%SR_W];
        end
        #(CLKPERIOD);
        for (k = 0; k < SR_W; k = k+1) begin
            if (shiftreg[k] == shiftreg_last[k]) begin
                $display ("check passed: shiftreg[%d] is %5.3f as expected", k, shiftreg[k]);
                tb_check_passed;
            end else begin
                $display ("check failed: shiftreg[%d] is %5.3f but expected %5.3f", k, shiftreg[k], shiftreg_last[k]);
                tb_check_failed;
            end
        end
    end

    tb_final_check;
end
