
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * List banned sites or users
 */
void
listbans(UR_OBJECT user)
{
    char filename[80];
    int i;

    strtolower(word[1]);
    if (!strcmp(word[1], "sites")) {
        write_user(user, "\n~BB*** Banned sites and domains ***\n\n");
        sprintf(filename, "%s/%s", DATAFILES, SITEBAN);
        switch (more(user, user->socket, filename)) {
        case 0:
            write_user(user, "There are no banned sites and domains.\n\n");
            return;
        case 1:
            user->misc_op = 2;
            break;
        }
        return;
    }
    if (!strcmp(word[1], "users")) {
        write_user(user, "\n~BB*** Banned users ***\n\n");
        sprintf(filename, "%s/%s", DATAFILES, USERBAN);
        switch (more(user, user->socket, filename)) {
        case 0:
            write_user(user, "There are no banned users.\n\n");
            return;
        case 1:
            user->misc_op = 2;
            break;
        }
        return;
    }
    if (!strcmp(word[1], "swears")) {
        write_user(user, "\n~BB*** Banned swear words ***\n\n");
        for (i = 0; swear_words[i]; ++i) {
            write_user(user, swear_words[i]);
            write_user(user, "\n");
        }
        if (!i) {
            write_user(user, "There are no banned swear words.\n");
        }
        if (amsys->ban_swearing) {
            write_user(user, "\n");
        } else {
            write_user(user, "\n(Swearing ban is currently off)\n\n");
        }
        return;
    }
    if (strcmp(word[1], "new")) {
        write_user(user,
                "\n~BB*** New users banned from sites and domains **\n\n");
        sprintf(filename, "%s/%s", DATAFILES, NEWBAN);
        switch (more(user, user->socket, filename)) {
        case 0:
            write_user(user,
                    "There are no sites and domains where new users have been banned.\n\n");
            return;
        case 1:
            user->misc_op = 2;
            break;
        }
        return;
    }
    write_user(user, "Usage: lban sites|users|new|swears\n");
}