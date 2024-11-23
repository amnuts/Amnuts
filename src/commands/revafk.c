
#include "../includes/defines.h"
#include "../includes/globals.h"
#include "../includes/commands.h"
#include "../includes/prototypes.h"

/*
 * Show recorded tells and pemotes
 */
void
revafk(UR_OBJECT user)
{
#if !!0
    static const char usage[] = "Usage: revafk\n";
#endif

    start_pager(user);
    write_user(user, "\n~BB~FG*** Your AFK review buffer ***\n\n");
    if (!review_buffer(user, rbfAFK)) {
        write_user(user, "AFK buffer is empty.\n");
    }
    write_user(user, "\n~BB~FG*** End ***\n\n");
    stop_pager(user);
}
