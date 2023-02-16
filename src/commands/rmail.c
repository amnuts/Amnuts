
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Read your mail
 */
void
rmail(UR_OBJECT user)
{
    char filename[80];
    int ret;

    sprintf(filename, "%s/%s/%s.M", USERFILES, USERMAILS, user->name);

    /* Just reading the one message or all new mail */
    if (word_count > 1) {
        strtolower(word[1]);
        if (!strcmp(word[1], "new")) {
            read_new_mail(user);
        } else {
            read_specific_mail(user);
        }
        return;
    }
    /* Update last read / new mail received time at head of file */
    if (!reset_mail_counts(user)) {
        write_user(user, "You do not have any mail.\n");
        return;
    }
    /* Reading the whole mail box */
    write_user(user, "\n~BB*** Your mailbox has the following messages ***\n\n");
    user->filepos = 0;
    ret = more(user, user->socket, filename);
    if (ret == 1) {
        user->misc_op = 2;
    }
}
