
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Change whether a rooms access is fixed or not
 */
void
change_room_fix(UR_OBJECT user, int fix)
{
    RM_OBJECT rm;
    const char *name;

    if (word_count < 2) {
        rm = user->room;
    } else {
        rm = get_room(word[1]);
        if (!rm) {
            write_user(user, nosuchroom);
            return;
        }
    }
    if (fix) {
        if (is_fixed_room(rm)) {
            if (rm == user->room) {
                write_user(user, "This room's access is already fixed.\n");
            } else {
                write_user(user, "That room's access is already fixed.\n");
            }
            return;
        }
    } else {
        if (!is_fixed_room(rm)) {
            if (rm == user->room) {
                write_user(user, "This room's access is already unfixed.\n");
            } else {
                write_user(user, "That room's access is already unfixed.\n");
            }
            return;
        }
    }
    rm->access ^= FIXED;
    reset_access(rm);
    write_syslog(SYSLOG, 1, "%s %s access to room %s.\n", user->name,
            is_fixed_room(rm) ? "FIXED" : "UNFIXED", rm->name);
    name = user->vis ? user->recap : invisname;
    if (user->room == rm) {
        vwrite_room_except(rm, user, "%s~RS has %s~RS access for this room.\n",
                name, is_fixed_room(rm) ? "~FRFIXED" : "~FGUNFIXED");
    } else {
        vwrite_room(rm, "This room's access has been %s~RS.\n",
                is_fixed_room(rm) ? "~FRFIXED" : "~FGUNFIXED");
    }
    vwrite_user(user, "Access for room %s is now %s~RS.\n", rm->name,
            is_fixed_room(rm) ? "~FRFIXED" : "~FGUNFIXED");
}
