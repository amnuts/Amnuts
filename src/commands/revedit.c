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
 * Show recorded tells and pemotes
 */
void
revedit(UR_OBJECT user)
{
#if !!0
    static const char usage[] = "Usage: revedit\n";
#endif

    start_pager(user);
    write_user(user, "\n~BB~FG*** Your EDIT review buffer ***\n\n");
    if (!review_buffer(user, rbfEDIT)) {
        write_user(user, "EDIT buffer is empty.\n");
    }
    write_user(user, "\n~BB~FG*** End ***\n\n");
    stop_pager(user);
}
