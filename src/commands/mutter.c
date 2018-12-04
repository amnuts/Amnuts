
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Tell something to everyone but one person
 */
void
mutter(UR_OBJECT user, char *inpstr)
{
    static const char usage[] = "Usage: mutter <user> <text>\n";
    const char *type;
    const char *name, *n;
    UR_OBJECT u;

    if (word_count < 3) {
        write_user(user, usage);
        return;
    }
    u = get_user_name(user, word[1]);
    if (!u) {
        write_user(user, notloggedon);
        return;
    }
    if (u == user) {
        write_user(user, "Talking about yourself is a sign of madness!\n");
        return;
    }
    if (user->room != u->room || (!u->vis && user->level < u->level)) {
        vwrite_user(user, "You cannot see %s~RS, so speak freely of them.\n",
                u->recap);
        return;
    }
    inpstr = remove_first(inpstr);
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
        type = "mutter";
    }
    if (!user->vis) {
        write_monitor(user, user->room, 0);
    }
    name = user->vis ? user->recap : invisname;
    n = u->vis ? u->recap : invisname;
    sprintf(text, "(NOT %s~RS) %s~RS ~FC%ss~RS: %s\n", n, name, type,
            inpstr);
#if !!0
    record(user->room, text);
#endif
    write_room_except_both(user->room, text, user, u);
    vwrite_user(user, "(NOT %s~RS) You ~FC%s~RS: %s\n", u->recap, type,
            inpstr);
}
