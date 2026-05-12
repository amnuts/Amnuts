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
 * Wake up some idle sod
 */
void
wake(UR_OBJECT user)
{
    UR_OBJECT u;
    const char *name;

    /* FIXME: Use sentinel other JAILED */
    if (user->muzzled != JAILED) {
        lang_user(user, "wake.muzzled");
        return;
    }
    if (word_count < 2) {
        lang_user(user, "wake.usage");
        return;
    }
    u = get_user_name(user, word[1]);
    if (!u) {
        write_user(user, notloggedon);
        return;
    }
    if (u == user) {
        lang_user(user, "wake.self");
        return;
    }
    if (check_igusers(u, user) && user->level < GOD) {
        lang_user(user, "wake.ignoring_you", u->recap);
        return;
    }
    if (u->ignbeeps && (user->level < WIZ || u->level > user->level)) {
        lang_user(user, "wake.ignoring_beeps", u->recap);
        return;
    }
    if (u->afk) {
        lang_user(user, "wake.target_afk");
        return;
    }
    if (u->malloc_start) {
        lang_user(user, "wake.target_editor");
        return;
    }
    if (u->ignall && (user->level < WIZ || u->level > user->level)) {
        lang_user(user, "wake.target_ignall");
        return;
    }
#ifdef NETLINKS
    if (!u->room) {
        lang_user(user, "wake.target_offsite", u->recap);
        return;
    }
#endif
    name = user->vis ? user->recap : invisname;
    lang_user(u, "wake.target_message",
            u->ignbeeps ? "" : "\007", name);
    lang_user(user, "wake.sent");
}
