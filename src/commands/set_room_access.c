
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Set rooms to public or private
 */
void
set_room_access(UR_OBJECT user, int priv)
{
    UR_OBJECT u;
    RM_OBJECT rm;
    const char *name;

    if (word_count < 2) {
        rm = user->room;
    } else {
        if (user->level < amsys->gatecrash_level) {
            write_user(user,
                    "You are not a high enough level to use the room option.\n");
            return;
        }
        rm = get_room(word[1]);
        if (!rm) {
            write_user(user, nosuchroom);
            return;
        }
    }
    if (is_personal_room(rm)) {
        if (rm == user->room) {
            write_user(user, "This room's access is personal.\n");
        } else {
            write_user(user, "That room's access is personal.\n");
        }
        return;
    }
    if (is_fixed_room(rm)) {
        if (rm == user->room) {
            write_user(user, "This room's access is fixed.\n");
        } else {
            write_user(user, "That room's access is fixed.\n");
        }
        return;
    }
    if (priv) {
        if (is_private_room(rm)) {
            if (rm == user->room) {
                write_user(user, "This room is already private.\n");
            } else {
                write_user(user, "That room is already private.\n");
            }
            return;
        }
        if (room_visitor_count(rm) < amsys->min_private_users
                && user->level < amsys->ignore_mp_level) {
            vwrite_user(user,
                    "You need at least %d users/clones in a room before it can be made private.\n",
                    amsys->min_private_users);
            return;
        }
    } else {
        if (!is_private_room(rm)) {
            if (rm == user->room) {
                write_user(user, "This room is already public.\n");
            } else {
                write_user(user, "That room is already public.\n");
            }
            return;
        }
        /* Reset any invites into the room & clear review buffer */
        for (u = user_first; u; u = u->next) {
            if (u->invite_room == rm) {
                u->invite_room = NULL;
            }
        }
        clear_revbuff(rm);
    }
    rm->access ^= PRIVATE;
    name = user->vis ? user->recap : invisname;
    if (rm == user->room) {
        vwrite_room_except(rm, user, "%s~RS has set the room to %s~RS.\n", name,
                is_private_room(rm) ? "~FRPRIVATE" : "~FGPUBLIC");
    } else {
        vwrite_room(rm, "This room has been set to %s~RS.\n",
                is_private_room(rm) ? "~FRPRIVATE" : "~FGPUBLIC");
    }
    vwrite_user(user, "Room set to %s~RS.\n",
            is_private_room(rm) ? "~FRPRIVATE" : "~FGPUBLIC");
}
