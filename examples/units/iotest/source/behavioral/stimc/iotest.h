#ifndef __IOTEST_H__
#define __IOTEST_H__

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

