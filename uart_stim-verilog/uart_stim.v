module uart_stim (
    rx_i,
    tx_o,

    cts_i,
    rtr_o
);

    parameter ID = 0;

    input  rx_i;
    output tx_o;

    input  cts_i;
    output rtr_o;

    initial begin
        $stimc_uart_stim_init();
    end

endmodule
