
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Put annoying user in jail
 */
void
arrest(UR_OBJECT user)
{
    UR_OBJECT u;
    RM_OBJECT rm;
    int on;

    if (word_count < 2) {
        write_user(user, "Usage: arrest <user>\n");
        return;
    }
    u = retrieve_user(user, word[1]);
    if (!u) {
        return;
    }
    on = retrieve_user_type == 1;
    /* error checks */
    if (u == user) {
        write_user(user, "You cannot arrest yourself.\n");
        return;
    }
    if (u->level >= user->level) {
        write_user(user,
                "You cannot arrest anyone of the same or higher level than yourself.\n");
        done_retrieve(u);
        return;
    }
    if (u->level == JAILED) {
        vwrite_user(user, "%s~RS has already been arrested.\n", u->recap);
        done_retrieve(u);
        return;
    }
    /* do it */
    u->vis = 1;
    u->unarrest = u->level;
    u->arrestby = user->level;
    u->level = JAILED;
    u->real_level = u->level;
    user_list_level(u->name, u->level);
    strcpy(u->date, (long_date(1)));
    sprintf(text, "~FR~OLYou have been placed under arrest.\n");
    if (!on) {
        send_mail(user, u->name, text, 0);
        vwrite_user(user, "%s has been placed under arrest.\n", u->name);
    } else {
        write_user(u, text);
        vwrite_user(user, "%s has been placed under arrest.\n", u->name);
        write_room(NULL, "The Hand of Justice reaches through the air...\n");
        rm = get_room_full(amsys->default_jail);
        if (!rm) {
            vwrite_user(user,
                    "Cannot find the jail, so %s~RS is arrested but still in the %s.\n",
                    u->recap, u->room->name);
        } else {
            move_user(u, rm, 2);
        }
        vwrite_room_except_both(NULL, user, u,
                "%s~RS has been placed under arrest...\n",
                u->recap);
    }
    write_syslog(SYSLOG, 1, "%s ARRESTED %s (at level %s)\n", user->name,
            u->name, user_level[u->arrestby].name);
    add_history(u->name, 1, "Was ~FRarrested~RS by %s (at level ~OL%s~RS).\n",
            user->name, user_level[u->arrestby].name);
    if (!on) {
        u->socket = -2;
        strcpy(u->site, u->last_site);
    }
    save_user_details(u, on);
    done_retrieve(u);
}