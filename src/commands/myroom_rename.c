#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * allows a user to rename their room
 */
void
personal_room_rename(UR_OBJECT user, char *inpstr)
{
    if (!amsys->personal_rooms) {
        write_user(user, "Personal room functions are currently disabled.\n");
        return;
    }
    if (word_count < 2) {
        write_user(user, "Usage: myname <name you want room to have>\n");
        return;
    }
    if (strcmp(user->room->owner, user->name)) {
        write_user(user, "You have to be in your personal room to rename it.\n");
        return;
    }
    if (strlen(inpstr) > PERSONAL_ROOMNAME_LEN) {
        write_user(user, "You cannot have a room name that long.\n");
        return;
    }
    if (strlen(inpstr) - teslen(inpstr, 0) < 1) {
        write_user(user, "You must enter a room name.\n");
        return;
    }
    strcpy(user->room->show_name, inpstr);
    vwrite_user(user, "You have now renamed your room to: %s\n",
            user->room->show_name);
    if (!personal_room_store(user->name, 1, user->room)) {
        write_syslog(SYSLOG | ERRLOG, 1,
                "ERROR: Unable to save personal room status in personal_room_rename()\n");
    }
}
