#include "tb_selfcheck.h"
#include <stimc.h>
#include <logging.h>

void tb_final_check (unsigned checks_done, unsigned errors, bool offensive)
{
        log_print ("%s", "");
        if (checks_done <= 0) {
            if (offensive) {
                log_print (" #####   ####  ### ### ##  ## ##### #####  ");
                log_print (" ##  ## ##  ## ####### ### ## ##    ##  ## ");
                log_print (" ##  ## ###### ## # ## ###### ####  ##  ## ");
                log_print (" ##  ## ##  ## ## # ## ## ### ##    ##  ## ");
                log_print (" #####  ##  ## ##   ## ##  ## ##### #####  ");
                log_print ("%s", "");
            }
            /* unknown */
            log_print (" ##  ## ##  ## ## ## ##  ##  ####  ##   ## ##  ## ");
            log_print (" ##  ## ### ## ####  ### ## ##  ## ## # ## ### ## ");
            log_print (" ##  ## ###### ###   ###### ##  ## ## # ## ###### ");
            log_print (" ##  ## ## ### ####  ## ### ##  ## ####### ## ### ");
            log_print ("  ####  ##  ## ## ## ##  ##  ####   ## ##  ##  ## ");
            log_print ("%s", "");
            log_print ("TBCHECK: UNWNOWN");
        } else if (errors == 0) {
            if (offensive) {
                log_print (" #####  ##    ####   ####  #####  ##  ##         ##   ## ##### ##    ##    ");
                log_print (" ##  ## ##   ##  ## ##  ## ##  ## ##  ##         ## # ## ##    ##    ##    ");
                log_print (" #####  ##   ##  ## ##  ## ##  ##  ####          ## # ## ####  ##    ##    ");
                log_print (" ##  ## ##   ##  ## ##  ## ##  ##   ##           ####### ##    ##    ##    ");
                log_print (" #####  ##### ####   ####  #####    ##            ## ##  ##### ##### ##### ");
                log_print ("%s", "");
            }
            /* passed */
            log_print (" #####   ####   ##### ##### ##### #####  ");
            log_print (" ##  ## ##  ## ##    ##     ##    ##  ## ");
            log_print (" #####  ######  ####  ####  ####  ##  ## ");
            log_print (" ##     ##  ##     ##    ## ##    ##  ## ");
            log_print (" ##     ##  ## ##### #####  ##### #####  ");
            log_print ("%s", "");
            log_print ("TBCHECK: PASSED");
        } else {
            if (offensive) {
                log_print (" ##### ##  ##  ####  ## ## #### ##  ##  ##### ");
                log_print (" ##    ##  ## ##  ## ####   ##  ### ## ##     ");
                log_print (" ####  ##  ## ##     ###    ##  ###### ## ### ");
                log_print (" ##    ##  ## ##  ## ####   ##  ## ### ##  ## ");
                log_print (" ##     ####   ####  ## ## #### ##  ##  ####  ");
                log_print ("%s", "");
            }
            /* failed */
            log_print (" ##### ####  #### ##    ##### #####  ");
            log_print (" ##   ##  ##  ##  ##    ##    ##  ## ");
            log_print (" #### ######  ##  ##    ####  ##  ## ");
            log_print (" ##   ##  ##  ##  ##    ##    ##  ## ");
            log_print (" ##   ##  ## #### ##### ##### #####  ");
            log_print ("%s", "");
            log_print ("TBCHECK: FAILED");
        }
        stimc_finish ();
}
