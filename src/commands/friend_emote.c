
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Emote something to all the people on the suers friends list
 */
void
friend_emote(UR_OBJECT user, char *inpstr)
{
    static const char usage[] = "Usage: femote <text>\n";
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
    /* check to see if use has friends listed */
    if (!count_friends(user)) {
        write_user(user, "You have no friends listed.\n");
        return;
    }
    /* sort out swearing */
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
    name = user->vis ? user->recap : invisname;
    sprintf(text, "~OL~FG>~RS [~FGFriend~RS] %s~RS%s%s\n", name,
            *inpstr != '\'' ? " " : "", inpstr);
    write_friends(user, text, 1);
    record_tell(user, user, text);
    write_user(user, text);
}
