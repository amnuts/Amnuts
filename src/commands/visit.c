#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * let a user go into another user's personal room if it is unlocked
 */
void
personal_room_visit(UR_OBJECT user)
{
    sds rmname;
    RM_OBJECT rm;

    if (word_count < 2) {
        write_user(user, "Usage: visit <user>\n");
        return;
    }
    if (!amsys->personal_rooms) {
        write_user(user, "Personal room functions are currently disabled.\n");
        return;
    }
    /* check if not same user */
    if (!strcasecmp(user->name, word[1])) {
        vwrite_user(user, "To go to your own room use the \"%s\" command.\n",
                command_table[MYROOM].name);
        return;
    }
    /* see if there is such a user */
    if (!find_user_listed(word[1])) {
        write_user(user, nosuchuser);
        return;
    }
    /* get room to go to */
    rmname = sdscatfmt(sdsempty(), "(%s)", word[1]);
    strtolower(rmname);
    rm = get_room_full(rmname);
    sdsfree(rmname);
    if (!rm) {
        write_user(user, nosuchroom);
        return;
    }
    /* can they go there? */
    if (!has_room_access(user, rm)) {
        write_user(user, "That room is currently private, you cannot enter.\n");
        return;
    }
    move_user(user, rm, 1);
}
