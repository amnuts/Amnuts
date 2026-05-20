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
 * Show the user's terminal/telnet negotiation state
 */
void
show_terminal(UR_OBJECT user)
{
    write_user(user, "+----------------------------------------------------------------------------+\n");
    write_user(user, "~FG Terminal Information~RS\n");
    write_user(user, "+----------------------------------------------------------------------------+\n\n");

    if (user->term_width != 80 || user->term_height != 24) {
        vwrite_user(user, "Terminal size : %d x %d (from NAWS)\n", user->term_width, user->term_height);
    } else {
        vwrite_user(user, "Terminal size : %d x %d (default, NAWS not received)\n", user->term_width, user->term_height);
    }
    if (*user->term_type) {
        vwrite_user(user, "Terminal type : %s\n", user->term_type);
    } else {
        write_user(user, "Terminal type : unknown\n");
    }
    int pager = effective_pager(user);
    if (user->pager >= MAX_LINES && user->pager <= 999) {
        vwrite_user(user, "Pager lines   : %d (manually set)\n", pager);
    } else {
        vwrite_user(user, "Pager lines   : %d (auto from terminal height)\n", pager);
    }
    vwrite_user(user, "Word wrap     : %s (width %d)\n", user->wrap ? "on" : "off", effective_wrap(user));
    vwrite_user(user, "Colour        : %s\n", user->colour ? "on" : "off");
    vwrite_user(user, "Char echo     : %s\n", user->charmode_echo ? "on" : "off");
    write_user(user, "\n+----------------------------------------------------------------------------+\n\n");
}
