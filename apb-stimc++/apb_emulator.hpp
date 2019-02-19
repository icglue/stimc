#ifndef __APB_EMULATOR_HPP__
#define __APB_EMULATOR_HPP__

#include "stimc++.h"

class apb_emulator : public stimcxx_module {
    private:
        port  apb_clk_i;
        port  apb_resetn_i;
        port  apb_clk_en_o;

        port  apb_addr_o;
        port  apb_sel_o;
        port  apb_enable_o;
        port  apb_write_o;
        port  apb_strb_o;
        port  apb_prot_o;
        port  apb_wdata_o;

        port  apb_ready_i;
        port  apb_rdata_i;
        port  apb_slverr_i;

        port  emulator_id_i;

        stimcxx_event clk_event;
        stimcxx_event reset_release_event;

    public:
        apb_emulator ();
        virtual ~apb_emulator ();

        bool write (uint32_t addr, uint8_t strb, uint32_t wdata);
        bool read (uint32_t addr, uint32_t &rdata);
        void testcontrol ();
        void clock ();
        void reset_release ();
};

#endif