
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Stop a user from using the go command and leaving the room they are currently in
 */
void
shackle(UR_OBJECT user)
{
    UR_OBJECT u;

    if (word_count < 2) {
        write_user(user, "Usage: shackle <user>\n");
        return;
    }
    u = get_user_name(user, word[1]);
    if (!u) {
        write_user(user, notloggedon);
        return;
    }
    if (user == u) {
        write_user(user, "You cannot shackle yourself!\n");
        return;
    }
#ifdef NETLINKS
    if (!u->room) {
        vwrite_user(user,
                "%s~RS is currently off site and cannot be shackled there.\n",
                u->recap);
        return;
    }
#endif
    if (u->level >= user->level) {
        write_user(user,
                "You cannot shackle someone of the same or higher level as yourself.\n");
        return;
    }
    if (u->lroom == 2) {
        vwrite_user(user, "%s~RS has already been shackled.\n", u->recap);
        return;
    }
    u->lroom = 2;
    vwrite_user(u, "\n~FR~OLYou have been shackled to the %s room.\n",
            u->room->name);
    vwrite_user(user, "~FR~OLYou shackled~RS %s~RS ~FR~OLto the %s room.\n",
            u->recap, u->room->name);
    sprintf(text, "~FRShackled~RS to the ~FB%s~RS room by ~FB~OL%s~RS.\n",
            u->room->name, user->name);
    add_history(u->name, 1, "%s", text);
    write_syslog(SYSLOG, 1, "%s SHACKLED %s to the room: %s\n", user->name,
            u->name, u->room->name);
}
