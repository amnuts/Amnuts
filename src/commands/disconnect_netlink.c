
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

#ifdef NETLINKS

/*
 * Disconnect a link
 */
void
disconnect_netlink(UR_OBJECT user)
{
    RM_OBJECT rm;
    NL_OBJECT nl;

    if (word_count < 2) {
        write_user(user, "Usage: disconnect <room service is linked to>\n");
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
    if (nl->type == UNCONNECTED) {
        write_user(user, "That rooms netlink is not connected.\n");
        return;
    }
    /* If link has hung at verification stage do not bother announcing it */
    if (nl->stage == UP) {
        sprintf(text, "~OLSYSTEM:~RS Disconnecting from %s in the %s.\n",
                nl->service, rm->name);
        write_room(NULL, text);
        write_syslog(NETLOG, 1,
                "NETLINK: Link to %s in the %s disconnected by %s.\n",
                nl->service, rm->name, user->name);
    } else {
        write_syslog(NETLOG, 1, "NETLINK: Link to %s disconnected by %s.\n",
                nl->service, user->name);
    }
    shutdown_netlink(nl);
    write_user(user, "Disconnected.\n");
}

#else

#define NO_NETLINKS

#endif
