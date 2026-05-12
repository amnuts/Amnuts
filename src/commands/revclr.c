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
 * Clear the review buffer
 */
void
revclr(UR_OBJECT user)
{
#if !!0
    static const char usage[] = "Usage: cbuff\n";
#endif
    const char *name;

    clear_revbuff(user->room);
    name = user->vis ? user->recap : invisname;
    lang_room(user->room, user, "revclr.room_notice", name);
    lang_user(user, "revclr.self");
}
