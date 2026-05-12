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
revafk(UR_OBJECT user)
{
#if !!0
    static const char usage[] = "Usage: revafk\n";
#endif

    start_pager(user);
    lang_user(user, "revafk.header");
    if (!review_buffer(user, rbfAFK)) {
        lang_user(user, "revafk.empty");
    }
    lang_user(user, "revafk.footer");
    stop_pager(user);
}
