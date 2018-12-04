
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * See review of conversation
 */
void
review(UR_OBJECT user)
{
#if !!0
    static const char usage[] = "Usage: review\n";
#endif
    RM_OBJECT rm;
    int i, line, cnt;

    if (word_count < 2 || user->level < GOD) {
        rm = user->room;
    } else {
        rm = get_room(word[1]);
        if (!rm) {
            write_user(user, nosuchroom);
            return;
        }
        if (!has_room_access(user, rm)) {
            write_user(user,
                    "That room is currently private, you cannot review the conversation.\n");
            return;
        }
        vwrite_user(user, "~FC(Review of %s room)\n", rm->name);
    }
    cnt = 0;
    start_pager(user);
    if (user->reverse_buffer) {
        for (i = REVIEW_LINES - 1; i >= 0; --i) {
            line = (rm->revline + i) % REVIEW_LINES;
            if (*rm->revbuff[line]) {
                if (!cnt++) {
                    write_user(user, "\n~BB~FG*** Room conversation buffer ***\n\n");
                }
                write_user(user, rm->revbuff[line]);
            }
        }
    } else {
        for (i = 0; i < REVIEW_LINES; ++i) {
            line = (rm->revline + i) % REVIEW_LINES;
            if (*rm->revbuff[line]) {
                if (!cnt++) {
                    write_user(user, "\n~BB~FG*** Room conversation buffer ***\n\n");
                }
                write_user(user, rm->revbuff[line]);
            }
        }
    }
    if (!cnt) {
        write_user(user, "Review buffer is empty.\n");
    } else {
        write_user(user, "\n~BB~FG*** End ***\n\n");
    }
    stop_pager(user);
}
