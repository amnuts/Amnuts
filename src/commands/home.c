
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

#ifdef NETLINKS

/*
 * Return to home site
 */
void
home(UR_OBJECT user)
{
    if (user->room) {
        write_user(user, "You are already on your home system.\n");
        return;
    }
    write_user(user, "~FB~OLYou traverse cyberspace...\n");
    write_syslog(NETLOG, 1, "NETLINK: %s returned from %s.\n", user->name,
            user->netlink->service);
    release_nl(user);
    if (user->vis) {
        vwrite_room_except(user->room, user, "%s~RS %s\n", user->recap,
                user->in_phrase);
    } else {
        write_room_except(user->room, invisenter, user);
    }
    look(user);
}

#else

#define NO_NETLINKS

#endif
