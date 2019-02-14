module apb_emulator (
    apb_clk_i,
    apb_resetn_i,
    apb_clk_en_o,

    apb_addr_o,
    apb_sel_o,
    apb_enable_o,
    apb_write_o,
    apb_strb_o,
    apb_prot_o,
    apb_wdata_o,

    apb_ready_i,
    apb_rdata_i,
    apb_slverr_i,

    emulator_id_i
);


    input         apb_clk_i;
    input         apb_resetn_i;
    input         apb_clk_en_o;

    output [31:0] apb_addr_o;
    output        apb_sel_o;
    output        apb_enable_o;
    output        apb_write_o;
    output  [3:0] apb_strb_o;
    output  [2:0] apb_prot_o;
    output [31:0] apb_wdata_o;

    input         apb_ready_i;
    input  [31:0] apb_rdata_i;
    input         apb_slverr_i;

    input  [31:0] emulator_id_i;

    initial begin
        $stimc_apb_emulator_init();
    end

endmodule
