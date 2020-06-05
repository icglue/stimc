integer i;
reg [DATA_W-1:0] cmp_val;

initial begin
    $display ("INFO:     vlog start:  full assignments stimc -> verilog");
    @(posedge clk);
    if (data_out_s !== 64'h0123456789abcdef) begin
        tb_check_failed;
        tb_final_check;
    end
    #0.5;
    if (data_out_s !== 64'h89abcdef01234567) begin
        tb_check_failed;
        tb_final_check;
    end
    #1;
    if (data_out_s !== {DATA_W {1'bz}}) begin
        tb_check_failed;
        tb_final_check;
    end
    #1;
    @(posedge clk);
    if (data_out_s !== {DATA_W {1'bx}}) begin
        tb_check_failed;
        tb_final_check;
    end

    $display ("INFO:     vlog start:  single-bit assignments stimc -> verilog");
    cmp_val = {DATA_W {1'bx}};
    for (i = 0; i < DATA_W; i = i+1) begin
        cmp_val[(DATA_W+i-2)%DATA_W] = 1'b0;
        cmp_val[i]                   = 1'b1;
        cmp_val[(i+2)%DATA_W]        = 1'bz;
        cmp_val[(i+4)%DATA_W]        = 1'bx;

        @(posedge clk);
        if (data_out_s !== cmp_val) begin
            $display ("ERROR:    comparison failed at iteration %d: is: %b - expected: %b", i, data_out_s, cmp_val);
            tb_check_failed;
            tb_final_check;
        end
    end

    $display ("INFO:     vlog start:  bit-range assignments stimc -> verilog");
    @(posedge clk);
    if (data_out_s !== 0) begin
        tb_check_failed;
        tb_final_check;
    end
    cmp_val = {DATA_W {1'b0}};
    for (i = 0; i < DATA_W-64; i = i+1) begin
        if (i % 2 == 0) begin
            cmp_val[i+:64] = 64'hffff_ffff_0000_ffff;
        end else begin
            cmp_val[i+:64] = 64'hffff_ffff_ffff_0000;
        end

        @(posedge clk);
        if (data_out_s !== cmp_val) begin
            $display ("ERROR:    comparison failed at iteration %d: is: %b - expected: %b", i, data_out_s, cmp_val);
            tb_check_failed;
            tb_final_check;
        end
    end

    $display ("INFO:     vlog start:  verilog -> stimc");
    data_in = {DATA_W {1'b0}};
    #(CLKPERIOD);
    data_in = {DATA_W {1'bx}};
    #(CLKPERIOD);
    data_in = {DATA_W {1'bz}};
    #(100*CLKPERIOD);
    data_in = {1'hz, 8'hx, 100'h0, 19'hz};
    #(50*CLKPERIOD);
    data_in = 128'hfedcba98765432100123456789abcdef;
end

initial begin
    #1000;
    $display ("ERROR:    timeout");
    tb_check_failed;
    tb_final_check;
end
