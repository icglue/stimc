defparam DATA_W=32;

initial begin
    #1000;
    $display ("ERROR:    timeout");
    tb_check_failed;
    tb_final_check;
end
