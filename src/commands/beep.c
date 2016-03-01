
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Beep a user - as tell but with audio warning
 */
void
beep(UR_OBJECT user, char *inpstr)
{
    static const char usage[] = "Usage: beep <user> [<text>]\n";
    const char *name;
    UR_OBJECT u;

    /* FIXME: Use sentinel other JAILED */
    if (user->muzzled != JAILED) {
        write_user(user, "You are muzzled, you cannot beep.\n");
        return;
    }
    if (word_count < 2) {
        write_user(user, usage);
        return;
    }
    u = get_user_name(user, word[1]);
    if (!u) {
        write_user(user, notloggedon);
        return;
    }
    if (u == user) {
        write_user(user, "Beeping yourself is yet another sign of madness!\n");
        return;
    }
    if (check_igusers(u, user) && user->level < GOD) {
        vwrite_user(user, "%s~RS is ignoring beeps from you.\n", u->recap);
        return;
    }
    if (u->ignbeeps && (user->level < WIZ || u->level > user->level)) {
        vwrite_user(user, "%s~RS is ignoring beeps at the moment.\n", u->recap);
        return;
    }
    if (u->afk) {
        write_user(user, "You cannot beep someone who is AFK.\n");
        return;
    }
    if (u->malloc_start) {
        vwrite_user(user, "%s~RS is writing a message at the moment.\n",
                u->recap);
        return;
    }
    if (u->ignall && (user->level < WIZ || u->level > user->level)) {
        vwrite_user(user, "%s~RS is not listening at the moment.\n", u->recap);
        return;
    }
#ifdef NETLINKS
    if (!u->room) {
        vwrite_user(user,
                "%s~RS is offsite and would not be able to reply to you.\n",
                u->recap);
        return;
    }
#endif
    name = user->vis || u->level >= user->level ? user->recap : invisname;
    if (word_count < 3) {
        vwrite_user(u, "\007%s~RS ~OL~FRbeeps to you~RS: ~FR-=[*] BEEP [*]=-\n",
                name);
        vwrite_user(user, "\007You ~OL~FRbeep to~RS %s~RS: ~FR-=[*] BEEP [*]=-\n",
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
    vwrite_user(u, "\007%s~RS ~OL~FRbeeps to you~RS: %s\n", name, inpstr);
    vwrite_user(user, "\007You ~OL~FRbeep to~RS %s~RS: %s\n", u->recap, inpstr);
}
