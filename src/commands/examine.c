
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Examine a user
 */
void
examine(UR_OBJECT user)
{
    char filename[80], text2[ARR_SIZE];
    FILE *fp;
    UR_OBJECT u;
    int on, days, hours, mins, timelen, days2, hours2, mins2, idle, cnt,
            newmail;
    int lastdays, lasthours, lastmins;

    if (word_count < 2) {
        u = user;
        on = 1;
    } else {
        u = retrieve_user(user, word[1]);
        if (!u) {
            return;
        }
        on = retrieve_user_type == 1;
    }

    days = u->total_login / 86400;
    hours = (u->total_login % 86400) / 3600;
    mins = (u->total_login % 3600) / 60;
    timelen = (int) (time(0) - u->last_login);
    days2 = timelen / 86400;
    hours2 = (timelen % 86400) / 3600;
    mins2 = (timelen % 3600) / 60;

    write_user(user,
            "+----------------------------------------------------------------------------+\n");
    sprintf(text2, "%s~RS %s", u->recap, u->desc);
    cnt = 45 + teslen(text2, 45);
    vwrite_user(user, "Name   : %-*.*s~RS Level : %s\n", cnt, cnt, text2,
            user_level[u->level].name);
    if (!on) {
        lastdays = u->last_login_len / 86400;
        lasthours = (u->last_login_len % 86400) / 3600;
        lastmins = (u->last_login_len % 3600) / 60;

        strftime(text2, ARR_SIZE, "%a %Y-%m-%d %H:%M:%S",
                localtime(&u->last_login));
        vwrite_user(user, "Last login : %s\n", text2);
        vwrite_user(user, "Which was  : %d day%s, %d hour%s, %d minute%s ago\n",
                days2, PLTEXT_S(days2), hours2, PLTEXT_S(hours2), mins2,
                PLTEXT_S(mins2));
        vwrite_user(user,
                "Was on for : %d day%s, %d hour%s, %d minute%s\nTotal login: %d day%s, %d hour%s, %d minute%s\n",
                lastdays, PLTEXT_S(lastdays), lasthours, PLTEXT_S(lasthours),
                lastmins, PLTEXT_S(lastmins), days, PLTEXT_S(days), hours,
                PLTEXT_S(hours), mins, PLTEXT_S(mins));
        if (user->level >= WIZ) {
            vwrite_user(user, "Last site  : %s\n", u->last_site);
        }
        newmail = mail_sizes(u->name, 1);
        if (newmail) {
            vwrite_user(user, "%s~RS has unread mail (%d).\n", u->recap, newmail);
        }
        write_user(user,
                "+----- ~OL~FCProfile~RS --------------------------------------------------------------+\n\n");
        sprintf(filename, "%s/%s/%s.P", USERFILES, USERPROFILES, u->name);
        fp = fopen(filename, "r");
        if (!fp) {
            write_user(user, "User has not yet witten a profile.\n\n");
        } else {
            fclose(fp);
            more(user, user->socket, filename);
        }
        write_user(user,
                "+----------------------------------------------------------------------------+\n\n");
        done_retrieve(u);
        return;
    }
    idle = (int) (time(0) - u->last_input) / 60;
    if (u->malloc_start) {
        vwrite_user(user, "Ignoring all: ~FCUsing Line Editor\n");
    } else {
        vwrite_user(user, "Ignoring all: %s\n", noyes[u->ignall]);
    }
    strftime(text2, ARR_SIZE, "%a %Y-%m-%d %H:%M:%S",
            localtime(&u->last_login));
    vwrite_user(user,
            "On since    : %s\nOn for      : %d day%s, %d hour%s, %d minute%s\n",
            text2, days2, PLTEXT_S(days2), hours2, PLTEXT_S(hours2), mins2,
            PLTEXT_S(mins2));
    if (u->afk) {
        vwrite_user(user, "Idle for    : %d minute%s ~BR(AFK)\n", idle,
                PLTEXT_S(idle));
        if (*u->afk_mesg) {
            vwrite_user(user, "AFK message : %s\n", u->afk_mesg);
        }
    } else {
        vwrite_user(user, "Idle for    : %d minute%s\n", idle, PLTEXT_S(idle));
    }
    vwrite_user(user, "Total login : %d day%s, %d hour%s, %d minute%s\n", days,
            PLTEXT_S(days), hours, PLTEXT_S(hours), mins, PLTEXT_S(mins));
    if (u->socket >= 0) {
        if (user->level >= WIZ) {
            vwrite_user(user, "Site        : %-40.40s  Port : %s\n", u->site,
                    u->site_port);
        }
#ifdef NETLINKS
        if (!u->room) {
            vwrite_user(user, "Home service: %s\n", u->netlink->service);
        }
#endif
    }
    newmail = mail_sizes(u->name, 1);
    if (newmail) {
        vwrite_user(user, "%s~RS has unread mail (%d).\n", u->recap, newmail);
    }
    write_user(user,
            "+----- ~OL~FCProfile~RS --------------------------------------------------------------+\n\n");
    sprintf(filename, "%s/%s/%s.P", USERFILES, USERPROFILES, u->name);
    fp = fopen(filename, "r");
    if (!fp) {
        write_user(user, "User has not yet written a profile.\n\n");
    } else {
        fclose(fp);
        more(user, user->socket, filename);
    }
    write_user(user,
            "+----------------------------------------------------------------------------+\n\n");
}
