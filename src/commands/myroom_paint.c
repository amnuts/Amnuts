#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Enter a description for a personal room
 */
void
personal_room_decorate(UR_OBJECT user, char *inpstr)
{
    if (inpstr) {
        if (!amsys->personal_rooms) {
            write_user(user, "Personal room functions are currently disabled.\n");
            return;
        }
        if (strcmp(user->room->owner, user->name)) {
            write_user(user,
                    "You have to be in your personal room to decorate it.\n");
            return;
        }
        if (word_count < 2) {
            write_user(user, "\n~BB*** Decorating your personal room ***\n\n");
            user->misc_op = 19;
            editor(user, NULL);
            return;
        }
        strcat(inpstr, "\n");
    } else {
        inpstr = user->malloc_start;
    }
    *user->room->desc = '\0';
    strncat(user->room->desc, inpstr, ROOM_DESC_LEN);
    if (strlen(user->room->desc) < strlen(inpstr)) {
        vwrite_user(user, "The description is too long for the room \"%s\".\n",
                user->room->name);
    }
    write_user(user, "You have now redecorated your personal room.\n");
    if (!personal_room_store(user->name, 1, user->room)) {
        write_syslog(SYSLOG | ERRLOG, 1,
                "ERROR: Unable to save personal room status in personal_room_decorate()\n");
    }
}