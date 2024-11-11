
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Unarrest a user who is currently under arrest and in jail
 */
void
unarrest(UR_OBJECT user)
{
    UR_OBJECT u;
    RM_OBJECT rm;
    int on;

    if (word_count < 2) {
        write_user(user, "Usage: unarrest <user>\n");
        return;
    }
    u = retrieve_user(user, word[1]);
    if (!u) {
        return;
    }
    on = retrieve_user_type == 1;
    /* error checks */
    if (u == user) {
        write_user(user, "You cannot unarrest yourself.\n");
        return;
    }
    if (u->level != JAILED) {
        vwrite_user(user, "%s~RS is not under arrest!\n", u->recap);
        done_retrieve(u);
        return;
    }
    if (user->level < u->arrestby) {
        vwrite_user(user, "%s~RS can only be unarrested by a %s or higher.\n",
                u->recap, user_level[u->arrestby].name);
        done_retrieve(u);
        return;
    }
    /* do it */
    u->level = u->unarrest;
    u->real_level = u->level;
    u->arrestby = JAILED; /* FIXME: Use sentinel other JAILED */
    user_list_level(u->name, u->level);
    strcpy(u->date, (long_date(1)));
    sprintf(text, "~FG~OLYou have been unarrested...  Now try to behave!\n");
    if (!on) {
        send_mail(user, u->name, text, 0);
        vwrite_user(user, "%s has been unarrested.\n", u->name);
    } else {
        write_user(u, text);
        vwrite_user(user, "%s has been unarrested.\n", u->name);
        write_room(NULL, "The Hand of Justice reaches through the air...\n");
        rm = get_room_full(amsys->default_warp);
        if (!rm) {
            vwrite_user(user,
                    "Cannot find a room for ex-cons, so %s~RS is still in the %s!\n",
                    u->recap, u->room->name);
        } else {
            move_user(u, rm, 2);
        }
    }
    write_syslog(SYSLOG, 1, "%s UNARRESTED %s\n", user->name, u->name);
    add_history(u->name, 1, "Was ~FGunarrested~RS by %s.\n", user->name);
    if (!on) {
        u->socket = -2;
        strcpy(u->site, u->last_site);
    }
    save_user_details(u, on);
    done_retrieve(u);
}