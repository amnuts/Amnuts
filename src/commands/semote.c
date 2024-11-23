
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Do a shout emote
 */
void
semote(UR_OBJECT user, char *inpstr)
{
    static const char usage[] = "Usage: semote <text>\n";
    const char *name;

    /* FIXME: Use sentinel other JAILED */
    if (user->muzzled != JAILED) {
        write_user(user, "You are muzzled, you cannot emote.\n");
        return;
    }
    if (word_count < 2) {
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
        inpstr = censor_swear_words(inpstr);
        break;
    case SBOFF:
    default:
        /* do nothing as ban_swearing is off */
        break;
    }
    if (!user->vis) {
        write_monitor(user, NULL, 0);
    }
    name = user->vis ? user->recap : invisname;
    sprintf(text, "~OL!~RS %s~RS%s%s\n", name, *inpstr != '\'' ? " " : "",
            inpstr);
    record_shout(text);
    write_room_ignore(NULL, user, text);
}
