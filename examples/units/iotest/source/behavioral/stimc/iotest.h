#ifndef STIMCXX_MODULE_IOTEST_H
#define STIMCXX_MODULE_IOTEST_H

#include "stimc++.h"

class iotest : public stimcxx::module {
    private:
        parameter WIDTH;
        parameter ID;

        port gpio_i;
        port gpio_o;

    public:
        iotest ();
        virtual ~iotest ();

        void testcontrol ();
};

#endif

