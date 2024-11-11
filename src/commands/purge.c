
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * allows the user to call the purge function
 */
void
purge_users(UR_OBJECT user)
{
    int exp = 0;

    if (word_count < 2) {
        write_user(user, "Usage: purge [-d] [-s <site>] [-t <days>]\n");
        return;
    }
    if (!strcmp(word[1], "-d")) {
        write_user(user,
                "~OL~FR***~RS Purging users with default settings ~OL~FR***\n");
        purge(1, NULL, 0);
    } else if (!strcmp(word[1], "-s")) {
        if (word_count < 3) {
            write_user(user, "Usage: purge [-d] [-s <site>] [-t <days>]\n");
            return;
        }
        /* check for variations of wild card */
        if (!strcmp("*", word[2])) {
            write_user(user, "You cannot purge users from the site \"*\".\n");
            return;
        }
        if (strstr(word[2], "**")) {
            write_user(user, "You cannot have \"**\" in your site to purge.\n");
            return;
        }
        if (strstr(word[2], "?*")) {
            write_user(user, "You cannot have \"?*\" in your site to purge.\n");
            return;
        }
        if (strstr(word[2], "*?")) {
            write_user(user, "You cannot have \"*?\" in your site to purge.\n");
            return;
        }
        vwrite_user(user,
                "~OL~FR***~RS Purging users with site \"%s\" ~OL~FR***\n",
                word[2]);
        purge(2, word[2], 0);
    } else if (!strcmp(word[1], "-t")) {
        if (word_count < 3) {
            write_user(user, "Usage: purge [-d] [-s <site>] [-t <days>]\n");
            return;
        }
        exp = atoi(word[2]);
        if (exp <= USER_EXPIRES) {
            write_user(user,
                    "You cannot purge users who last logged in less than the default time.\n");
            vwrite_user(user, "The current default is: %d days\n", USER_EXPIRES);
            return;
        }
        if (exp < 0 || exp > 999) {
            write_user(user, "You must enter the amount days from 0-999.\n");
            return;
        }
        vwrite_user(user,
                "~OL~FR***~RS Purging users who last logged in over %d days ago ~OL~FR***\n",
                exp);
        purge(3, NULL, exp);
    } else {
        write_user(user, "Usage: purge [-d] [-s <site>] [-t <days>]\n");
        return;
    }
    /* finished purging - give result */
    vwrite_user(user,
            "Checked ~OL%d~RS user%s (~OL%d~RS skipped), ~OL%d~RS %s purged.  User count is now ~OL%d~RS.\n",
            amsys->purge_count, PLTEXT_S(amsys->purge_count),
            amsys->purge_skip, amsys->users_purged,
            PLTEXT_WAS(amsys->users_purged), amsys->user_count);
}
