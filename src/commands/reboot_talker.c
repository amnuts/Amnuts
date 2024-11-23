
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Reboot talker interface func
 */
void
reboot_com(UR_OBJECT user)
{
    if (!amsys->rs_which) {
        write_user(user,
                "The shutdown countdown is currently active, you must cancel it first.\n");
        return;
    }
    if (!strcmp(word[1], "cancel")) {
        if (!amsys->rs_countdown) {
            write_user(user, "The reboot countdown is not currently active.\n");
            return;
        }
        if (amsys->rs_countdown && !amsys->rs_user) {
            write_user(user,
                    "Someone else is currently setting the reboot countdown.\n");
            return;
        }
        write_room(NULL, "~OLSYSTEM:~RS~FG Reboot cancelled.\n");
        write_syslog(SYSLOG, 1, "%s cancelled the reboot countdown.\n",
                user->name);
        amsys->rs_countdown = 0;
        amsys->rs_announce = 0;
        amsys->rs_which = -1;
        amsys->rs_user = NULL;
        return;
    }
    if (word_count > 1 && !is_number(word[1])) {
        write_user(user, "Usage: reboot [<secs>|cancel]\n");
        return;
    }
    if (amsys->rs_countdown) {
        write_user(user,
                "The reboot countdown is currently active, you must cancel it first.\n");
        return;
    }
    if (word_count < 2) {
        amsys->rs_countdown = 0;
        amsys->rs_announce = 0;
        amsys->rs_which = 1;
        amsys->rs_user = NULL;
    } else {
        amsys->rs_countdown = atoi(word[1]);
        amsys->rs_which = 1;
    }
    write_user(user,
            "\n\07~FY~OL~LI*** WARNING - This will reboot the talker! ***\n\nAre you sure about this (y|n)? ");
    user->misc_op = 7;
    no_prompt = 1;
}