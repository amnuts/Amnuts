
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

#ifdef NETLINKS

#include "netlinks.h"

/*
 * Show type of data being received down links (this is usefull when a link has hung)
 */
void
netdata(UR_OBJECT user)
{
    char from[80], name[USER_NAME_LEN + 1];
    NL_OBJECT nl;
    int cnt;

    cnt = 0;
    write_user(user, "\n~BB*** Mail receiving status ***\n\n");
    for (nl = nl_first; nl; nl = nl->next) {
        if (nl->type == UNCONNECTED || !nl->mailfile) {
            continue;
        }
        if (!cnt++) {
            write_user(user,
                    "To              : From                       Last recv.\n\n");
        }
        sprintf(from, "%s@%s", nl->mail_from, nl->service);
        sprintf(text, "%-15s : %-25s  %d seconds ago.\n", nl->mail_to, from,
                (int) (time(0) - nl->last_recvd));
        write_user(user, text);
    }
    if (!cnt) {
        write_user(user, "No mail being received.\n\n");
    } else {
        write_user(user, "\n");
    }
    cnt = 0;
    write_user(user, "\n~BB*** Message receiving status ***\n\n");
    for (nl = nl_first; nl; nl = nl->next) {
        if (nl->type == UNCONNECTED || !nl->mesg_user) {
            continue;
        }
        if (!cnt++) {
            write_user(user, "To              : From             Last recv.\n\n");
        }
        if (nl->mesg_user == (UR_OBJECT) - 1) {
            strcpy(name, "<unknown>");
        } else {
            strcpy(name, nl->mesg_user->name);
        }
        sprintf(text, "%-15s : %-15s  %d seconds ago.\n", name, nl->service,
                (int) (time(0) - nl->last_recvd));
        write_user(user, text);
    }
    if (!cnt) {
        write_user(user, "No messages being received.\n\n");
    } else {
        write_user(user, "\n");
    }
}

#else

#define NO_NETLINKS

#endif
