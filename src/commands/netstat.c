
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

#ifdef NETLINKS

#include "netlinks.h"

/*
 * List defined netlinks and their status
 */
void
netstat(UR_OBJECT user)
{
    static const char *const allow[] = {"  ?", "ALL", " IN", "OUT"};
    static const char *const type[] = {"  -", " IN", "OUT"};
    char nlstat[9], vers[8];
    NL_OBJECT nl;
    UR_OBJECT u;
    int iu, ou, a;

    if (!nl_first) {
        write_user(user, "No remote connections configured.\n");
        return;
    }
    write_user(user,
            "\n~BB*** Netlink data & status ***\n\n~FCService name    : Allow Type Status IU OU Version  Site\n\n");
    for (nl = nl_first; nl; nl = nl->next) {
        iu = ou = 0;
        if (nl->stage == UP) {
            for (u = user_first; u; u = u->next) {
                if (u->netlink == nl) {
                    if (u->type == REMOTE_TYPE) {
                        ++iu;
                    }
                    if (!u->room) {
                        ++ou;
                    }
                }
            }
        }
        if (nl->type == UNCONNECTED) {
            strcpy(nlstat, "~FRDOWN");
            strcpy(vers, "-");
        } else {
            if (nl->stage == UP) {
                strcpy(nlstat, "  ~FGUP");
            } else {
                strcpy(nlstat, " ~FYVER");
            }
            if (!nl->ver_major) {
                /* Pre - 3.2 version */
                strcpy(vers, "3.?.?");
            } else {
                sprintf(vers, "%d.%d.%d", nl->ver_major, nl->ver_minor,
                        nl->ver_patch);
            }
        }
        /*
           If link is incoming and remoter vers < 3.2 we have no way of knowing
           what the permissions on it are so set to blank
         */
        if (!nl->ver_major && nl->type == INCOMING && nl->allow != IN) {
            a = 0;
        } else {
            a = nl->allow + 1;
        }
        sprintf(text, "%-15s :   %s  %s   %s~RS %2d %2d %7s  %s %s\n",
                nl->service, allow[a], type[nl->type], nlstat, iu, ou, vers,
                nl->site, nl->port);
        write_user(user, text);
    }
    write_user(user, "\n");
}

#else

#define NO_NETLINKS

#endif
