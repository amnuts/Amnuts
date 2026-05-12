/****************************************************************************
             Amnuts - Copyright (C) Andrew Collington, 1996-2026

                   talker@amnuts.net - https://amnuts.net/

                                 based on

   NUTS version 3.3.3 (Triple Three :) - Copyright (C) Neil Robertson 1996
 ***************************************************************************/

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
        write_user_lang(user, "set_desc.current", user->desc);
        return;
    }
    if (strstr(colour_com_strip(inpstr), "(CLONE)")) {
        write_user_lang(user, "set_desc.disallowed");
        return;
    }
    if (strlen(inpstr) > USER_DESC_LEN) {
        write_user_lang(user, "set_desc.too_long");
        return;
    }
    strcpy(user->desc, inpstr);
    write_user_lang(user, "set_desc.set");
    /* check to see if user should be promoted */
    check_autopromote(user, 2);
}
