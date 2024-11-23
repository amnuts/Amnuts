
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Invite a user into a private room
 */
void
invite(UR_OBJECT user)
{
    UR_OBJECT u;
    RM_OBJECT rm;
    const char *name;

    if (word_count < 2) {
        write_user(user, "Invite who?\n");
        return;
    }
    rm = user->room;
    if (!is_private_room(rm)) {
        write_user(user, "This room is currently public.\n");
        return;
    }
    u = get_user_name(user, word[1]);
    if (!u) {
        write_user(user, notloggedon);
        return;
    }
    if (u == user) {
        write_user(user,
                "Inviting yourself to somewhere is the third sign of madness.\n");
        return;
    }
    if (u->room == rm) {
        vwrite_user(user, "%s~RS is already here!\n", u->recap);
        return;
    }
    if (u->invite_room == rm) {
        vwrite_user(user, "%s~RS has already been invited into here.\n",
                u->recap);
        return;
    }
    vwrite_user(user, "You invite %s~RS in.\n", u->recap);
    name = user->vis || u->level >= user->level ? user->recap : invisname;
    vwrite_user(u, "%s~RS has invited you into the %s.\n", name, rm->name);
    u->invite_room = rm;
    strcpy(u->invite_by, user->name);
}
