#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Direct a say to someone, even though the whole room can hear it
 */
void
say_to(UR_OBJECT user, char *inpstr)
{
    static const char usage[] = "Usage: sayto <user> <text>\n";
    const char *type;
    const char *name, *n;
    UR_OBJECT u;

    /* FIXME: Use sentinel other JAILED */
    if (user->muzzled != JAILED) {
        write_user(user, "You are muzzled, you cannot speak.\n");
        return;
    }
    if (word_count < 3 && *inpstr != '-') {
        write_user(user, usage);
        return;
    }
    if (word_count < 2 && *inpstr == '-') {
        write_user(user, usage);
        return;
    }
    u = get_user_name(user, word[1]);
    if (!u) {
        write_user(user, notloggedon);
        return;
    }
    if (u == user) {
        write_user(user, "Talking to yourself is the first sign of madness!\n");
        return;
    }
    if (user->room != u->room || (!u->vis && user->level < u->level)) {
        vwrite_user(user,
                "You cannot see %s~RS, so you cannot say anything to them!\n",
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
        type = "say";
    }
    if (!user->vis) {
        write_monitor(user, user->room, 0);
    }
    name = user->vis ? user->recap : invisname;
    n = u->vis ? u->recap : invisname;
    sprintf(text, "(%s~RS) %s~RS ~FC%ss~RS: %s\n", n, name, type, inpstr);
    record(user->room, text);
    write_room_except(user->room, text, user);
    vwrite_user(user, "(%s~RS) You ~FC%s~RS: %s\n", u->recap, type, inpstr);
}
