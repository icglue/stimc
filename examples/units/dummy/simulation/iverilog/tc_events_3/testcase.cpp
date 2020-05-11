#include "dummy.hpp"
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

static bool check_timeout (int id, bool timeout, bool expected) {
    checks++;
    fprintf (stderr, "Check %d - %s: %stimeout when expecting %s\n", id,
            (timeout == expected) ? "PASS" : "FAIL",
            timeout ? "" : "no ",
            expected ? "one" : "none");

    if (expected == timeout) {
        return true;
    } else {
        errors++;
        return false;
    }
}

void dummy::testcontrol ()
{
    bool timeout = false;

    /*********************************************/
    /* check: event combination and timeout */
    /*********************************************/

    data_out_o = 0;
    wait (5, SC_NS);
    data_out_o = 1;
    wait (10, SC_NS);
    e1.trigger ();
    wait (10, SC_NS);
    data_out_o = 2;
    wait (10, SC_NS);
    e2.trigger ();
    wait (10, SC_NS);
    data_out_o = 3;
    wait (10, SC_NS); /* timeout happens here */
    e3.trigger ();
    wait (10, SC_NS);
    data_out_o = 4;
    wait (10, SC_NS);
    e4.trigger ();
    wait (10, SC_NS);
    data_out_o = 5;
    wait (10, SC_NS);
    e5.trigger ();
    wait (10, SC_NS);
    data_out_o = 6;

    /*********************************************/

    timeout = wait (e1 | e2 | e3 | e4 | e5, 50, SC_NS);

    check_timeout (4, timeout, false);
    check (5, 11, data_in_i);

    wait (20, SC_NS);
    check (6, 12, data_in_i);
    wait (20, SC_NS);

    /*********************************************/

    data_out_o = 20;
    wait (5, SC_NS);
    data_out_o = 21;
    wait (10, SC_NS);
    e1.trigger ();
    wait (10, SC_NS);
    data_out_o = 22;
    wait (10, SC_NS);
    e2.trigger ();
    wait (10, SC_NS);
    data_out_o = 23;
    wait (10, SC_NS); /* timeout happens here */
    e3.trigger ();
    wait (10, SC_NS);
    data_out_o = 24;
    wait (10, SC_NS);
    e4.trigger ();
    wait (10, SC_NS);
    data_out_o = 25;
    wait (10, SC_NS);
    e5.trigger ();
    wait (10, SC_NS);
    data_out_o = 26;
    wait (40, SC_NS);
    data_out_o = 27;
    wait (10, SC_NS);

    /*********************************************/

    timeout = wait (e1 | e2 | e4 | e5, 50e-9);

    check_timeout (10, timeout, true);
    check (11, 32, data_in_i);

    wait (35, SC_NS);
    check (12, 34, data_in_i);

    wait (20, SC_NS);

    /*********************************************/
    /* finish */
    /*********************************************/
    wait (clk_event);
    tb_final_check (checks, errors, false);

    /*********************************************/
}

void dummy::testcontrol2 ()
{
    bool timeout = false;

    /*********************************************/
    /* check: event combination and timeout */
    /*********************************************/

    timeout = wait (e1 & e2 & e3 & e4 & e5, 50, SC_NS);

    check_timeout (1, timeout, true);

    check (2, 3, data_in_i);

    wait (60, SC_NS);

    check (3, 6, data_in_i);

    /*********************************************/

    data_out_o = 10;
    wait (20, SC_NS);
    data_out_o = 11;
    wait (10, SC_NS);
    e4.trigger();
    wait (10, SC_NS);
    data_out_o = 12;
    wait (30, SC_NS);
    data_out_o = 13;

    /*********************************************/

    timeout = wait (e1 & e2 & e3 & e4 & e5, 120e-9);

    check_timeout (7, timeout, false);

    check (8, 25, data_in_i);

    wait (60, SC_NS);

    check (9, 27, data_in_i);
    /*********************************************/

    data_out_o = 30;
    wait (20, SC_NS);
    data_out_o = 31;
    wait (10, SC_NS);
    e3.trigger();
    wait (10, SC_NS);
    data_out_o = 32;
    wait (20, SC_NS); /* timeout here */
    data_out_o = 33;
    wait (10, SC_NS);
    e4.trigger();
    wait (10, SC_NS);
    data_out_o = 34;
    wait (10, SC_NS);
    data_out_o = 35;

    /*********************************************/
}
