#include <dummy.h>
#include <logging.h>
#include <tb_selfcheck.h>

using namespace stimcxx;

static int errors = 0;
static int checks = 0;

struct t_data {
    unsigned t_id;

    event &e_wait;
    event &e_trig;

    t_data (unsigned id, event &w, event &t) : t_id(id), e_wait(w), e_trig(t) {}
};

static unsigned value = 1;

static void func_t (void *data)
{
    struct t_data *d = (struct t_data *) data;

    unsigned id = d->t_id;

    unsigned expected = id;

    log_info ("thread %d: started", id);

    while (true) {
        log_info ("thread %d: waiting for trigger", id);
        wait (d->e_wait);

        log_info ("thread %d: value = %d, expected = %d", id, value, expected);

        if (value != expected) {
            log_error ("thread %d: value <-> expected mismatch", id);
            errors++;
        }
        checks++;

        expected += 3;
        value++;

        log_info ("thread %d: waiting for 1ns", id);
        wait (1, SC_NS);

        log_info ("thread %d: triggering next", id);
        d->e_trig.trigger();
    }
}

static event e1;
static event e2;
static event e3;

static struct t_data d1 (1, e1, e2);
static struct t_data d2 (2, e2, e3);
static struct t_data d3 (3, e3, e1);

void dummy::testcontrol ()
{
    stimc_spawn_thread (func_t, &d1, 0);
    stimc_spawn_thread (func_t, &d2, 0);
    stimc_spawn_thread (func_t, &d3, 0);

    wait (1, SC_NS);
    e1.trigger();

    wait (10, SC_NS);

    log_info ("value = %d", value);
    if (value != 11) {
        log_error ("value != expected value (10)");
        errors++;
    }

    tb_final_check (checks, errors, false);
}
