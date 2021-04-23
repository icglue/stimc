module iotest (
    gpio_b
);

    parameter WIDTH = 8;
    parameter ID    = 0;

    inout [WIDTH-1:0] gpio_b;

    /* stimc bidirectional workaround: dummy in/out port split */
    reg  [WIDTH-1:0] gpio_o;
    wire [WIDTH-1:0] gpio_i;

    assign gpio_b = gpio_o;
    assign gpio_i = gpio_b;

    initial gpio_o = {WIDTH {1'bz}};

    initial begin
        $stimc_iotest_init();
    end
endmodule
