
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Show command, i.e., "Type --> <text>"
 */
void
show(UR_OBJECT user, char *inpstr)
{
    static const char usage[] = "Usage: show <text>\n";

    /* FIXME: Use sentinel other JAILED */
    if (user->muzzled != JAILED) {
        write_user(user, "You are currently muzzled and cannot show.\n");
        return;
    }
    if (word_count < 2 && strlen(inpstr) < 2) {
        write_user(user, usage);
        return;
    }
    switch (amsys->ban_swearing) {
    case SBMAX:
        if (contains_swearing(inpstr)) {
            write_user(user, noswearing);
            return;
        }
        break;
    case SBMIN:
        if (!is_private_room(user->room)) {
            inpstr = censor_swear_words(inpstr);
        }
        break;
    case SBOFF:
    default:
        /* do nothing as ban_swearing is off */
        break;
    }
    write_monitor(user, user->room, 0);
    vwrite_room(user->room, "~OL~FCType -->~RS %s\n", inpstr);
}
