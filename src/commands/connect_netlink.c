
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

#ifdef NETLINKS

/*
 * Connect a netlink. Use the room as the key
 */
void
connect_netlink(UR_OBJECT user)
{
    RM_OBJECT rm;
    NL_OBJECT nl;

    if (word_count < 2) {
        write_user(user, "Usage: connect <room service is linked to>\n");
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
    if (nl->type != UNCONNECTED) {
        write_user(user, "That rooms netlink is already up.\n");
        return;
    }
    write_user(user,
            "Attempting connect (this may cause a temporary hang)...\n");
    write_syslog(NETLOG, 1,
            "NETLINK: Connection attempt to %s initiated by %s.\n",
            nl->service, user->name);
    nl->socket = socket_connect(nl->site, nl->port);
    if (nl->socket < 0) {
        vwrite_user(user, "~FRConnect failed: %s.\n",
                nl->socket != -1 ? "Unknown hostname" : strerror(errno));
        write_syslog(NETLOG, 1, "NETLINK: Failed to connect to %s: %s.\n",
                nl->service,
                nl->socket != -1 ? "Unknown hostname" : strerror(errno));
        return;
    }
    nl->type = OUTGOING;
    nl->stage = VERIFYING;
    nl->last_recvd = time(0);
    nl->connect_room = rm;
    write_user(user, "~FGInitial connection made...\n");
    write_syslog(NETLOG, 1, "NETLINK: Connected to %s (%s:%s).\n", nl->service,
            nl->site, nl->port);
}

#else

#define NO_NETLINKS

#endif
