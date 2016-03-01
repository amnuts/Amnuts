
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Turn on and off each individual system log, or globally on and off
 */
void
logging(UR_OBJECT user)
{
    char temp[ARR_SIZE];
    int cnt;

    if (word_count < 2) {
        write_user(user, "Usage: logging -l|-s|-r|-n|-e|-on|-off\n");
        return;
    }
    strtolower(word[1]);
    /* deal with listing the log status first */
    if (!strcmp(word[1], "-l")) {
        write_user(user,
                "\n+----------------------------------------------------------------------------+\n");
        write_user(user,
                "| ~OL~FCSystem log status~RS                                                          |\n");
        write_user(user,
                "+----------------------------------------------------------------------------+\n");
        sprintf(temp,
                "General system log : ~OL%s~RS   Account request logs : ~OL%s~RS   Netlinks log : ~OL%s~RS",
                amsys->logging & SYSLOG ? "ON " : "OFF",
                amsys->logging & REQLOG ? "ON " : "OFF",
                amsys->logging & NETLOG ? "ON " : "OFF");
        cnt = 74 + teslen(temp, 74);
        vwrite_user(user, "| %-*.*s |\n", cnt, cnt, temp);
        sprintf(temp, "         Error log : ~OL%s~RS",
                amsys->logging & ERRLOG ? "ON " : "OFF");
        cnt = 74 + teslen(temp, 74);
        vwrite_user(user, "| %-*.*s |\n", cnt, cnt, temp);
        write_user(user,
                "+----------------------------------------------------------------------------+\n\n");
        return;
    }
    /* (un)set syslog bit */
    if (!strcmp(word[1], "-s")) {
        /* if already on */
        if (amsys->logging & SYSLOG) {
            write_syslog(SYSLOG, 1, "%s switched general system logging OFF.\n",
                    user->name);
        }
        amsys->logging ^= SYSLOG;
        /* if now on */
        if (amsys->logging & SYSLOG) {
            write_syslog(SYSLOG, 1, "%s switched general system logging ON.\n",
                    user->name);
        }
        vwrite_user(user, "You have now turned the general system logging %s.\n",
                amsys->logging & SYSLOG ? "~OL~FGON~RS" : "~OL~FROFF~RS");
        return;
    }
    /* (un)set reqlog bit */
    if (!strcmp(word[1], "-r")) {
        /* if already on */
        if (amsys->logging & REQLOG) {
            write_syslog(REQLOG, 1, "%s switched account request logging OFF.\n",
                    user->name);
        }
        amsys->logging ^= REQLOG;
        /* if now on */
        if (amsys->logging & REQLOG) {
            write_syslog(REQLOG, 1, "%s switched account request logging ON.\n",
                    user->name);
        }
        vwrite_user(user, "You have now turned the account request logging %s.\n",
                amsys->logging & REQLOG ? "~OL~FGON~RS" : "~OL~FROFF~RS");
        return;
    }
    /* (un)set netlog bit */
    if (!strcmp(word[1], "-n")) {
#ifdef NETLINKS
        /* if already on */
        if (amsys->logging & NETLOG) {
            write_syslog(NETLOG, 1, "%s switched netlink logging OFF.\n",
                    user->name);
        }
        amsys->logging ^= NETLOG;
        /* if now on */
        if (amsys->logging & NETLOG) {
            write_syslog(NETLOG, 1, "%s switched netlink logging ON.\n",
                    user->name);
        }
        vwrite_user(user, "You have now turned the netlink logging %s.\n",
                amsys->logging & NETLOG ? "~OL~FGON~RS" : "~OL~FROFF~RS");
#else
        write_user(user, "Netlinks are not currently active.\n");
#endif
        return;
    }
    /* (un)set errlog bit */
    if (!strcmp(word[1], "-e")) {
        /* if already on */
        if (amsys->logging & ERRLOG) {
            write_syslog(ERRLOG, 1, "%s switched error logging OFF.\n", user->name);
        }
        amsys->logging ^= ERRLOG;
        /* if on already */
        if (amsys->logging & ERRLOG) {
            write_syslog(REQLOG, 1, "%s switched error logging ON.\n", user->name);
        }
        vwrite_user(user, "You have now turned the error logging %s.\n",
                amsys->logging & ERRLOG ? "~OL~FGON~RS" : "~OL~FROFF~RS");
        return;
    }
    /* set all bit */
    if (!strcmp(word[1], "-on")) {
        if (!(amsys->logging & SYSLOG)) {
            amsys->logging |= SYSLOG;
            write_syslog(SYSLOG, 1, "%s switched general system logging ON.\n",
                    user->name);
        }
        if (!(amsys->logging & REQLOG)) {
            amsys->logging |= REQLOG;
            write_syslog(REQLOG, 1, "%s switched acount request logging ON.\n",
                    user->name);
        }
        if (!(amsys->logging & NETLOG)) {
            amsys->logging |= NETLOG;
            write_syslog(NETLOG, 1, "%s switched netlink logging ON.\n",
                    user->name);
        }
        if (!(amsys->logging & ERRLOG)) {
            amsys->logging |= ERRLOG;
            write_syslog(ERRLOG, 1, "%s switched error logging ON.\n", user->name);
        }
        write_user(user, "You have now turned all logging ~OL~FGON~RS.\n");
        return;
    }
    /* unset all bit */
    if (!strcmp(word[1], "-off")) {
        if (amsys->logging & SYSLOG) {
            write_syslog(SYSLOG, 1, "%s switched general system logging OFF.\n",
                    user->name);
            amsys->logging &= ~SYSLOG;
        }
        if (amsys->logging & REQLOG) {
            write_syslog(REQLOG, 1, "%s switched acount request logging OFF.\n",
                    user->name);
            amsys->logging &= ~REQLOG;
        }
        if (amsys->logging & NETLOG) {
            write_syslog(NETLOG, 1, "%s switched netlink logging OFF.\n",
                    user->name);
            amsys->logging &= ~NETLOG;
        }
        if (amsys->logging & ERRLOG) {
            write_syslog(ERRLOG, 1, "%s switched error logging OFF.\n", user->name);
            amsys->logging &= ~ERRLOG;
        }
        write_user(user, "You have now turned all logging ~OL~FROFF~RS.\n");
        return;
    }
    write_user(user, "Usage: logging -l|-s|-r|-n|-e|-on|-off\n");
}