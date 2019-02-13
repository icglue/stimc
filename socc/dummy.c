#include <vpi_user.h>
#include <stdlib.h>

static int socc_dummy_init_cptf (PLI_BYTE8* user_data)
{
    return 0;
}

static int socc_dummy_init_cltf (PLI_BYTE8* user_data)
{
    vpiHandle taskref      = vpi_handle(vpiSysTfCall, NULL);
    vpiHandle taskscope    = vpi_handle(vpiScope, taskref);
    const char *scope_name = vpi_get_str(vpiFullName, taskscope);

    vpi_printf("VPI routine scope: \"%s\"\n", scope_name);

    return 0;
}

static void socc_dummy_register ()
{
    s_vpi_systf_data tf_data;

    tf_data.type      = vpiSysTask;
    tf_data.tfname    = "$socc_dummy_init";
    tf_data.calltf    = socc_dummy_init_cltf;
    tf_data.compiletf = socc_dummy_init_cptf;
    tf_data.sizetf    = 0;
    tf_data.user_data = NULL;

    vpi_register_systf(&tf_data);
}

void (*vlog_startup_routines[])() = {
    socc_dummy_register,
    0
};
