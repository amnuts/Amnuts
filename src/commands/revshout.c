
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * See review of shouts
 */
void
revshout(UR_OBJECT user)
{
#if !!0
    static const char usage[] = "Usage: revshout\n";
#endif
    int i, line, cnt;

    cnt = 0;
    start_pager(user);
    if (user->reverse_buffer) {
        for (i = REVIEW_LINES - 1; i >= 0; --i) {
            line = (amsys->sbuffline + i) % REVIEW_LINES;
            if (*amsys->shoutbuff[line]) {
                if (!cnt++) {
                    write_user(user, "~BB~FG*** Shout review buffer ***\n\n");
                }
                write_user(user, amsys->shoutbuff[line]);
            }
        }
    } else {
        for (i = 0; i < REVIEW_LINES; ++i) {
            line = (amsys->sbuffline + i) % REVIEW_LINES;
            if (*amsys->shoutbuff[line]) {
                if (!cnt++) {
                    write_user(user, "~BB~FG*** Shout review buffer ***\n\n");
                }
                write_user(user, amsys->shoutbuff[line]);
            }
        }
    }
    if (!cnt) {
        write_user(user, "Shout review buffer is empty.\n");
    } else {
        write_user(user, "\n~BB~FG*** End ***\n\n");
    }
    stop_pager(user);
}
