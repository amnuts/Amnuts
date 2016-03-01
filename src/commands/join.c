
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Join a user in another room
 */
void
join(UR_OBJECT user)
{
    UR_OBJECT u;
    RM_OBJECT rm;

    if (word_count < 2) {
        write_user(user, "Usage: join <user>\n");
        return;
    }
    if (user->lroom == 2) {
        write_user(user, "You have been shackled and cannot move.\n");
        return;
    }
    u = get_user_name(user, word[1]);
    if (!u) {
        write_user(user, notloggedon);
        return;
    }
    if (user == u) {
        write_user(user,
                "You really want join yourself?  What would the neighbours think?\n");
        return;
    }
    rm = u->room;
#ifdef NETLINKS
    if (!rm) {
        vwrite_user(user,
                "%s~RS is currently off site so you cannot join them.\n",
                u->recap);
        return;
    }
#endif
    if (rm == user->room) {
        vwrite_user(user, "You are already with %s~RS in the %s~RS.\n", u->recap,
                rm->show_name);
        return;
    }
    move_user(user, rm, 0);
    vwrite_user(user,
            "~FC~OLYou have joined~RS %s~RS ~FC~OLin the~RS %s~RS~FC~OL.\n",
            u->recap, u->room->show_name);
}