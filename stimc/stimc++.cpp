#include "stimc++.h"

stimcxx_module::stimcxx_module ()
{
    stimc_module_init (&(this->_module));
}

stimcxx_module::~stimcxx_module ()
{
}

stimcxx_module::port::port (stimcxx_module &m, const char *name)
{
    this->_port = stimc_port_init (&(m._module), name);
}

stimcxx_module::port::~port ()
{
}

stimcxx_module::parameter::parameter (stimcxx_module &m, const char *name)
{
    this->_parameter = stimc_parameter_init (&(m._module), name);

    s_vpi_value v;
    v.format = vpiIntVal;
    vpi_get_value (this->_parameter, &v);

    this->value = v.value.integer;
}

stimcxx_module::parameter::~parameter ()
{
}

stimcxx_event::stimcxx_event ()
{
    this->_event = stimc_event_create ();
}

stimcxx_event::~stimcxx_event ()
{
}
