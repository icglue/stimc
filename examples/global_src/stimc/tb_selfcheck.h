#ifndef GLOBAL_TB_SELFCHECK_H
#define GLOBAL_TB_SELFCHECK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

void tb_final_check (unsigned checks_done, unsigned errors, bool offensive);

#ifdef __cplusplus
}
#endif

#endif

