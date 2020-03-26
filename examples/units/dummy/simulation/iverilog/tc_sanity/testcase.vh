
initial begin
    data_in = {DATA_W {1'b0}};
    #(2.5*CLKPERIOD);
    data_in = {DATA_W {1'bx}};
    #(CLKPERIOD);
    data_in = 'h12345678;
    #(CLKPERIOD);
    data_in = {DATA_W {1'bz}};
    #(100*CLKPERIOD);
    data_in = {1'hz, 8'hx, 100'h0, 19'hz};
    #(50*CLKPERIOD);
    $finish ();
end
