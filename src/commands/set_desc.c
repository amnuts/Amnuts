
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Set user description
 */
void
set_desc(UR_OBJECT user, char *inpstr)
{
    if (word_count < 2) {
        vwrite_user(user, "Your current description is: %s\n", user->desc);
        return;
    }
    if (strstr(colour_com_strip(inpstr), "(CLONE)")) {
        write_user(user, "You cannot have that description.\n");
        return;
    }
    if (strlen(inpstr) > USER_DESC_LEN) {
        write_user(user, "Description too long.\n");
        return;
    }
    strcpy(user->desc, inpstr);
    write_user(user, "Description set.\n");
    /* check to see if user should be promoted */
    check_autopromote(user, 2);
}
