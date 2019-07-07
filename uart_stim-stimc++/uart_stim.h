#ifndef __UART_STIM_H__
#define __UART_STIM_H__

#include "stimc++.h"

class uart_stim : public stimcxx_module {
    private:
        parameter ID;

        port rx_i;
        port tx_o;

        port cts_i;
        port rtr_o;

        stimcxx_event event_rx_start;
        stimcxx_event event_rx_word;
        stimcxx_event event_tx_start;
        stimcxx_event event_tx_done;
        stimcxx_event event_cts;

        char word_rx_last;
        bool word_rx_valid;

        char word_tx;

        double mode_boudrate;

        bool mode_with_flowctrl;
        bool mode_with_2stoppbits;
        bool mode_with_parity;
        bool mode_parity_odd;

        bool state_connected;

    private:
        void tx_thread ();
        void rx_thread ();

        void cts_posedge ();
        void rx_negedge ();

    protected:


    public:
        uart_stim ();
        virtual ~uart_stim ();

        void connect ();
        void disconnect ();

        void set_mode (double bps, bool flowctrl = false, bool second_stopbit = false, bool parity = false, bool parity_odd = false);

        void tx_word (char word, bool wait_done = true);
        bool rx_word (char &word, bool wait_rx  = true);

        void testcontrol ();
};

#endif

