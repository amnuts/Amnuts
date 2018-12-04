
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Move to another room
 */
void
go(UR_OBJECT user)
{
    RM_OBJECT rm;
    int i;

    if (user->lroom == 2) {
        write_user(user, "You have been shackled and cannot move.\n");
        return;
    }
    if (word_count < 2) {
        rm = get_room_full(amsys->default_warp);
        if (!rm) {
            write_user(user, "You cannot warp to the main room at this time.\n");
            return;
        }
        if (user->room == rm) {
            vwrite_user(user, "You are already in the %s!\n", rm->name);
            return;
        }
        move_user(user, rm, 1);
        return;
    }
#ifdef NETLINKS
    release_nl(user);
    if (transfer_nl(user)) {
        return;
    }
#endif
    rm = get_room(word[1]);
    if (!rm) {
        write_user(user, nosuchroom);
        return;
    }
    if (rm == user->room) {
        vwrite_user(user, "You are already in the %s!\n", rm->name);
        return;
    }
    /* See if link from current room */
    for (i = 0; i < MAX_LINKS; ++i) {
        if (user->room->link[i] == rm) {
            break;
        }
    }
    if (i < MAX_LINKS) {
        move_user(user, rm, 0);
        return;
    }
    if (is_personal_room(rm)) {
        write_user(user,
                "To go to another user's home you must \".visit\" them.\n");
        return;
    }
    if (user->level < WIZ) {
        vwrite_user(user, "The %s is not adjoined to here.\n", rm->name);
        return;
    }
    move_user(user, rm, 1);
}
