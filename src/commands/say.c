#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Say user speech.
 */
void
say(UR_OBJECT user, char *inpstr)
{
    static const char usage[] = "Usage: say <text>\n";
    const char *type;
    const char *name;

    /* FIXME: Use sentinel other JAILED */
    if (user->muzzled != JAILED) {
        write_user(user, "You are muzzled, you cannot speak.\n");
        return;
    }
    if (!strlen(inpstr)) {
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
    type = smiley_type(inpstr);
    if (!type) {
        type = "say";
    }
    if (user->type == CLONE_TYPE) {
        sprintf(text, "Clone of %s~RS ~FG%ss~RS: %s\n", user->recap, type,
                inpstr);
        record(user->room, text);
        write_room(user->room, text);
        return;
    }
    if (!user->vis) {
        write_monitor(user, user->room, 0);
    }
    name = user->vis ? user->recap : invisname;
    sprintf(text, "%s~RS ~FG%ss~RS: %s\n", name, type, inpstr);
    record(user->room, text);
    write_room_except(user->room, text, user);
    vwrite_user(user, "You ~FG%s~RS: %s\n", type, inpstr);
}