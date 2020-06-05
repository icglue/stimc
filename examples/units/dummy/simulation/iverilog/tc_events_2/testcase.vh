defparam DATA_W=32;

always @(data_out_s) begin
    data_in = data_out_s;
end

initial begin
    #1000;
    $display ("ERROR:    timeout");
    tb_check_failed;
    tb_final_check;
end
