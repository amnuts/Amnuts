
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Send mail message to everyone
 */
void
level_mail(UR_OBJECT user, char *inpstr)
{
    if (inpstr) {
        static const char usage[] = "Usage: lmail <level>|wizzes|all [<text>]\n";

        /* FIXME: Use sentinel other JAILED */
        if (user->muzzled != JAILED) {
            write_user(user, "You are muzzled, you cannot mail anyone.\n");
            return;
        }
        if (word_count < 2) {
            write_user(user, usage);
            return;
        }
        strtoupper(word[1]);
        user->lmail_lev = get_level(word[1]);
        if (user->lmail_lev == NUM_LEVELS) {
            if (!strcmp(word[1], "WIZZES")) {
                user->lmail_lev = WIZ;
            } else if (!strcmp(word[1], "ALL")) {
                user->lmail_lev = JAILED;
            } else {
                write_user(user, usage);
                return;
            }
            user->lmail_all = !0;
        } else {
            user->lmail_all = 0;
        }
        if (word_count < 3) {
#ifdef NETLINKS
            if (user->type == REMOTE_TYPE) {
                write_user(user,
                        "Sorry, due to software limitations remote users cannot use the line editor.\nUse the \".lmail <level>|wizzes|all <text>\" method instead.\n");
                return;
            }
#endif
            if (!user->lmail_all) {
                vwrite_user(user,
                        "\n~FG*** Writing broadcast level mail message to all the %ss ***\n\n",
                        user_level[user->lmail_lev].name);
            } else {
                if (user->lmail_lev == WIZ) {
                    write_user(user,
                            "\n~FG*** Writing broadcast level mail message to all the Wizzes ***\n\n");
                } else {
                    write_user(user,
                            "\n~FG*** Writing broadcast level mail message to everyone ***\n\n");
                }
            }
            user->misc_op = 9;
            editor(user, NULL);
            return;
        }
        strcat(inpstr, "\n"); /* XXX: risky but hopefully it will be ok */
        inpstr = remove_first(inpstr);
    } else {
        inpstr = user->malloc_start;
    }
    if (!send_broadcast_mail(user, inpstr, user->lmail_lev, user->lmail_all)) {
        write_user(user, "There does not seem to be anyone to send mail to.\n");
        user->lmail_all = 0;
        user->lmail_lev = NUM_LEVELS;
        return;
    }
    if (!user->lmail_all) {
        vwrite_user(user, "You have sent mail to all the %ss.\n",
                user_level[user->lmail_lev].name);
        write_syslog(SYSLOG, 1, "%s sent mail to all the %ss.\n", user->name,
                user_level[user->lmail_lev].name);
    } else {
        if (user->lmail_lev == WIZ) {
            write_user(user, "You have sent mail to all the Wizzes.\n");
            write_syslog(SYSLOG, 1, "%s sent mail to all the Wizzes.\n",
                    user->name);
        } else {
            write_user(user, "You have sent mail to all the users.\n");
            write_syslog(SYSLOG, 1, "%s sent mail to all the users.\n", user->name);
        }
    }
    user->lmail_all = 0;
    user->lmail_lev = NUM_LEVELS;
}
