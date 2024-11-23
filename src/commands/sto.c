#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Shout something to someone
 */
void
sto(UR_OBJECT user, char *inpstr)
{
    static const char usage[] = "Usage: sto <user> <text>\n";
    const char *type;
    const char *name, *n;
    UR_OBJECT u;

    /* FIXME: Use sentinel other JAILED */
    if (user->muzzled != JAILED) {
        write_user(user, "You are muzzled, you cannot speak.\n");
        return;
    }
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
        write_user(user, "Talking to yourself is the first sign of madness.\n");
        return;
    }
    if (check_igusers(u, user) && user->level < GOD) {
        vwrite_user(user, "%s~RS is ignoring you.\n",
                u->recap);
        return;
    }
    if (u->igntells && (user->level < WIZ || u->level > user->level)) {
        vwrite_user(user, "%s~RS is ignoring stuff at the moment.\n",
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
        inpstr = censor_swear_words(inpstr);
        break;
    case SBOFF:
    default:
        /* do nothing as ban_swearing is off */
        break;
    }
    type = smiley_type(inpstr);
    if (!type) {
        type = "shout";
    }
    if (!user->vis) {
        write_monitor(user, NULL, 0);
    }
    name = user->vis ? user->recap : invisname;
    n = u->vis ? u->recap : invisname;
    sprintf(text, "~OL!~RS %s~RS ~OL%ss~RS to ~OL%s~RS: %s~RS\n", name, type, n, inpstr);
    record_shout(text);
    write_room_except(NULL, text, user);
    vwrite_user(user, "~OL!~RS You ~OL%s~RS to ~OL%s~RS: %s~RS\n", type, n, inpstr);
}
