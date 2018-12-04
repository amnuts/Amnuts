
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * bring a user to the same room
 */
void
bring(UR_OBJECT user)
{
    UR_OBJECT u;
    RM_OBJECT rm;

    if (word_count < 2) {
        write_user(user, "Usage: bring <user>\n");
        return;
    }
    u = get_user_name(user, word[1]);
    if (!u) {
        write_user(user, notloggedon);
        return;
    }
    rm = user->room;
    if (user == u) {
        write_user(user,
                "You ~OLreally~RS want to bring yourself?!  What would others think?!\n");
        return;
    }
    if (rm == u->room) {
        vwrite_user(user, "%s~RS is already here!\n", u->recap);
        return;
    }
    if (u->level >= user->level && user->level != GOD) {
        write_user(user,
                "You cannot move a user of equal or higher level that yourself.\n");
        return;
    }
    write_user(user, "You chant a mystic spell...\n");
    if (user->vis) {
        vwrite_room_except(user->room, user, "%s~RS chants a mystic spell...\n",
                user->recap);
    } else {
        write_monitor(user, user->room, 0);
        vwrite_room_except(user->room, user, "%s chants a mystic spell...\n",
                invisname);
    }
    move_user(u, rm, 2);
    prompt(u);
}