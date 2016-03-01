#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * allow a user to bump others from their personal room
 */
void
personal_room_bgone(UR_OBJECT user)
{
    RM_OBJECT rm;
    UR_OBJECT u;

    if (!amsys->personal_rooms) {
        write_user(user, "Personal room functions are currently disabled.\n");
        return;
    }
    if (word_count < 2) {
        write_user(user, "Usage: mybgone <user>/all\n");
        return;
    }
    if (strcmp(user->room->owner, user->name)) {
        write_user(user,
                "You have to be in your personal room to bounce people from it.\n");
        return;
    }
    /* get room to bounce people to */
    rm = get_room_full(amsys->default_warp);
    if (!rm) {
        write_user(user,
                "No one can be bounced from your personal room at this time.\n");
        return;
    }
    strtolower(word[1]);
    /* bounce everyone out - except GODS */
    if (!strcmp(word[1], "all")) {
        for (u = user_first; u; u = u->next) {
            if (u == user || u->room != user->room || u->level == GOD) {
                continue;
            }
            vwrite_user(user, "%s~RS is forced to leave the room.\n", u->recap);
            write_user(u, "You are being forced to leave the room.\n");
            move_user(u, rm, 0);
        }
        return;
    }
    /* send out just the one user */
    u = get_user_name(user, word[1]);
    if (!u) {
        write_user(user, notloggedon);
        return;
    }
    if (u->room != user->room) {
        vwrite_user(user, "%s~RS is not in your personal room.\n", u->recap);
        return;
    }
    if (u->level == GOD) {
        vwrite_user(user, "%s~RS cannot be forced to leave your personal room.\n",
                u->recap);
        return;
    }
    vwrite_user(user, "%s~RS is forced to leave the room.\n", u->recap);
    write_user(u, "You are being forced to leave the room.\n");
    move_user(u, rm, 0);
}
