
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Emote something
 */
void
emote(UR_OBJECT user, char *inpstr)
{
    static const char usage[] = "Usage: emote <text>\n";
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
    if (user->type == CLONE_TYPE) {
        sprintf(text, "Clone of %s~RS%s%s\n", name, *inpstr != '\'' ? " " : "",
                inpstr);
        record(user->room, text);
        write_room(user->room, text);
        return;
    }
    sprintf(text, "%s~RS%s%s\n", name, *inpstr != '\'' ? " " : "", inpstr);
    record(user->room, text);
    write_room_ignore(user->room, user, text);
}
