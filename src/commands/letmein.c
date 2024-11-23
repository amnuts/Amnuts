
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Ask to be let into a private room
 */
void
letmein(UR_OBJECT user)
{
    RM_OBJECT rm;
    int i;

    if (word_count < 2) {
        write_user(user, "Knock on what door?\n");
        return;
    }
    rm = get_room(word[1]);
    if (!rm) {
        write_user(user, nosuchroom);
        return;
    }
    if (rm == user->room) {
        vwrite_user(user, "You are already in the %s!\n", rm->name);
        return;
    }
    for (i = 0; i < MAX_LINKS; ++i)
        if (user->room->link[i] == rm) {
            break;
        }
    if (i >= MAX_LINKS) {
        vwrite_user(user, "The %s is not adjoined to here.\n", rm->name);
        return;
    }
    if (!is_private_room(rm)) {
        vwrite_user(user, "The %s is currently public.\n", rm->name);
        return;
    }
    vwrite_user(user, "You knock asking to be let into the %s.\n", rm->name);
    vwrite_room_except(user->room, user,
            "%s~RS knocks asking to be let into the %s.\n",
            user->recap, rm->name);
    vwrite_room(rm, "%s~RS knocks asking to be let in.\n", user->recap);
}
