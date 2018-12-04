
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"
#include "rooms.h"

/*
 * Show talker rooms
 */
void
rooms(UR_OBJECT user, int show_topics, int wrap)
{
    RM_OBJECT rm;
    UR_OBJECT u;
#ifdef NETLINKS
    NL_OBJECT nl;
    char serv[SERV_NAME_LEN + 1], nstat[9];
    char rmaccess[9];
#endif
    int cnt, rm_cnt, rm_pub, rm_priv;

    if (word_count < 2) {
        if (!wrap) {
            user->wrap_room = room_first;
        }
        if (show_topics) {
            write_user(user,
                    "\n+----------------------------------------------------------------------------+\n");
            write_user(user,
                    "~FC~OLpl  u/m~RS  | ~OL~FCname~RS                 - ~FC~OLtopic\n");
            write_user(user,
                    "+----------------------------------------------------------------------------+\n");
        } else {
            write_user(user,
                    "\n+----------------------------------------------------------------------------+\n");
            write_user(user,
                    "~FC~OLRoom name            ~RS|~FC~OL Access  Users  Mesgs  Inlink  LStat  Service\n");
            write_user(user,
                    "+----------------------------------------------------------------------------+\n");
        }
        rm_cnt = 0;
        for (rm = user->wrap_room; rm; rm = rm->next) {
            if (is_personal_room(rm)) {
                continue;
            }
            if (rm_cnt == user->pager - 4) {
                switch (show_topics) {
                case 0:
                    user->misc_op = 10;
                    break;
                case 1:
                    user->misc_op = 11;
                    break;
                }
                write_user(user, "~BB~FG-=[*]=- PRESS <RETURN>, E TO EXIT:~RS ");
                return;
            }
            cnt = 0;
            for (u = user_first; u; u = u->next)
                if (u->type != CLONE_TYPE && u->room == rm) {
                    ++cnt;
                }
            if (show_topics) {
                vwrite_user(user, "%c%c %2d/%-2d | %s%-20.20s~RS - %s\n",
                        is_private_room(rm) ? 'P' : ' ',
                        is_fixed_room(rm) ? '*' : ' ', cnt, rm->mesg_cnt,
                        is_private_room(rm) ? "~FR~OL" : "", rm->name, rm->topic);
            }
#ifdef NETLINKS
            else {
                if (is_private_room(rm)) {
                    strcpy(rmaccess, " ~FRPRIV");
                } else {
                    strcpy(rmaccess, " ~FGPUB ");
                }
                if (is_fixed_room(rm)) {
                    *rmaccess = '*';
                }
                nl = rm->netlink;
                *serv = '\0';
                if (!nl) {
                    if (rm->inlink) {
                        strcpy(nstat, " ~FRDOWN");
                    } else {
                        strcpy(nstat, "    -");
                    }
                } else {
                    if (nl->type == UNCONNECTED) {
                        strcpy(nstat, " ~FRDOWN");
                    } else if (nl->stage == UP) {
                        strcpy(nstat, "   ~FGUP");
                    } else {
                        strcpy(nstat, "  ~FYVER");
                    }
                }
                if (nl) {
                    strcpy(serv, nl->service);
                }
                vwrite_user(user, "%-20s | %9s~RS  %5d  %5d  %-6s  %s~RS  %s\n",
                        rm->name, rmaccess, cnt, rm->mesg_cnt, noyes[rm->inlink],
                        nstat, serv);
            }
#endif
            ++rm_cnt;
            user->wrap_room = rm->next;
        }
        user->misc_op = 0;
        rm_pub = rm_priv = 0;
        for (rm = room_first; rm; rm = rm->next) {
            if (is_personal_room(rm)) {
                continue;
            }
            if (is_private_room(rm)) {
                ++rm_priv;
            } else {
                ++rm_pub;
            }
        }
        write_user(user,
                "+----------------------------------------------------------------------------+\n");
        vwrite_user(user,
                "There is a total of ~OL%d~RS rooms.  ~OL%d~RS %s public, and ~OL%d~RS %s private.\n",
                rm_priv + rm_pub, rm_pub, PLTEXT_IS(rm_pub), rm_priv,
                PLTEXT_IS(rm_priv));
        write_user(user,
                "+----------------------------------------------------------------------------+\n\n");
        return;
    }
    if (!strcasecmp(word[1], "-l")) {
        write_user(user, "The following rooms are default...\n\n");
        vwrite_user(user, "Default main room : ~OL%s\n", room_first->name);
        vwrite_user(user, "Default warp room : ~OL%s\n",
                *amsys->default_warp ? amsys->default_warp : "<none>");
        vwrite_user(user, "Default jail room : ~OL%s\n",
                *amsys->default_jail ? amsys->default_jail : "<none>");
#ifdef GAMES
        vwrite_user(user, "Default bank room : ~OL%s\n",
                *amsys->default_bank ? amsys->default_bank : "<none>");
        vwrite_user(user, "Default shoot room : ~OL%s\n",
                *amsys->default_shoot ? amsys->default_shoot : "<none>");
#endif
        if (!priv_room[0].name) {
            write_user(user,
                    "\nThere are no level specific rooms currently availiable.\n\n");
            return;
        }
        write_user(user, "\nThe following rooms are level specific...\n\n");
        for (cnt = 0; priv_room[cnt].name; ++cnt) {
            vwrite_user(user,
                    "~FC%s~RS is for users of level ~OL%s~RS and above.\n",
                    priv_room[cnt].name, user_level[priv_room[cnt].level].name);
        }
        vwrite_user(user,
                "\nThere is a total of ~OL%d~RS level specific rooms.\n\n",
                cnt);
        return;
    }
    write_user(user, "Usage: rooms [-l]\n");
}
