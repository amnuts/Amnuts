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
 * Muzzle an annoying user so he cant speak, emote, echo, write, smail
 * or bcast. Muzzles have levels from WIZ to GOD so for instance a wiz
 * cannot remove a muzzle set by a god
 */
void
muzzle(UR_OBJECT user)
{
    UR_OBJECT u;
    int on;

    if (word_count < 2) {
        lang_user(user, "muzzle.usage");
        return;
    }
    u = retrieve_user(user, word[1]);
    if (!u) {
        return;
    }
    on = retrieve_user_type == 1;
    /* error checks */
    if (u == user) {
        lang_user(user, "muzzle.self");
        return;
    }
    if (u->level >= user->level) {
        lang_user(user, "muzzle.higher_level");
        done_retrieve(u);
        return;
    }
    if (u->muzzled >= user->level) {
        lang_user(user, "muzzle.already", u->recap);
        done_retrieve(u);
        return;
    }
    /* do the muzzle */
    u->muzzled = user->level;
    lang_user(user, "muzzle.applied", u->bw_recap,
            user_level[user->level].name);
    write_syslog(SYSLOG, 1, "%s muzzled %s (level %d).\n", user->name, u->name,
            user->level);
    add_history(u->name, 1, "Level %d (%s) ~FRmuzzle~RS put on by %s.\n",
            user->level, user_level[user->level].name, user->name);
    sprintf(text, "~FR~OLYou have been muzzled!\n");
    if (!on) {
        send_mail(user, u->name, text, 0);
    } else {
        write_user(u, text);
    }
    /* finish up */
    if (!on) {
        strcpy(u->site, u->last_site);
        u->socket = -2;
    }
    save_user_details(u, on);
    done_retrieve(u);
}
