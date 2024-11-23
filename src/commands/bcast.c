
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Broadcast an important message
 */
void
bcast(UR_OBJECT user, char *inpstr, int beeps)
{
    static const char usage[] = "Usage: bcast <text>\n";
    static const char busage[] = "Usage: bbcast <text>\n";

    /* FIXME: Use sentinel other JAILED */
    if (user->muzzled != JAILED) {
        write_user(user, "You are muzzled, you cannot broadcast.\n");
        return;
    }
    if (word_count < 2) {
        switch (beeps) {
        case 0:
            write_user(user, usage);
            return;
        case 1:
            write_user(user, busage);
            return;
        }
    }
    /* wizzes should be trusted...But they are not! */
    switch (amsys->ban_swearing) {
    case SBMAX:
        if (contains_swearing(inpstr)) {
            write_user(user, noswearing);
            return;
        }
        break;
    case SBMIN:
        inpstr = censor_swear_words(inpstr);
        break;
    case SBOFF:
    default:
        /* do nothing as ban_swearing is off */
        break;
    }
    force_listen = 1;
    write_monitor(user, NULL, 0);
    vwrite_room(NULL, "%s~OL~FR--==<~RS %s~RS ~OL~FR>==--\n",
            beeps ? "\007" : "", inpstr);
}
