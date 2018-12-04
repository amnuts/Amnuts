
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Allow a user to move between rooms again
 */
void
unshackle(UR_OBJECT user)
{
    UR_OBJECT u;

    if (word_count < 2) {
        write_user(user, "Usage: unshackle <user>\n");
        return;
    }
    u = get_user_name(user, word[1]);
    if (!u) {
        write_user(user, notloggedon);
        return;
    }
    if (user == u) {
        write_user(user, "You cannot unshackle yourself!\n");
        return;
    }
    if (u->lroom != 2) {
        vwrite_user(user, "%s~RS in not currently shackled.\n", u->recap);
        return;
    }
    u->lroom = 0;
    write_user(u, "\n~FG~OLYou have been unshackled.\n");
    write_user(u,
            "You can now use the ~FCset~RS command to alter the ~FBroom~RS attribute.\n");
    vwrite_user(user, "~FG~OLYou unshackled~RS %s~RS ~FG~OLfrom the %s room.\n",
            u->recap, u->room->name);
    sprintf(text, "~FGUnshackled~RS from the ~FB%s~RS room by ~FB~OL%s~RS.\n",
            u->room->name, user->name);
    add_history(u->name, 1, "%s", text);
    write_syslog(SYSLOG, 1, "%s UNSHACKLED %s from the room: %s\n", user->name,
            u->name, u->room->name);
}
