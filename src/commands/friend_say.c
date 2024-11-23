
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Say user speech to all people listed on users friends list
 */
void
friend_say(UR_OBJECT user, char *inpstr)
{
    static const char usage[] = "Usage: fsay <text>\n";
    const char *type;
    const char *name;

    /* FIXME: Use sentinel other JAILED */
    if (user->muzzled != JAILED) {
        write_user(user, "You are muzzled, you cannot speak.\n");
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
    /* sort our swearing */
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
    type = smiley_type(inpstr);
    if (!type) {
        type = "say";
    }
    name = user->vis ? user->recap : invisname;
    sprintf(text, "~OL~FG>~RS [~FGFriend~RS] %s~RS ~FG%ss~RS: %s\n", name,
            type, inpstr);
    write_friends(user, text, 1);
    sprintf(text, "~OL~FG>~RS [~FGFriend~RS] You ~FG%s~RS: %s\n", type,
            inpstr);
    record_tell(user, user, text);
    write_user(user, text);
}
