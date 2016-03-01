
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Show who is on.  type 0=who, 1=fwho, 2=people
 */
void
who(UR_OBJECT user, int type)
{
    char line[RECAP_NAME_LEN + USER_DESC_LEN * 3];
    char rname[ROOM_NAME_LEN + PERSONAL_ROOMNAME_LEN + 1];
    char idlestr[20];
    char sockstr[3];
    const char *portstr;
    UR_OBJECT u;
    int mins;
    int idle;
    int linecnt;
    int rnamecnt;
    int total;
    int invis;
    int logins;
#ifdef NETLINKS
    int remote = 0;
#endif

    total = invis = logins = 0;

    if (type == 2 && !strcmp(word[1], "key")) {
        write_user(user,
                "\n+----------------------------------------------------------------------------+\n");
        write_user(user,
                "| ~OL~FCUser login stages are as follows~RS                                           |\n");
        write_user(user,
                "+----------------------------------------------------------------------------+\n");
        vwrite_user(user,
                "| ~FY~OLStage %d :~RS The user has logged onto the port and is entering their name     |\n",
                LOGIN_NAME);
        vwrite_user(user,
                "| ~FY~OLStage %d :~RS The user is entering their password for the first time           |\n",
                LOGIN_PASSWD);
        vwrite_user(user,
                "| ~FY~OLStage %d :~RS The user is new and has been asked to confirm their password     |\n",
                LOGIN_CONFIRM);
        vwrite_user(user,
                "| ~FY~OLStage %d :~RS The user has entered the pre-login information prompt            |\n",
                LOGIN_PROMPT);
        write_user(user,
                "+----------------------------------------------------------------------------+\n\n");
        return;
    }
    write_user(user,
            "\n+----------------------------------------------------------------------------+\n");
    write_user(user,
            align_string(1, 78, 0, NULL, "%sCurrent users %s",
            (user->login) ? "" : "~FG", long_date(1)));
    switch (type) {
    case 0:
        vwrite_user(user,
                "%s  Name                                           : Room            : Tm/Id\n",
                (user->login) ? "" : "~FC");
        break;
    case 1:
        write_user(user,
                "~FCFriend                                           : Room            : Tm/Id\n");
        break;
    case 2:
        write_user(user,
                "~FCName            : Level Line Ignall Visi Idle Mins  Port  Site/Service\n");
        break;
    }
    write_user(user,
            "+----------------------------------------------------------------------------+\n\n");
    for (u = user_first; u; u = u->next) {
        if (u->type == CLONE_TYPE) {
            continue;
        }
        mins = (int) (time(0) - u->last_login) / 60;
        idle = (int) (time(0) - u->last_input) / 60;
#ifdef NETLINKS
        if (u->type == REMOTE_TYPE) {
            portstr = "   @";
        } else
#endif
        {
#ifdef WIZPORT
            if (u->wizport) {
                portstr = " WIZ";
            } else
#endif
            {
                portstr = "MAIN";
            }
        }
        if (u->login) {
            if (type != 2) {
                continue;
            }
            vwrite_user(user,
                    "~FY[Login stage %d]~RS :  -      %2d   -    -  %4d    -  %s  %s:%s\n",
                    u->login, u->socket, idle, portstr, u->site, u->site_port);
            ++logins;
            continue;
        }
        if (type == 1 && !user_is_friend(user, u)) {
            continue;
        }
        ++total;
#ifdef NETLINKS
        if (u->type == REMOTE_TYPE) {
            ++remote;
        }
#endif
        if (!u->vis) {
            ++invis;
            if (u->level > user->level && !(user->level >= ARCH)) {
                continue;
            }
        }
        if (type == 2) {
            if (u->afk) {
                strcpy(idlestr, " ~FRAFK~RS");
            } else if (u->malloc_start) {
                strcpy(idlestr, "~FCEDIT~RS");
            } else {
                sprintf(idlestr, "%4d", idle);
            }
#ifdef NETLINKS
            if (u->type == REMOTE_TYPE) {
                strcpy(sockstr, " @");
            } else
#endif
                sprintf(sockstr, "%2d", u->socket);
            linecnt = 15 + teslen(u->recap, 15);
            vwrite_user(user,
                    "%-*.*s~RS : %-5.5s   %s %-6s %-4s %s %4d  %-4s  %s\n",
                    linecnt, linecnt, u->recap, user_level[u->level].name,
                    sockstr, noyes[u->ignall], noyes[u->vis], idlestr, mins,
                    portstr, u->site);
            continue;
        }
        sprintf(line, "  %s~RS %s", u->recap, u->desc);
        if (!u->vis) {
            *line = '*';
        }
#ifdef NETLINKS
        if (u->type == REMOTE_TYPE) {
            line[1] = '@';
        }
        if (!u->room) {
            sprintf(rname, "@%s", u->netlink->service);
        } else {
#endif
            strcpy(rname,
                    is_personal_room(u->room) ? u->room->show_name : u->room->name);
#ifdef NETLINKS
        }
#endif

        /* Count number of colour coms to be taken account of when formatting */
        if (u->afk) {
            strcpy(idlestr, "~FRAFK~RS");
        } else if (u->malloc_start) {
            strcpy(idlestr, "~FCEDIT~RS");
        } else if (idle >= 30) {
            strcpy(idlestr, "~FYIDLE~RS");
        } else {
            sprintf(idlestr, "%d/%d", mins, idle);
        }
        linecnt = 45 + teslen(line, 45);
        rnamecnt = 15 + teslen(rname, 15);
        vwrite_user(user, "%-*.*s~RS  %1.1s : %-*.*s~RS : %s\n", linecnt, linecnt,
                line, user_level[u->level].alias, rnamecnt, rnamecnt, rname,
                idlestr);
    }
    write_user(user,
            "\n+----------------------------------------------------------------------------+\n");
    switch (type) {
    case 0:
    case 2:
        if (type == 2) {
            sprintf(text, "and ~OL~FG%d~RS login%s ", logins, PLTEXT_S(logins));
        }
#ifdef NETLINKS
        write_user(user,
                align_string(1, 78, 0, NULL,
                "Total of ~OL~FG%d~RS user%s %s: ~OL~FC%d~RS visible, ~OL~FC%d~RS invisible, ~OL~FC%d~RS remote",
                total, PLTEXT_S(total), type == 2 ? text : "",
                amsys->num_of_users - invis, invis, remote));
#else
        write_user(user,
                align_string(1, 78, 0, NULL,
                "Total of ~OL~FG%d~RS user%s %s: ~OL~FC%d~RS visible, ~OL~FC%d~RS invisible",
                total, PLTEXT_S(total), type == 2 ? text : "",
                amsys->num_of_users - invis, invis));
#endif
        break;
    case 1:
        write_user(user,
                align_string(1, 78, 0, NULL,
                "Total of ~OL~FG%d~RS friend%s : ~OL~FC%d~RS visible, ~OL~FC%d~RS invisible",
                total, PLTEXT_S(total), total - invis, invis));
        break;
    }
    write_user(user,
            "+----------------------------------------------------------------------------+\n");
}

