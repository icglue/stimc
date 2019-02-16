#ifndef __STIMCXX_H__
#define __STIMCXX_H__

#include <stimc.h>

class stimcxx_module {
    private:
        stimc_module _module;
    public:
        stimcxx_module ();
        virtual ~stimcxx_module ();


    public:
        class port {
            private:
                stimc_port _port;
            public:
                port (stimcxx_module &m, const char *name);
                virtual ~port ();
        };

        class parameter {
            private:
                stimc_parameter _parameter;
                int32_t value;
            public:
                parameter (stimcxx_module &m, const char *name);
                virtual ~parameter ();
        };
};

class stimcxx_event {
    private:
        stimc_event _event;
    public:
        stimcxx_event ();
        virtual ~stimcxx_event ();
};


#define STIMCXX_INIT(module) \
static int _stimcxx_module_ ## module ## _init_cptf (PLI_BYTE8* user_data __attribute__((unused)))\
{\
    return 0;\
}\
\
static int _stimcxx_module_ ## module ## _init_cltf (PLI_BYTE8* user_data __attribute__((unused)))\
{\
    module *m __attribute__((unused));\
    m = new module ();\
\
    return 0;\
}\
\
static void _stimcxx_module_ ## module ## _register (void)\
{\
    s_vpi_systf_data tf_data;\
\
    tf_data.type      = vpiSysTask;\
    tf_data.tfname    = "$stimc_" #module "_init";\
    tf_data.calltf    = _stimcxx_module_ ## module ## _init_cltf;\
    tf_data.compiletf = _stimcxx_module_ ## module ## _init_cptf;\
    tf_data.sizetf    = 0;\
    tf_data.user_data = NULL;\
\
    vpi_register_systf(&tf_data);\
}

#define STIMCXX_EXPORT(module)\
    _stimcxx_module_ ## module ## _register,

#endif
