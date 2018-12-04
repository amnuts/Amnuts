
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * View the system log
 */
void
viewlog(UR_OBJECT user)
{
    static const char emp[] = "This log file is empty.\n\n";
    char logfile[80];
    char dstr[32];
    FILE *fp;
    int lines, cnt, cnt2, type;
    enum lvl_value lvl;
    int c;

    if (word_count < 2) {
        write_user(user,
                "Usage: viewlog sys|net|req|err|<level> [<lines from end>]\n");
        return;
    }
    *logfile = '\0';
    strtoupper(word[1]);
    lvl = get_level(word[1]);
    strftime(dstr, 32, "%Y%m%d", localtime(&amsys->boot_time));
    if (word_count == 2) {
        if (!strcmp(word[1], "SYS")) {
            sprintf(logfile, "%s/%s.%s", LOGFILES, MAINSYSLOG, dstr);
            write_user(user, "\n~BB~FG*** System log ***\n\n");
        } else if (!strcmp(word[1], "NET")) {
            sprintf(logfile, "%s/%s.%s", LOGFILES, NETSYSLOG, dstr);
            write_user(user, "\n~BB~FG*** Netlink log ***\n\n");
        } else if (!strcmp(word[1], "REQ")) {
            sprintf(logfile, "%s/%s.%s", LOGFILES, REQSYSLOG, dstr);
            write_user(user, "\n~BB~FG*** Account Request log ***\n\n");
        } else if (!strcmp(word[1], "ERR")) {
            sprintf(logfile, "%s/%s.%s", LOGFILES, ERRSYSLOG, dstr);
            write_user(user, "\n~BB~FG*** Error log ***\n\n");
        } else if (lvl != NUM_LEVELS) {
            vwrite_user(user, "\n~BB~FG*** User list for level \"%s\" ***\n\n",
                    user_level[lvl].name);
            if (!amsys->level_count[lvl]) {
                write_user(user, emp);
                return;
            }
            user->user_page_lev = lvl;
            switch (more_users(user)) {
            case 0:
                write_user(user, emp);
                return;
            case 1:
                user->misc_op = 16;
                break;
            }
            return;
        } else {
            write_user(user,
                    "Usage: viewlog sys|net|req|err|<level> [<lines from end>]\n");
            return;
        }
        switch (more(user, user->socket, logfile)) {
        case 0:
            write_user(user, emp);
            return;
        case 1:
            user->misc_op = 2;
            break;
        }
        return;
    }
    lines = atoi(word[2]);
    if (lines < 1) {
        write_user(user,
                "Usage: viewlog sys|net|req|err|<level> [<lines from the end>]\n");
        return;
    }
    type = 0;
    /* find out which log */
    if (!strcmp(word[1], "SYS")) {
        sprintf(logfile, "%s/%s.%s", LOGFILES, MAINSYSLOG, dstr);
        type = SYSLOG;
    }
    if (!strcmp(word[1], "NET")) {
        sprintf(logfile, "%s/%s.%s", LOGFILES, NETSYSLOG, dstr);
        type = NETLOG;
    }
    if (!strcmp(word[1], "REQ")) {
        sprintf(logfile, "%s/%s.%s", LOGFILES, REQSYSLOG, dstr);
        type = REQLOG;
    }
    if (!strcmp(word[1], "ERR")) {
        sprintf(logfile, "%s/%s.%s", LOGFILES, ERRSYSLOG, dstr);
        type = ERRLOG;
    }
    if (lvl != NUM_LEVELS) {
        if (!amsys->level_count[lvl]) {
            write_user(user, emp);
            return;
        }
        if (lines > amsys->level_count[lvl]) {
            vwrite_user(user, "There %s only %d line%s in the log.\n",
                    PLTEXT_IS(amsys->level_count[lvl]), amsys->level_count[lvl],
                    PLTEXT_S(amsys->level_count[lvl]));
            return;
        }
        if (lines == amsys->level_count[lvl]) {
            vwrite_user(user, "\n~BB~FG*** User list for level \"%s\" ***\n\n",
                    user_level[lvl].name);
        } else {
            user->user_page_pos = amsys->level_count[lvl] - lines;
            vwrite_user(user,
                    "\n~BB~FG*** User list for level \"%s\" (last %d line%s) ***\n\n",
                    user_level[lvl].name, lines, PLTEXT_S(lines));
        }
        user->user_page_lev = lvl;
        switch (more_users(user)) {
        case 0:
            write_user(user, emp);
            return;
        case 1:
            user->misc_op = 16;
            break;
        }
        return;
    }
    /* count total lines */
    fp = fopen(logfile, "r");
    if (!fp) {
        write_user(user, emp);
        return;
    }
    cnt = 0;
    for (c = getc(fp); c != EOF; c = getc(fp)) {
        if (c == '\n') {
            ++cnt;
        }
    }
    if (cnt < lines) {
        vwrite_user(user, "There %s only %d line%s in the log.\n", PLTEXT_IS(cnt),
                cnt, PLTEXT_S(cnt));
        fclose(fp);
        return;
    }
    if (cnt == lines) {
        switch (type) {
        case SYSLOG:
            write_user(user, "\n~BB~FG*** System log ***\n\n");
            break;
        case NETLOG:
            write_user(user, "\n~BB~FG*** Netlink log ***\n\n");
            break;
        case REQLOG:
            write_user(user, "\n~BB~FG*** Account Request log ***\n\n");
            break;
        case ERRLOG:
            write_user(user, "\n~BB~FG*** Error log ***\n\n");
            break;
        }
        fclose(fp);
        switch (more(user, user->socket, logfile)) {
        case 0:
            break;
        case 1:
            user->misc_op = 2;
            break;
        }
#if !!0
        more(user, user->socket, logfile);
#endif
        return;
    }
    /* Find line to start on */
    fseek(fp, 0, 0);
    cnt2 = 0;
    for (c = getc(fp); c != EOF; c = getc(fp)) {
        if (c != '\n') {
            continue;
        }
        if (++cnt2 == cnt - lines) {
            break;
        }
    }
    if (c == EOF) {
        fclose(fp);
        vwrite_user(user, "%s: Line count error.\n", syserror);
        write_syslog(SYSLOG | ERRLOG, 0,
                "ERROR: Line count error in viewlog().\n");
        return;
    }
    user->filepos = ftell(fp);
    fclose(fp);
    switch (type) {
    case SYSLOG:
        vwrite_user(user, "\n~BB~FG*** System log (last %d line%s) ***\n\n",
                lines, PLTEXT_S(lines));
        break;
    case NETLOG:
        vwrite_user(user, "\n~BB~FG*** Netlink log (last %d line%s) ***\n\n",
                lines, PLTEXT_S(lines));
        break;
    case REQLOG:
        vwrite_user(user,
                "\n~BB~FG*** Account Request log (last %d line%s) ***\n\n",
                lines, PLTEXT_S(lines));
        break;
    case ERRLOG:
        vwrite_user(user, "\n~BB~FG*** Error log (last %d line%s) ***\n\n", lines,
                PLTEXT_S(lines));
        break;
    }
    if (more(user, user->socket, logfile) != 1) {
        user->filepos = 0;
    } else {
        user->misc_op = 2;
    }
}
