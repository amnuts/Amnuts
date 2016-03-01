
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Delete some or all of your mail. A problem here is once we have deleted
 * some mail from the file do we mark the file as read? If not we could
 * have a situation where the user deletes all his mail but still gets
 * the YOU HAVE UNREAD MAIL message on logging on if the idiot forgot to
 * read it first.
 */
void
dmail(UR_OBJECT user)
{
    int num, cnt;
    char filename[80];

    if (word_count < 2) {
        write_user(user, "Usage: dmail all\n");
        write_user(user, "       dmail <#>\n");
        write_user(user, "       dmail to <#>\n");
        write_user(user, "       dmail from <#> to <#>\n");
        return;
    }
    if (get_wipe_parameters(user) == -1) {
        return;
    }
    num = mail_sizes(user->name, 0);
    if (!num) {
        write_user(user, "You have no mail to delete.\n");
        return;
    }
    sprintf(filename, "%s/%s/%s.M", USERFILES, USERMAILS, user->name);
    if (user->wipe_from == -1) {
        write_user(user, "\07~OL~FR~LIDelete all of your mail?~RS (y/n): ");
        user->misc_op = 18;
        return;
    }
    if (user->wipe_from > num) {
        vwrite_user(user, "You only have %d mail message%s.\n", num,
                PLTEXT_S(num));
        return;
    }
    cnt = wipe_messages(filename, user->wipe_from, user->wipe_to, 1);
    reset_mail_counts(user);
    if (cnt == num) {
        remove(filename);
        vwrite_user(user, "There %s only %d mail message%s, all now deleted.\n",
                PLTEXT_WAS(cnt), cnt, PLTEXT_S(cnt));
        return;
    }
    vwrite_user(user, "%d mail message%s deleted.\n", cnt, PLTEXT_S(cnt));
    user->read_mail = time(0) + 1;
}
