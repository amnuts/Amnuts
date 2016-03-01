
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Send mail message
 */
void
smail(UR_OBJECT user, char *inpstr)
{
    if (inpstr) {
        static const char usage[] = "Usage: smail <user>[@<service>] [<text>]\n";
        char *remote;

        /* FIXME: Use sentinel other JAILED */
        if (user->muzzled != JAILED) {
            write_user(user, "You are muzzled, you cannot mail anyone.\n");
            return;
        }
        if (word_count < 2) {
            write_user(user, usage);
            return;
        }
        *word[1] = toupper(*word[1]);
        remote = strchr(word[1], '@');
        /* See if user exists */
        if (!remote) {
            UR_OBJECT u = get_user(word[1]);

            /* See if user has local account */
            if (!find_user_listed(word[1])) {
                if (!u) {
                    write_user(user, nosuchuser);
                } else {
                    vwrite_user(user,
                            "%s is a remote user and does not have a local account.\n",
                            u->name);
                }
                return;
            }
            if (u) {
                if (u == user && user->level < ARCH) {
                    write_user(user,
                            "Trying to mail yourself is the fifth sign of madness.\n");
                    return;
                }
                /* FIXME: Should check for offline users as well */
                if (check_igusers(u, user) && user->level < GOD) {
                    vwrite_user(user, "%s~RS is ignoring smails from you.\n", u->recap);
                    return;
                }
                strcpy(word[1], u->name);
            }
        } else if (remote == word[1]) {
            write_user(user, "Users name missing before @ sign.\n");
            return;
        }
        strcpy(user->mail_to, word[1]);
        if (word_count < 3) {
#ifdef NETLINKS
            if (user->type == REMOTE_TYPE) {
                write_user(user,
                        "Sorry, due to software limitations remote users cannot use the line editor.\nUse the \".smail <user> <text>\" method instead.\n");
                return;
            }
#endif
            vwrite_user(user, "\n~BB*** Writing mail message to %s ***\n\n",
                    user->mail_to);
            user->misc_op = 4;
            editor(user, NULL);
            return;
        }
        /* One line mail */
        strcat(inpstr, "\n");
        inpstr = remove_first(inpstr);
    } else {
        if (*user->malloc_end-- != '\n') {
            *user->malloc_end-- = '\n';
        }
        inpstr = user->malloc_start;
    }
    strtoname(user->mail_to);
    send_mail(user, user->mail_to, inpstr, 0);
    send_copies(user, inpstr);
    *user->mail_to = '\0';
}
