#include "uart_stim.h"

uart_stim::uart_stim () :
    STIMCXX_PARAMETER (ID),

    STIMCXX_PORT (rx_i),
    STIMCXX_PORT (tx_o),

    STIMCXX_PORT (cts_i),
    STIMCXX_PORT (rtr_o)
{
    /* init... */
    mode_boudrate        = 115200;
    mode_with_flowctrl   = false;
    mode_with_2stoppbits = false;
    mode_with_parity     = false;
    mode_parity_odd      = false;

    state_connected = false;

    word_rx_last  = 0;
    word_rx_valid = false;

    STIMCXX_REGISTER_STARTUP_THREAD (testcontrol);
    STIMCXX_REGISTER_STARTUP_THREAD (tx_thread);
    STIMCXX_REGISTER_STARTUP_THREAD (rx_thread);

    STIMCXX_REGISTER_METHOD (negedge, rx_i, rx_negedge);
    STIMCXX_REGISTER_METHOD (posedge, cts_i, cts_posedge);
}

uart_stim::~uart_stim ()
{}

void uart_stim::set_mode (double bps, bool flowctrl, bool second_stopbit, bool parity, bool parity_odd)
{
    this->mode_boudrate = bps;

    this->mode_with_flowctrl   = flowctrl;
    this->mode_with_2stoppbits = second_stopbit;
    this->mode_with_parity     = parity;
    this->mode_parity_odd      = parity_odd;
}

void uart_stim::connect ()
{
    if (state_connected) return;

    tx_o <<= 1;
    if (mode_with_flowctrl) {
        rtr_o <<= 1;
    }

    state_connected = true;
}

void uart_stim::disconnect ()
{
    if (!state_connected) return;

    tx_o.set_z ();
    rtr_o.set_z ();

    state_connected = false;
}

void uart_stim::rx_negedge ()
{
    event_rx_start.trigger ();
}

void uart_stim::cts_posedge ()
{
    event_cts.trigger ();
}

static uint32_t gen_parity (uint32_t word_8bit, bool odd = true)
{
    uint32_t parity;

    parity = word_8bit ^ (word_8bit >> 4);
    parity = parity    ^ (parity    >> 2);
    parity = parity    ^ (parity    >> 1);

    if (odd) parity = ~parity;

    parity = parity & 1;

    return parity;
}

void uart_stim::tx_thread ()
{
    while (true) {
        wait (event_tx_start);

        __sync_synchronize ();

        if (!state_connected) continue;

        uint32_t word = (uint8_t)this->word_tx;

        double bit_time = 1.0 / this->mode_boudrate;

        if (this->mode_with_flowctrl) {
            while (cts_i != 1) {
                wait (event_cts);
            }
        }

        int      seqlen = 10;
        uint32_t seq    = word << 1;
        seq |= 0xf << 9;

        if (mode_with_2stoppbits) seqlen++;
        if (mode_with_parity) {
            seq |= (gen_parity (word, mode_parity_odd) << 9);
            seqlen++;
        }

        for (int i = 0; i < seqlen; i++) {
            tx_o <<= ((seq >> i) & 1);
            wait (bit_time);
        }

        event_tx_done.trigger ();
    }
}

void uart_stim::rx_thread ()
{
    while (true) {
        wait (event_rx_start);

        __sync_synchronize ();

        if (!this->state_connected) continue;

        uint32_t word = 0;

        double bit_time = 1.0 / this->mode_boudrate;

        wait (1.5 * bit_time);

        for (int i = 0; i < 8; i++) {
            word |= (rx_i & 1) << i;
            wait (bit_time);
        }

        bool parity_error = false;
        if (this->mode_with_parity) {
            uint32_t parity_read = rx_i & 1;
            uint32_t parity_calc = gen_parity (word, mode_parity_odd);
            if (parity_read != parity_calc) parity_error = true;

            wait (bit_time);
        }

        bool frame_error = false;

        if ((rx_i & 1) != 1) frame_error = true;

        if (mode_with_2stoppbits) {
            wait (bit_time);
            if ((rx_i & 1) != 1) frame_error = true;
        }

        word_rx_last  = word;
        word_rx_valid = (!parity_error) && (!frame_error);

        __sync_synchronize ();

        event_rx_word.trigger ();
    }
}

void uart_stim::tx_word (char word, bool wait_done)
{
    word_tx = word;
    __sync_synchronize ();
    event_tx_start.trigger ();
    __sync_synchronize ();
    if (wait_done) {
        wait (event_tx_done);
    }
}

bool uart_stim::rx_word (char &word, bool wait_rx)
{
    if (!word_rx_valid) {
        if (!wait_rx) return false;

        wait (event_rx_word);
    }

    __sync_synchronize ();

    if (word_rx_valid) {
        word          = word_rx_last;
        word_rx_valid = false;
        return true;
    }

    return false;
}

void uart_stim::testcontrol ()
{
    connect ();

    wait (100, SC_US);

    const char *msg = "Hello, World!\n";

    if (ID == 0) {
        for (const char *w = msg; *w != '\0'; w++) {
            tx_word (*w);
        }

        wait (10, SC_US);

    } else if (ID == 1) {
        bool error = false;

        for (const char *w = msg; *w != '\0'; w++) {
            char rx;

            if (rx_word (rx)) {
                if (*w == rx) {
                    if (*w != '\n') {
                        printf ("received '%c'\n", rx);
                    }
                } else {
                    printf ("received '%c' (0x%02x) but expected '%c' (0x%02x)\n", rx, rx, *w, *w);
                    error = true;
                }
            }
        }

        if (error) {
            printf ("FAIL\n");
        } else {
            printf ("PASS\n");
        }

        wait (10, SC_US);
        finish ();
    }
}

STIMCXX_INIT (uart_stim)

