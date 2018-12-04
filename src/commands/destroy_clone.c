
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Destroy user clone
 */
void
destroy_clone(UR_OBJECT user)
{
    UR_OBJECT u, u2;
    RM_OBJECT rm;
    const char *name;

    /* Check room and user */
    if (word_count < 2) {
        rm = user->room;
    } else {
        rm = get_room(word[1]);
        if (!rm) {
            write_user(user, nosuchroom);
            return;
        }
    }
    if (word_count > 2) {
        u2 = get_user_name(user, word[2]);
        if (!u2) {
            write_user(user, notloggedon);
            return;
        }
        if (u2->level >= user->level) {
            write_user(user,
                    "You cannot destroy the clone of a user of an equal or higher level.\n");
            return;
        }
    } else {
        u2 = user;
    }
    for (u = user_first; u; u = u->next) {
        if (u->type == CLONE_TYPE && u->room == rm && u->owner == u2) {
            break;
        }
    }
    if (!u) {
        if (u2 == user) {
            vwrite_user(user, "You do not have a clone in the %s.\n", rm->name);
        } else {
            vwrite_user(user, "%s~RS does not have a clone the %s.\n", u2->recap,
                    rm->name);
        }
        return;
    }
    destruct_user(u);
    reset_access(rm);
    write_user(user,
            "~FM~OLYou whisper a sharp spell and the clone is destroyed.\n");
    name = user->vis ? user->bw_recap : invisname;
    vwrite_room_except(user->room, user, "~FM~OL%s whispers a sharp spell...\n",
            name);
    vwrite_room(rm, "~FM~OLThe clone of %s shimmers and vanishes.\n",
            u2->bw_recap);
    if (u2 != user) {
        vwrite_user(u2, "~OLSYSTEM: ~FR%s has destroyed your clone in the %s.\n",
                user->bw_recap, rm->name);
    }
    destructed = 0;
}
