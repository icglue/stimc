#include "dummy.h"
#include "tb_selfcheck.h"

using namespace stimcxx;

static event e1;
static event e2;
static event e3;
static event e4;
static event e5;

static int errors = 0;
static int checks = 0;

static bool check (int id, uint32_t expected, uint32_t actual) {
    checks++;
    if (expected == actual) {
        fprintf (stderr, "Check %d - PASS: value was %d (as expected)\n", id, actual);
        return true;
    } else {
        errors++;
        fprintf (stderr, "Check %d - FAIL: value was %d (expected %d)\n", id, actual, expected);
        return false;
    }
}

void dummy::testcontrol ()
{
    data_out_o = 0;
    wait (clk_event);

    /*********************************************/
    /* check: event combination */
    /*********************************************/
    data_out_o = 1;
    wait (clk_event);
    e1.trigger ();

    data_out_o = 2;
    wait (clk_event);
    e2.trigger ();

    data_out_o = 3;
    wait (clk_event);
    e3.trigger ();

    data_out_o = 4;
    wait (clk_event);
    e4.trigger ();

    data_out_o = 5;
    wait (clk_event);
    e5.trigger ();

    wait (e1 | e2 | e3 | e4 | e5);

    check (2, 11, data_in_i);

    wait (10, SC_NS);
    check (3, 21, data_in_i);

    /*********************************************/
    /* finish */
    /*********************************************/
    wait (clk_event);
    tb_final_check (checks, errors, false);

    /*********************************************/
}

void dummy::testcontrol2 ()
{
    /*********************************************/
    /* check: event combination */
    /*********************************************/
    wait (e1 & e2 & e3 & e4 & e5);

    uint32_t val = data_in_i;

    if (check (1, 5, val)) {
        val = 1;
    } else {
        val = 0;
    }

    data_out_o = val + 10;
    wait (clk_event);
    e2.trigger ();

    wait (2, SC_NS);
    data_out_o = 20;
    e1.trigger ();
    e2.trigger ();
    e3.trigger ();
    e4.trigger ();
    e5.trigger ();

    wait (2, SC_NS);
    data_out_o = 21;

    /*********************************************/
}
