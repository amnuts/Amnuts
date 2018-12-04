
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Write a suggestion to the board, or read if if you can
 */
void
suggestions(UR_OBJECT user, char *inpstr)
{
    char filename[30], *c;
    FILE *fp;
    int cnt;

    if (com_num == RSUG) {
        sprintf(filename, "%s/%s", MISCFILES, SUGBOARD);
        write_user(user,
                "~BB~FG*** The Suggestions board has the following ideas ***\n\n");
        switch (more(user, user->socket, filename)) {
        case 0:
            write_user(user, "There are no suggestions.\n\n");
            break;
        case 1:
            user->misc_op = 2;
            break;
        }
        return;
    }
    if (inpstr) {
        /* FIXME: Use sentinel other JAILED */
        if (user->muzzled != JAILED) {
            write_user(user, "You are muzzled, you cannot make suggestions.\n");
            return;
        }
        if (word_count < 2) {
#ifdef NETLINKS
            if (user->type == REMOTE_TYPE) {
                write_user(user,
                        "Sorry, due to software limitations remote users cannot use the line editor.\nUse the \".suggest <text>\" method instead.\n");
                return;
            }
#endif
            write_user(user, "~BB~FG*** Writing a suggestion ***\n\n");
            user->misc_op = 8;
            editor(user, NULL);
            return;
        }
        strcat(inpstr, "\n"); /* XXX: risky but hopefully it will be ok */
    } else {
        inpstr = user->malloc_start;
    }
    sprintf(filename, "%s/%s", MISCFILES, SUGBOARD);
    fp = fopen(filename, "a");
    if (!fp) {
        vwrite_user(user, "%s: cannot add suggestion.\n", syserror);
        write_syslog(SYSLOG | ERRLOG, 0,
                "ERROR: Cannot open file %s to write in suggestions().\n",
                filename);
        return;
    }
    sprintf(text, "~OLFrom: %s  %s\n", user->bw_recap, long_date(0));
    fputs(text, fp);
    cnt = 0;
    for (c = inpstr; *c; ++c) {
        putc(*c, fp);
        if (*c == '\n') {
            cnt = 0;
        } else {
            ++cnt;
        }
        if (cnt == 80) {
            putc('\n', fp);
            cnt = 0;
        }
    }
    putc('\n', fp);
    fclose(fp);
    write_user(user, "Suggestion written.  Thank you for your contribution.\n");
    ++amsys->suggestion_count;
}
