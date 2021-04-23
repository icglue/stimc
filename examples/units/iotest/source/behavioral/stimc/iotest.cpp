#include "iotest.h"

iotest::iotest () :
    STIMCXX_PARAMETER (WIDTH),
    STIMCXX_PARAMETER (ID),

    STIMCXX_PORT (gpio_i),
    STIMCXX_PORT (gpio_o)
{
    /* init... */
    gpio_o <<= stimcxx::Z;

    STIMCXX_REGISTER_STARTUP_THREAD (testcontrol);
}

iotest::~iotest ()
{}

void __attribute__((weak)) iotest::testcontrol ()
{}

#if !defined NO_STIMCXX_EXPORT
    STIMCXX_EXPORT (iotest)
#else
    STIMCXX_INIT (iotest)
#endif

