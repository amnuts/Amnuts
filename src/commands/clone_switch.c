
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * User swaps places with his own clone. All we do is swap the rooms the objects are in
 */
void
clone_switch(UR_OBJECT user)
{
    UR_OBJECT u;
    RM_OBJECT rm;

    /*
       if no room was given then check to see how many clones user has.  If 1, then
       move the user to that clone, else give an error
     */
    if (word_count < 2) {
        UR_OBJECT tu;

        u = NULL;
        for (tu = user_first; tu; tu = tu->next) {
            if (tu->type == CLONE_TYPE && tu->owner == user) {
                if (u) {
                    write_user(user,
                            "You have more than one clone active.  Please specify a room to switch to.\n");
                    return;
                }
                u = tu;
            }
        }
        if (!u) {
            write_user(user, "You do not currently have any active clones.\n");
            return;
        }
        rm = u->room;
    } else {
        /* if a room name was given then try to switch to a clone there */
        rm = get_room(word[1]);
        if (!rm) {
            write_user(user, nosuchroom);
            return;
        }
        for (u = user_first; u; u = u->next) {
            if (u->type == CLONE_TYPE && u->room == rm && u->owner == user) {
                break;
            }
        }
        if (!u) {
            write_user(user, "You do not have a clone in that room.\n");
            return;
        }
    }
    write_user(user, "\n~FB~OLYou experience a strange sensation...\n");
    u->room = user->room;
    user->room = rm;
    vwrite_room_except(user->room, user, "The clone of %s comes alive!\n",
            u->name);
    vwrite_room_except(u->room, u, "%s~RS turns into a clone!\n", u->recap);
    look(user);
}
