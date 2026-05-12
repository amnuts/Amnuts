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

#ifdef NETLINKS

/*
 * Return to home site
 */
void
home(UR_OBJECT user)
{
    if (user->room) {
        lang_user(user, "home.already_home");
        return;
    }
    lang_user(user, "home.traverse");
    write_syslog(NETLOG, 1, "NETLINK: %s returned from %s.\n", user->name,
            user->netlink->service);
    release_nl(user);
    if (user->vis) {
        lang_room(user->room, user, "home.room_arrival", user->recap,
                user->in_phrase);
    } else {
        write_room_except(user->room, invisenter, user);
    }
    look(user);
}

#else

#define NO_NETLINKS

#endif
