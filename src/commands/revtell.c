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
revtell(UR_OBJECT user)
{
#if !!0
    static const char usage[] = "Usage: revtell\n";
#endif

    start_pager(user);
    write_user(user, "\n~BB~FG*** Your tell buffer ***\n\n");
    if (!review_buffer(user, rbfTELL)) {
        write_user(user, "Revtell buffer is empty.\n");
    }
    write_user(user, "\n~BB~FG*** End ***\n\n");
    stop_pager(user);
}
