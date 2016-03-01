
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Show users own clones
 */
void
myclones(UR_OBJECT user)
{
    UR_OBJECT u;
    int cnt;

    cnt = 0;
    for (u = user_first; u; u = u->next) {
        if (u->type != CLONE_TYPE || u->owner != user) {
            continue;
        }
        if (!cnt++) {
            write_user(user, "\n~BB*** Rooms you have clones in ***\n\n");
        }
        vwrite_user(user, "  %s\n", u->room->name);
    }
    if (!cnt) {
        write_user(user, "You have no clones.\n");
    } else {
        vwrite_user(user, "\nTotal of ~OL%d~RS clone%s.\n\n", cnt, PLTEXT_S(cnt));
    }
}
