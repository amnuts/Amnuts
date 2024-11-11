#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * do a promotion of a user that lasts only for the current session
 */
void
temporary_promote(UR_OBJECT user)
{
    UR_OBJECT u;
    enum lvl_value lvl;

    if (word_count < 2) {
        write_user(user, "Usage: tpromote <user> [<level>]\n");
        return;
    }
    u = get_user_name(user, word[1]);
    if (!u) {
        write_user(user, notloggedon);
        return;
    }
    if (u == user) {
        write_user(user,
                "You cannot promote yourself, temporarily or otherwise.\n");
        return;
    }
    /* determine what level to promote them to */
    if (u->level >= user->level) {
        write_user(user,
                "You cannot temporarily promote anyone of the same or greater level than you.\n");
        return;
    }
    if (word_count == 3) {
        lvl = get_level(word[2]);
        if (lvl == NUM_LEVELS) {
            write_user(user, "Usage: tpromote <user> [<level>]\n");
            return;
        }
        if (lvl <= u->level) {
            vwrite_user(user,
                    "You must specify a level higher than %s currently is.\n",
                    u->name);
            return;
        }
    } else {
        lvl = (enum lvl_value) (u->level + 1);
    }
    if (lvl == GOD) {
        vwrite_user(user, "You cannot temporarily promote anyone to level %s.\n",
                user_level[lvl].name);
        return;
    }
    if (lvl >= user->level) {
        write_user(user,
                "You cannot temporarily promote anyone to a higher level than your own.\n");
        return;
    }
    /* if they have already been temp promoted this session then restore normal level first */
    if (u->level > u->real_level) {
        u->level = u->real_level;
    }
    u->real_level = u->level;
    u->level = lvl;
    vwrite_user(user, "You temporarily promote %s to %s.\n", u->name,
            user_level[u->level].name);
    vwrite_room_except(u->room, u,
            "~OL~FG%s~RS~OL~FG starts to glow as their power increases...\n",
            u->bw_recap);
    vwrite_user(u, "~OL~FGYou have been promoted (temporarily) to level %s.\n",
            user_level[u->level].name);
    write_syslog(SYSLOG, 1, "%s TEMPORARILY promote %s to %s.\n", user->name,
            u->name, user_level[u->level].name);
    sprintf(text, "Was temporarily to level %s.\n", user_level[u->level].name);
    add_history(u->name, 1, "%s", text);
}
