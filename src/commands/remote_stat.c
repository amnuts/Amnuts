
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

#ifdef NETLINKS

#include "netlinks.h"

/*
 * Stat a remote system
 */
void
remote_stat(UR_OBJECT user)
{
    NL_OBJECT nl;
    RM_OBJECT rm;

    if (word_count < 2) {
        write_user(user, "Usage: rstat <room service is linked to>\n");
        return;
    }
    rm = get_room(word[1]);
    if (!rm) {
        write_user(user, nosuchroom);
        return;
    }
    nl = rm->netlink;
    if (!nl) {
        write_user(user, "That room is not linked to a service.\n");
        return;
    }
    if (nl->stage != UP) {
        write_user(user, "Not (fully) connected to service.\n");
        return;
    }
    if (nl->ver_major <= 3 && nl->ver_minor < 1) {
        write_user(user,
                "The NUTS version running that service does not support this facility.\n");
        return;
    }
    sprintf(text, "%s %s\n", netcom[NLC_RSTAT], user->name);
    write_sock(nl->socket, text);
    write_user(user, "Request sent.\n");
}

#else

#define NO_NETLINKS

#endif
