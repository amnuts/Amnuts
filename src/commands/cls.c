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
 * Clear the screen
 */
void
cls(UR_OBJECT user)
{
    int i;
    /* For clients that don't support telnet clear screen escape character, we flood them with new lines... */
    for (i = 0; i < 6; ++i) {
        lang_user(user, "cls.newline_flood");
    }
    /* ...and for the others, here's The Real Thing (TM) */
    lang_user(user, "cls.escape_seq");
}
