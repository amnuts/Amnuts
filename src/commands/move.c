
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Wizard moves a user to another room
 */
void
move(UR_OBJECT user)
{
    UR_OBJECT u;
    RM_OBJECT rm;
    const char *name;

    if (word_count < 2) {
        write_user(user, "Usage: move <user> [<room>]\n");
        return;
    }
    u = get_user_name(user, word[1]);
    if (!u) {
        write_user(user, notloggedon);
        return;
    }
    if (u->login) {
        write_user(user, "That user is not logged in.\n");
        return;
    }
    if (word_count < 3) {
        rm = user->room;
    } else {
        rm = get_room(word[2]);
        if (!rm) {
            write_user(user, nosuchroom);
            return;
        }
    }
    if (user == u) {
        write_user(user,
                "Trying to move yourself this way is the fourth sign of madness.\n");
        return;
    }
    if (u->level >= user->level) {
        write_user(user,
                "You cannot move a user of equal or higher level than yourself.\n");
        return;
    }
    if (rm == u->room) {
        vwrite_user(user, "%s~RS is already in the %s.\n", u->recap, rm->name);
        return;
    };
    if (!has_room_access(user, rm)) {
        vwrite_user(user,
                "The %s is currently private, %s~RS cannot be moved there.\n",
                rm->name, u->recap);
        return;
    }
    write_user(user, "~FC~OLYou chant an ancient spell...\n");
    name = user->vis ? user->recap : invisname;
    if (!user->vis) {
        write_monitor(user, user->room, 0);
    }
    vwrite_room_except(user->room, user,
            "%s~RS ~FC~OLchants an ancient spell...\n", name);
    move_user(u, rm, 2);
    prompt(u);
}
