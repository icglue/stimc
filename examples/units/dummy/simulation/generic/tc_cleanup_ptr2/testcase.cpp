#include <dummy.h>
#include <tb_selfcheck.h>
#include <stimc++cleanup.h>

using namespace stimcxx;

static constexpr int alen = 256;

static int errors = 0;
static int checks = 0;

#ifdef STIMCXX_DISABLE_STACK_UNWIND
#error "this testcase should be run without STIMCXX_DISABLE_STACK_UNWIND defined"
#endif

void dummy::testcontrol ()
{
    data_out_o = 0;
    wait (clk_event);

    cleanup_ptr<int> pcui (new int (5));

    cleanup_ptr<int[]> pcua (new int[256]);

    *pcui = 21;
    pcua[0] = *pcui;
    for (int i = 1; i < alen; i++) {
        pcua[i] = pcua[i-1] + 1;
    }

    if (pcua[alen-1] != alen-1 + 21) {
        errors++;
    }
    checks++;

    /*********************************************/
    wait (100e-6);
}

void dummy::testcontrol2 ()
{
    wait (clk_event);

    wait (clk_event);
    cleanup_ptr<int[]> pcuanull;
    wait (clk_event);

    cleanup_ptr<int[]> pcua(new int[alen]);
    cleanup_ptr<int> pcui(new int (10));

    *pcui = 3;
    pcua[0] = *pcui;
    for (int i = 1; i < alen; i++) {
        pcua[i] = pcua[i-1] + 1;
    }

    if (pcua[alen-1] != alen-1 + 3) {
        errors++;
    }
    checks++;

    wait (clk_event);

    tb_final_check (checks, errors, false);
}
