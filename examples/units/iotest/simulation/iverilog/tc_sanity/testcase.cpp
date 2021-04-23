#include <iotest.h>
#include <logging.h>
#include <tb_selfcheck.h>

using namespace stimcxx;

volatile unsigned errors = 0;

void iotest::testcontrol ()
{
    log_set_level (LOG_LEVEL_DEBUG);

    wait (10, SC_NS);

    if ((unsigned) ID == 0) {
        gpio_o(0) = 1;
        gpio_o(1) = 0;
        wait (10, SC_NS);

        if ((gpio_i(2) != 1) || (gpio_i(3) != 0)) {
            log_warn ("miscompared gpio in at ID 0");
            errors++;
        }

        gpio_o(2) = 0;
        gpio_o(3) = 1;
        wait (10, SC_NS);
        gpio_o(0) = Z;
        gpio_o(1) = Z;
        wait (10, SC_NS);

        if ((gpio_i(0) != 0) || (gpio_i(1) != 1)) {
            log_warn ("miscompared gpio in at ID 0");
            errors++;
        }
    } else {
        gpio_o(2) = 1;
        gpio_o(3) = 0;
        wait (10, SC_NS);

        if ((gpio_i(0) != 1) || (gpio_i(1) != 0)) {
            log_warn ("miscompared gpio in at ID 1");
            errors++;
        }

        gpio_o(0) = 0;
        gpio_o(1) = 1;
        wait (10, SC_NS);
        gpio_o(2) = Z;
        gpio_o(3) = Z;
        wait (10, SC_NS);

        if ((gpio_i(2) != 0) || (gpio_i(3) != 1)) {
            log_warn ("miscompared gpio in at ID 1");
            errors++;
        }
    }

    if ((unsigned) ID == 0) {
        wait (10, SC_NS);
        tb_final_check (1, errors, false);
    }
}
