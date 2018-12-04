
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Write on the message board
 */
void
write_board(UR_OBJECT user, char *inpstr)
{
    char filename[80]; /* TODO: the max filename size should be calculated */
    const char *name;
    char *c;
    FILE *fp;
    int cnt;

    if (inpstr) {
        /* FIXME: Use sentinel other JAILED */
        if (user->muzzled != JAILED) {
            write_user(user, "You are muzzled, you cannot write on the board.\n");
            return;
        }
        if (word_count < 2) {
#ifdef NETLINKS
            if (user->type == REMOTE_TYPE) {
                /* Editor will not work over netlink because all the prompts will go wrong */
                write_user(user,
                        "Sorry, due to software limitations remote users cannot use the line editor.\nUse the \".write <text>\" method instead.\n");
                return;
            }
#endif
            write_user(user, "\n~BB*** Writing board message ***\n\n");
            user->misc_op = 3;
            editor(user, NULL);
            return;
        }
        strcat(inpstr, "\n"); /* XXX: risky but hopefully it will be ok */
    } else {
        inpstr = user->malloc_start;
    }
    switch (amsys->ban_swearing) {
    case SBMAX:
        if (contains_swearing(inpstr)) {
            write_user(user, noswearing);
            return;
        }
        break;
    case SBMIN:
        if (!is_personal_room(user->room)) {
            inpstr = censor_swear_words(inpstr);
        }
        break;
    case SBOFF:
    default:
        /* do nothing as ban_swearing is off */
        break;
    }
    if (is_personal_room(user->room)) {
        sprintf(filename, "%s/%s/%s.B", USERFILES, USERROOMS, user->room->owner);
    } else {
        sprintf(filename, "%s/%s.B", DATAFILES, user->room->name);
    }
    fp = fopen(filename, "a");
    if (!fp) {
        vwrite_user(user, "%s: cannot write to file.\n", syserror);
        write_syslog(SYSLOG | ERRLOG, 0,
                "ERROR: Cannot open file %s to append in write_board().\n",
                filename);
        return;
    }
    name = user->vis ? user->recap : invisname;
    /*
       The posting time (PT) is the time its written in machine readable form, this
       makes it easy for this program to check the age of each message and delete
       as appropriate in check_messages()
     */
#ifdef NETLINKS
    if (user->type == REMOTE_TYPE) {
        sprintf(text, "PT: %d\r~OLFrom: %s@%s  %s\n", (int) (time(0)), name,
                user->netlink->service, long_date(0));
    } else
#endif
    {
        sprintf(text, "PT: %d\r~OLFrom: %s  %s\n", (int) (time(0)), name,
                long_date(0));
    }
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
    write_user(user, "You write the message on the board.\n");
    vwrite_room_except(user->room, user, "%s writes a message on the board.\n",
            name);
    ++user->room->mesg_cnt;
}