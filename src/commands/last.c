
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Shows when a user was last logged on
 */
void
show_last_login(UR_OBJECT user)
{
    char line[ARR_SIZE], tmp[ARR_SIZE];
    UR_OBJECT u;
    int timelen, days, hours, mins, i, cnt;

    if (word_count > 2) {
        write_user(user, "Usage: last [<user>]\n");
        return;
    }

    /* if checking last on a user */
    if (word_count == 2) {
        /* get user */
        u = retrieve_user(user, word[1]);
        if (!u) {
            return;
        }
        /* error checking */
        if (u == user) {
            write_user(user, "You are already logged on!\n");
            return;
        }
        if (retrieve_user_type == 1) {
            vwrite_user(user, "%s~RS is currently logged on.\n", u->recap);
            return;
        }
        /* show details */
        timelen = (int) (time(0) - u->last_login);
        days = timelen / 86400;
        hours = (timelen % 86400) / 3600;
        mins = (timelen % 3600) / 60;
        write_user(user,
                "\n+----------------------------------------------------------------------------+\n");
        cnt = 52 + teslen(u->recap, 52);
        vwrite_user(user, "| ~FC~OLLast login details of~RS %-*.*s~RS |\n",
                cnt, cnt, u->recap);
        write_user(user,
                "+----------------------------------------------------------------------------+\n");
        strftime(tmp, ARR_SIZE, "%a %Y-%m-%d %H:%M:%S",
                localtime(&u->last_login));
        vwrite_user(user, "| Was last logged in %-55s |\n", tmp);
        sprintf(tmp, "Which was %d day%s, %d hour%s and %d minute%s ago", days,
                PLTEXT_S(days), hours, PLTEXT_S(hours), mins, PLTEXT_S(mins));
        vwrite_user(user, "| %-74s |\n", tmp);
        sprintf(tmp, "Was on for %d hour%s and %d minute%s",
                u->last_login_len / 3600, PLTEXT_S(u->last_login_len / 3600),
                (u->last_login_len % 3600) / 60,
                PLTEXT_S((u->last_login_len % 3600) / 60));
        vwrite_user(user, "| %-74s |\n", tmp);
        write_user(user,
                "+----------------------------------------------------------------------------+\n\n");
        done_retrieve(u);
        return;
    }
    /* if checking all of the last users to log on */
    /* get each line of the logins and check if that user is still on & print out the result. */
    write_user(user,
            "\n+----------------------------------------------------------------------------+\n");
    write_user(user,
            "| ~FC~OLThe last users to have logged in~RS                                           |\n");
    write_user(user,
            "+----------------------------------------------------------------------------+\n");
    for (i = 0; i < LASTLOGON_NUM; ++i) {
        if (!*last_login_info[i].name) {
            continue;
        }
        u = get_user(last_login_info[i].name);
        if (last_login_info[i].on && (u && !u->vis && user->level < WIZ)) {
            continue;
        }
        sprintf(line, "%s %s", last_login_info[i].name, last_login_info[i].time);
        if (last_login_info[i].on) {
            sprintf(text, "| %-67s ~OL~FYONLINE~RS |\n", line);
        } else {
            sprintf(text, "| %-74s |\n", line);
        }
        write_user(user, text);
    }
    write_user(user,
            "+----------------------------------------------------------------------------+\n\n");
}
