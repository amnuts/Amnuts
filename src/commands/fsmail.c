#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Send mail message to all people on your friends list
 */
void
friend_smail(UR_OBJECT user, char *inpstr)
{
    FU_OBJECT fu;

    if (inpstr) {
        /* FIXME: Use sentinel other JAILED */
        if (user->muzzled != JAILED) {
            write_user(user, "You are muzzled, you cannot mail anyone.\n");
            return;
        }
        /* check to see if use has friends listed */
        if (!count_friends(user)) {
            write_user(user, "You have no friends listed.\n");
            return;
        }
        if (word_count < 2) {
            /* go to the editor to smail */
#ifdef NETLINKS
            if (user->type == REMOTE_TYPE) {
                write_user(user,
                        "Sorry, due to software limitations remote users cannot use the line editor.\nUse the \".fsmail <text>\" method instead.\n");
                return;
            }
#endif
            write_user(user,
                    "\n~BB*** Writing mail message to all your friends ***\n\n");
            user->misc_op = 24;
            editor(user, NULL);
        }
        /* do smail - no editor */
        strcat(inpstr, "\n");
    } else {
        /* now do smail - out of editor */
        if (*user->malloc_end-- != '\n') {
            *user->malloc_end-- = '\n';
        }
        inpstr = user->malloc_start;
    }
    for (fu = user->fu_first; fu; fu = fu->next) {
        if (fu->flags & fufFRIEND) {
            send_mail(user, fu->name, inpstr, 2);
        }
    }
    write_user(user, "Mail sent to all people on your friends list.\n");
    write_syslog(SYSLOG, 1,
            "%s sent mail to all people on their friends list.\n",
            user->name);
}