

always @(posedge clk) begin
    $display ("shiftreg: %08b", shiftreg);
end

reg [SR_W-1:0] shiftreg_last;
integer j;

initial begin
    @(posedge reset_n);

    shiftreg_last = shiftreg;

    for (j = 0; j < 20; j = j+1) begin
        shiftreg_last = {shiftreg_last[SR_W-2:0], shiftreg_last[SR_W-1]};
        #(CLKPERIOD);
        if (shiftreg == shiftreg_last) begin
            $display ("check passed: shiftreg is %b as expected", shiftreg);
            tb_check_passed;
        end else begin
            $display ("check failed: shiftreg is %b but expected %b", shiftreg, shiftreg_last);
            tb_check_failed;
        end
    end

    tb_final_check;
end
