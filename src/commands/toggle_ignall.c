
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Switch ignoring all on and off
 */
void
toggle_ignall(UR_OBJECT user)
{
    if (!user->ignall) {
        write_user(user, "You are now ignoring everyone.\n");
        if (user->vis) {
            vwrite_room_except(user->room, user,
                    "%s~RS is now ignoring everyone.\n", user->recap);
        } else {
            vwrite_room_except(user->room, user,
                    "%s~RS is now ignoring everyone.\n", invisname);
        }
        user->ignall = 1;
        return;
    }
    write_user(user, "You will now hear everyone again.\n");
    if (user->vis) {
        vwrite_room_except(user->room, user, "%s~RS is listening again.\n",
                user->recap);
    } else {
        vwrite_room_except(user->room, user, "%s~RS is listening again.\n",
                invisname);
    }
    user->ignall = 0;
}
