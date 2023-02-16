
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Force a user to become invisible
 */
void
make_invis(UR_OBJECT user)
{
    UR_OBJECT user2;

    if (word_count < 2) {
        write_user(user, "Usage: makeinvis <user>\n");
        return;
    }
    user2 = get_user_name(user, word[1]);
    if (!user2) {
        write_user(user, notloggedon);
        return;
    }
    if (user == user2) {
        write_user(user, "There is an easier way to make yourself invisible!\n");
        return;
    }
    if (!user2->vis) {
        vwrite_user(user, "%s~RS is already invisible.\n", user2->recap);
        return;
    }
    if (user2->level > user->level) {
        vwrite_user(user, "%s~RS cannot be forced invisible.\n", user2->recap);
        return;
    }
    user2->vis = 0;
    vwrite_user(user, "You force %s~RS to become invisible.\n", user2->recap);
    write_user(user2, "You have been forced to become invisible.\n");
    vwrite_room_except(user2->room, user2,
            "You see %s~RS mysteriously disappear into the shadows!\n",
            user2->recap);
}
