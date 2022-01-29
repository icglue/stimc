#include <dummy.h>
#include <tb_selfcheck.h>

using namespace stimcxx;

class cleanup_int_array : public thread_cleanup {
    public:
        int *data;
    public:
        cleanup_int_array () :
            data (nullptr)
        {}

        /* inimal effort to skip copy/move/... */
        cleanup_int_array            (const cleanup_int_array &p) = delete;
        cleanup_int_array& operator= (const cleanup_int_array &p) = delete;

        ~cleanup_int_array ()
        {
            if (data != nullptr) delete[] data;
        }
};

void dummy::testcontrol ()
{
    data_out_o = 0;
    wait (clk_event);

    cleanup_int_array *cu1 = new cleanup_int_array ();

    cleanup_int_array *cu2 __attribute__((unused)) = new cleanup_int_array ();

    int *a = new int[256];
    cu1->data = a;

    /*********************************************/
    wait (100e-6);
}

void dummy::testcontrol2 ()
{
    wait (clk_event);

    cleanup_int_array *cu1 = new cleanup_int_array ();
    wait (clk_event);
    cleanup_int_array *cu2 __attribute__((unused)) = new cleanup_int_array ();
    wait (clk_event);

    int *a = new int[256];
    cu1->data = a;

    wait (clk_event);

    tb_final_check (1, 0, false);
}
