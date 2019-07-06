`timescale 1us/1ps

module tb_uart_stim ();

    wire uart_txrx_0to1;
    wire uart_txrx_1to0;

    wire uart_rtrcts_0to1;
    wire uart_rtrcts_1to0;

    uart_stim #(
        .ID (0)
    ) i_uart_stim_0 (
        .rx_i (uart_txrx_1to0),
        .tx_o (uart_txrx_0to1),

        .cts_i (uart_rtrcts_1to0),
        .rtr_o (uart_rtrcts_0to1)
    );

    uart_stim #(
        .ID (1)
    ) i_uart_stim_1 (
        .rx_i (uart_txrx_0to1),
        .tx_o (uart_txrx_1to0),

        .cts_i (uart_rtrcts_0to1),
        .rtr_o (uart_rtrcts_1to0)
    );

    initial begin
        #(2000);
        $display ("ERROR: timeout");
        #(1);
        $finish ();
    end

endmodule
