
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * no longer invite a user to the room you are in if you invited them
 */
void
uninvite(UR_OBJECT user)
{
    UR_OBJECT u;
    const char *name;

    if (word_count < 2) {
        write_user(user, "Uninvite who?\n");
        return;
    }
    u = get_user_name(user, word[1]);
    if (!u) {
        write_user(user, notloggedon);
        return;
    }
    if (u == user) {
        write_user(user, "You cannot uninvite yourself.\n");
        return;
    }
    if (!u->invite_room) {
        vwrite_user(user, "%s~RS has not been invited anywhere.\n", u->recap);
        return;
    }
    if (strcmp(u->invite_by, user->name)) {
        vwrite_user(user, "%s~RS has not been invited anywhere by you!\n",
                u->recap);
        return;
    }
    vwrite_user(user, "You cancel your invitation to %s~RS.\n", u->recap);
    name = user->vis || u->level >= user->level ? user->recap : invisname;
    vwrite_user(u, "%s~RS cancels your invitation.\n", name);
    u->invite_room = NULL;
    *u->invite_by = '\0';
}
