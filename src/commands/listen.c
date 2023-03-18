#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Allows a user to listen to everything again
 */
void
user_listen(UR_OBJECT user)
{
    int yes;

    yes = 0;
    if (user->ignall) {
        user->ignall = 0;
        ++yes;
    }
    if (user->igntells) {
        user->igntells = 0;
        ++yes;
    }
    if (user->ignshouts) {
        user->ignshouts = 0;
        ++yes;
    }
    if (user->ignpics) {
        user->ignpics = 0;
        ++yes;
    }
    if (user->ignlogons) {
        user->ignlogons = 0;
        ++yes;
    }
    if (user->ignwiz) {
        user->ignwiz = 0;
        ++yes;
    }
    if (user->igngreets) {
        user->igngreets = 0;
        ++yes;
    }
    if (user->ignbeeps) {
        user->ignbeeps = 0;
        ++yes;
    }
    if (!yes) {
        write_user(user, "You are already listening to everything.\n");
        return;
    }
    write_user(user, "You listen to everything again.\n");
    if (user->vis) {
        vwrite_room_except(user->room, user,
                "%s~RS is now listening to you all again.\n", user->recap);
    }
}
