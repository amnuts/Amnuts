
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * delete suggestions from the board
 */
void
delete_suggestions(UR_OBJECT user)
{
    char filename[80];
    int cnt;

    if (word_count < 2) {
        write_user(user, "Usage: dsug all\n");
        write_user(user, "Usage: dsug <#>\n");
        write_user(user, "Usage: dsug to <#>\n");
        write_user(user, "Usage: dsug from <#> to <#>\n");
        return;
    }
    if (get_wipe_parameters(user) == -1) {
        return;
    }
    if (!amsys->suggestion_count) {
        write_user(user, "There are no suggestions to delete.\n");
        return;
    }
    sprintf(filename, "%s/%s", MISCFILES, SUGBOARD);
    if (user->wipe_from == -1) {
        remove(filename);
        write_user(user, "All suggestions deleted.\n");
        write_syslog(SYSLOG, 1, "%s wiped all suggestions from the %s board\n",
                user->name, SUGBOARD);
        amsys->suggestion_count = 0;
        return;
    }
    if (user->wipe_from > amsys->suggestion_count) {
        vwrite_user(user, "There %s only %d suggestion%s on the board.\n",
                PLTEXT_IS(amsys->suggestion_count), amsys->suggestion_count,
                PLTEXT_S(amsys->suggestion_count));
        return;
    }
    cnt = wipe_messages(filename, user->wipe_from, user->wipe_to, 0);
    if (cnt == amsys->suggestion_count) {
        remove(filename);
        vwrite_user(user,
                "There %s only %d suggestion%s on the board, all now deleted.\n",
                PLTEXT_WAS(cnt), cnt, PLTEXT_S(cnt));
        write_syslog(SYSLOG, 1, "%s wiped all suggestions from the %s board\n",
                user->name, SUGBOARD);
        amsys->suggestion_count = 0;
        return;
    }
    amsys->suggestion_count -= cnt;
    vwrite_user(user, "%d suggestion%s deleted.\n", cnt, PLTEXT_S(cnt));
    write_syslog(SYSLOG, 1, "%s wiped %d suggestion%s from the %s board\n",
            user->name, cnt, PLTEXT_S(cnt), SUGBOARD);
}
