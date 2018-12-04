
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Wipe some messages off the board
 */
void
wipe_board(UR_OBJECT user)
{
    char filename[80];
    const char *name;
    RM_OBJECT rm;
    int cnt;

    rm = user->room;
    if (word_count < 2 && ((user->level >= WIZ && !is_personal_room(rm))
            || (is_personal_room(rm)
            && (is_my_room(user, rm)
            || user->level >= GOD)))) {
        write_user(user, "Usage: wipe all\n");
        write_user(user, "       wipe <#>\n");
        write_user(user, "       wipe to <#>\n");
        write_user(user, "       wipe from <#> to <#>\n");
        return;
    } else if (word_count < 2 && ((user->level < WIZ && !is_personal_room(rm))
            || (is_personal_room(rm)
            && !is_my_room(user, rm)
            && user->level < GOD))) {
        write_user(user, "Usage: wipe <#>\n");
        return;
    }
    if (is_personal_room(rm)) {
        if (!is_my_room(user, rm) && user->level < GOD && !check_board_wipe(user)) {
            return;
        } else if (get_wipe_parameters(user) == -1) {
            return;
        }
    } else {
        if (user->level < WIZ && !(check_board_wipe(user))) {
            return;
        } else if (get_wipe_parameters(user) == -1) {
            return;
        }
    }
    name = user->vis ? user->recap : invisname;
    if (is_personal_room(rm)) {
        sprintf(filename, "%s/%s/%s.B", USERFILES, USERROOMS, rm->owner);
    } else {
        sprintf(filename, "%s/%s.B", DATAFILES, rm->name);
    }
    if (!rm->mesg_cnt) {
        write_user(user, "There are no messages on the room board.\n");
        return;
    }
    if (user->wipe_from == -1) {
        remove(filename);
        write_user(user, "All messages deleted.\n");
        if (user->level < GOD || user->vis) {
            vwrite_room_except(rm, user, "%s~RS wipes the message board.\n", name);
        }
        write_syslog(SYSLOG, 1,
                "%s wiped all messages from the board in the %s.\n",
                user->name, rm->name);
        rm->mesg_cnt = 0;
        return;
    }
    if (user->wipe_from > rm->mesg_cnt) {
        vwrite_user(user, "There %s only %d message%s on the board.\n",
                PLTEXT_IS(rm->mesg_cnt), rm->mesg_cnt,
                PLTEXT_S(rm->mesg_cnt));
        return;
    }
    cnt = wipe_messages(filename, user->wipe_from, user->wipe_to, 0);
    if (cnt == rm->mesg_cnt) {
        remove(filename);
        vwrite_user(user,
                "There %s only %d message%s on the board, all now deleted.\n",
                PLTEXT_WAS(rm->mesg_cnt), rm->mesg_cnt,
                PLTEXT_S(rm->mesg_cnt));
        if (user->level < GOD || user->vis) {
            vwrite_room_except(rm, user, "%s wipes the message board.\n", name);
        }
        write_syslog(SYSLOG, 1,
                "%s wiped all messages from the board in the %s.\n",
                user->name, rm->name);
        rm->mesg_cnt = 0;
        return;
    }
    rm->mesg_cnt -= cnt;
    vwrite_user(user, "%d board message%s deleted.\n", cnt, PLTEXT_S(cnt));
    if (user->level < GOD || user->vis) {
        vwrite_room_except(rm, user, "%s wipes some messages from the board.\n",
                name);
    }
    write_syslog(SYSLOG, 1, "%s wiped %d message%s from the board in the %s.\n",
            user->name, cnt, PLTEXT_S(cnt), rm->name);
}

