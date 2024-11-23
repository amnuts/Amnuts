
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Clone a user in another room
 */
void
create_clone(UR_OBJECT user)
{
    UR_OBJECT u;
    RM_OBJECT rm;
    const char *name;
    int cnt;

    /* Check room */
    if (word_count < 2) {
        rm = user->room;
    } else {
        rm = get_room(word[1]);
        if (!rm) {
            write_user(user, nosuchroom);
            return;
        }
    }
    /* If room is private then no-can-do */
    if (!has_room_access(user, rm)) {
        write_user(user,
                "That room is currently private, you cannot create a clone there.\n");
        return;
    }
    /* Count clones and see if user already has a copy there , no point having 2 in the same room */
    cnt = 0;
    for (u = user_first; u; u = u->next) {
        if (u->type == CLONE_TYPE && u->owner == user) {
            if (u->room == rm) {
                vwrite_user(user, "You already have a clone in the %s.\n", rm->name);
                return;
            }
            if (++cnt == amsys->max_clones) {
                write_user(user,
                        "You already have the maximum number of clones allowed.\n");
                return;
            }
        }
    }
    /* Create clone */
    u = create_user();
    if (!u) {
        vwrite_user(user, "%s: Unable to create copy.\n", syserror);
        write_syslog(SYSLOG | ERRLOG, 0,
                "ERROR: Unable to create user copy in clone().\n");
        return;
    }
    u->type = CLONE_TYPE;
    u->socket = user->socket;
    u->room = rm;
    u->owner = user;
    u->vis = 1;
    strcpy(u->name, user->name);
    strcpy(u->recap, user->recap);
    strcpy(u->bw_recap, colour_com_strip(u->recap));
    strcpy(u->desc, "~BR~OL(CLONE)");
    if (rm == user->room) {
        write_user(user,
                "~FB~OLYou wave your hands, mix some chemicals and a clone is created here.\n");
    } else {
        vwrite_user(user,
                "~FB~OLYou wave your hands, mix some chemicals, and a clone is created in the %s.\n",
                rm->name);
    }
    name = user->vis ? user->bw_recap : invisname;
    vwrite_room_except(user->room, user, "~FB~OL%s waves their hands...\n",
            name);
    vwrite_room_except(rm, user,
            "~FB~OLA clone of %s appears in a swirling magical mist!\n",
            user->bw_recap);
}
