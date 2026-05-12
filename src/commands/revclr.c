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
    write_room_lang(user->room, user, "revclr.room_notice", name);
    write_user_lang(user, "revclr.self");
}
