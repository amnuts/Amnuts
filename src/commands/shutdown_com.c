
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Shutdown talker interface func. Countdown time is entered in seconds so
 * we can specify less than a minute till reboot.
 */
void
shutdown_com(UR_OBJECT user)
{
    if (amsys->rs_which == 1) {
        write_user(user,
                "The reboot countdown is currently active, you must cancel it first.\n");
        return;
    }
    if (!strcmp(word[1], "cancel")) {
        if (!amsys->rs_countdown || amsys->rs_which) {
            write_user(user, "The shutdown countdown is not currently active.\n");
            return;
        }
        if (amsys->rs_countdown && !amsys->rs_which && !amsys->rs_user) {
            write_user(user,
                    "Someone else is currently setting the shutdown countdown.\n");
            return;
        }
        write_room(NULL, "~OLSYSTEM:~RS~FG Shutdown cancelled.\n");
        write_syslog(SYSLOG, 1, "%s cancelled the shutdown countdown.\n",
                user->name);
        amsys->rs_countdown = 0;
        amsys->rs_announce = 0;
        amsys->rs_which = -1;
        amsys->rs_user = NULL;
        return;
    }
    if (word_count > 1 && !is_number(word[1])) {
        write_user(user, "Usage: shutdown [<secs>|cancel]\n");
        return;
    }
    if (amsys->rs_countdown && !amsys->rs_which) {
        write_user(user,
                "The shutdown countdown is currently active, you must cancel it first.\n");
        return;
    }
    if (word_count < 2) {
        amsys->rs_countdown = 0;
        amsys->rs_announce = 0;
        amsys->rs_which = -1;
        amsys->rs_user = NULL;
    } else {
        amsys->rs_countdown = atoi(word[1]);
        amsys->rs_which = 0;
    }
    write_user(user,
            "\n\07~FR~OL~LI*** WARNING - This will shutdown the talker! ***\n\nAre you sure about this (y|n)? ");
    user->misc_op = 1;
    no_prompt = 1;
}
