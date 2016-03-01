#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * allows a user to lock their room out to access from anyone
 */
void
personal_room_lock(UR_OBJECT user)
{
    if (!amsys->personal_rooms) {
        write_user(user, "Personal room functions are currently disabled.\n");
        return;
    }
    if (strcmp(user->room->owner, user->name)) {
        write_user(user,
                "You have to be in your personal room to lock and unlock it.\n");
        return;
    }
    user->room->access ^= PRIVATE;
    if (is_private_room(user->room)) {
        write_user(user,
                "You have now ~OL~FRlocked~RS your room to all the other users.\n");
    } else {
        write_user(user,
                "You have now ~OL~FGunlocked~RS your room to all the other users.\n");
    }
    if (!personal_room_store(user->name, 1, user->room))
        write_syslog(SYSLOG | ERRLOG, 1,
            "ERROR: Unable to save personal room status in personal_room_lock()\n");
}