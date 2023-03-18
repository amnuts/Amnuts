
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * put speech in a think bubbles
 */
void
think_it(UR_OBJECT user, char *inpstr)
{
#if !!0
    static const char usage[] = "Usage: think [<text>]\n";
#endif
    const char *name;

    /* FIXME: Use sentinel other JAILED */
    if (user->muzzled != JAILED) {
        write_user(user, "You are muzzled, you cannot think out loud.\n");
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
    if (!user->vis) {
        write_monitor(user, user->room, 0);
    }
    name = user->vis ? user->recap : invisname;
    if (word_count < 2) {
        sprintf(text, "%s~RS thinks nothing--now that is just typical!\n", name);
    } else {
        sprintf(text, "%s~RS thinks . o O ( %s~RS )\n", name, inpstr);
    }
    record(user->room, text);
    write_room(user->room, text);
}
