
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Show recorded tells and pemotes
 */
void
revtell(UR_OBJECT user)
{
#if !!0
    static const char usage[] = "Usage: revtell\n";
#endif

    start_pager(user);
    write_user(user, "\n~BB~FG*** Your tell buffer ***\n\n");
    if (!review_buffer(user, rbfTELL)) {
        write_user(user, "Revtell buffer is empty.\n");
    }
    write_user(user, "\n~BB~FG*** End ***\n\n");
    stop_pager(user);
}
