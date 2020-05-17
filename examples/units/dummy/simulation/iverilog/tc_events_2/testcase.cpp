#include "dummy.h"
#include "tb_selfcheck.h"

using namespace stimcxx;

static event e;

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
    /*********************************************/
    /* check: event with timeout */
    /*********************************************/

    data_out_o = 0;

    wait (10, SC_NS);
    data_out_o = 1;
    wait (10, SC_NS);

    /* timeout happens here */

    wait (10, SC_NS);
    data_out_o = 2;
    wait (10, SC_NS);

    /* this shoud trigger nothing */
    e.trigger ();
    wait (10, SC_NS);

    data_out_o = 3;

    wait (10, SC_NS);

    /* here read of "3" should happen + second wait */
    wait (10, SC_NS);
    data_out_o = 4;
    wait (10, SC_NS);
    e.trigger ();
    wait (10, SC_NS);
    data_out_o = 5;
    wait (10, SC_NS);

    /*********************************************/
    /* check: event with floating point timeouts */
    /*********************************************/

    data_out_o = 10;

    wait (10e-9);
    data_out_o = 11;
    wait (10e-9);

    /* timeout happens here */

    wait (10e-9);
    data_out_o = 12;
    wait (10e-9);

    /* this shoud trigger nothing */
    e.trigger ();
    wait (10e-9);

    data_out_o = 13;

    wait (10e-9);

    /* here read of "3" should happen + second wait */
    wait (10e-9);
    data_out_o = 14;
    wait (10e-9);
    e.trigger ();
    wait (10e-9);
    data_out_o = 15;
    wait (10e-9);

    /*********************************************/
    /* finish */
    /*********************************************/
    wait (50, SC_NS);
    tb_final_check (checks, errors, false);

    /*********************************************/
}

void dummy::testcontrol2 ()
{
    bool     timeout = false;
    uint32_t val     = 0;

    /*********************************************/
    /* check: event with timeout */
    /*********************************************/

    /* timeout */
    timeout = wait (e, 20, SC_NS);

    check_timeout (1, timeout, true);

    val = data_in_i;
    check (2, 1, val);

    /* event from previous timeout must not trigger here */
    wait (40, SC_NS);

    val = data_in_i;
    check (3, 3, val);

    timeout = wait (e, 50, SC_NS);

    check_timeout (4, timeout, false);

    val = data_in_i;
    check (5, 4, val);

    wait (20, SC_NS);

    val = data_in_i;
    check (5, 5, val);

    /*********************************************/
    /* check: event with floating point timeouts */
    /*********************************************/

    /* timeout */
    timeout = wait (e, 20e-9);

    check_timeout (6, timeout, true);

    val = data_in_i;
    check (7, 11, val);

    /* event from previous timeout must not trigger here */
    wait (40e-9);

    val = data_in_i;
    check (8, 13, val);

    timeout = wait (e, 50e-9);

    check_timeout (9, timeout, false);

    val = data_in_i;
    check (10, 14, val);

    wait (20e-9);

    val = data_in_i;
    check (11, 15, val);

    /*********************************************/
}
