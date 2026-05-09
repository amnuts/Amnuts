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
 * Switch prompt on and off
 */
void
toggle_prompt(UR_OBJECT user)
{
    if (user->prompt) {
        write_user(user, "Prompt ~FROFF.\n");
        user->prompt = 0;
        return;
    }
    write_user(user, "Prompt ~FGON.\n");
    user->prompt = 1;
}
