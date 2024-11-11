
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Show all clones on the system
 */
void
allclones(UR_OBJECT user)
{
    UR_OBJECT u;
    int cnt;

    cnt = 0;
    for (u = user_first; u; u = u->next) {
        if (u->type != CLONE_TYPE) {
            continue;
        }
        if (!cnt++) {
            vwrite_user(user, "\n~BB*** Current clones %s ***\n\n", long_date(1));
        }
        vwrite_user(user, "%-15s : %s\n", u->name, u->room->name);
    }
    if (!cnt) {
        write_user(user, "There are no clones on the system.\n");
    } else {
        vwrite_user(user, "\nTotal of ~OL%d~RS clone%s.\n\n", cnt, PLTEXT_S(cnt));
    }
}
