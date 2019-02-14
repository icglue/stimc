module tb_apb ();

    wire        apb_clk_s;
    wire        apb_resetn_s;
    wire        apb_clk_en_s;

    wire [31:0] apb_addr_s;
    wire        apb_sel_s;
    wire        apb_enable_s;
    wire        apb_write_s;
    wire  [3:0] apb_strb_s;
    wire  [2:0] apb_prot_s;
    wire [31:0] apb_wdata_s;

    wire        apb_ready_s;
    wire [31:0] apb_rdata_s;
    wire        apb_slverr_s;

    wire [31:0] emulator_id_s;

    apb_emulator i_apb_emulator (
        .apb_clk_i     (apb_clk_s),
        .apb_resetn_i  (apb_resetn_s),
        .apb_clk_en_o  (apb_clk_en_s),

        .apb_addr_o    (apb_addr_s),
        .apb_sel_o     (apb_sel_s),
        .apb_enable_o  (apb_enable_s),
        .apb_write_o   (apb_write_s),
        .apb_strb_o    (apb_strb_s),
        .apb_prot_o    (apb_prot_s),
        .apb_wdata_o   (apb_wdata_s),

        .apb_ready_i   (apb_ready_s),
        .apb_rdata_i   (apb_rdata_s),
        .apb_slverr_i  (apb_slverr_s),

        .emulator_id_i (emulator_id_s)
    );

    localparam APB_CLKPERIOD = 2.0;
    reg        apb_clk;
    reg        apb_resetn;

    reg [31:0] apb_rdata;
    reg        apb_ready;
    reg        apb_slverr;

    assign apb_clk_s     = apb_clk;
    assign apb_resetn_s  = apb_resetn;

    assign apb_rdata_s   = apb_rdata;
    assign apb_ready_s   = apb_ready;
    assign apb_slverr_s  = apb_slverr;

    assign emulator_id_s = 32'h0;

    initial begin
        apb_clk    = 1'b0;
        apb_resetn = 1'b0;
        #(3.3*APB_CLKPERIOD);
        apb_resetn = 1'b1;
    end

    always #(APB_CLKPERIOD / 2.0) begin
        apb_clk = ~apb_clk;
    end

    initial begin
        $dumpfile ("dump.vcd");
        $dumpvars (0);
    end

    initial begin
        apb_rdata  = 32'h0;
        apb_ready  =  1'b1;
        apb_slverr =  1'b0;
        #(10*APB_CLKPERIOD);
        $finish ();
    end



endmodule
