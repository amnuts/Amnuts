
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Wake up some idle sod
 */
void
wake(UR_OBJECT user)
{
    static const char usage[] = "Usage: wake <user>\n";
    UR_OBJECT u;
    const char *name;

    /* FIXME: Use sentinel other JAILED */
    if (user->muzzled != JAILED) {
        write_user(user, "You are muzzled, you cannot wake anyone.\n");
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
        write_user(user,
                "Trying to wake yourself up is the eighth sign of madness.\n");
        return;
    }
    if (check_igusers(u, user) && user->level < GOD) {
        vwrite_user(user, "%s~RS is ignoring wakes from you.\n", u->recap);
        return;
    }
    if (u->ignbeeps && (user->level < WIZ || u->level > user->level)) {
        vwrite_user(user, "%s~RS is ignoring wakes at the moment.\n", u->recap);
        return;
    }
    if (u->afk) {
        write_user(user, "You cannot wake someone who is AFK.\n");
        return;
    }
    if (u->malloc_start) {
        write_user(user, "You cannot wake someone who is in the editor.\n");
        return;
    }
    if (u->ignall && (user->level < WIZ || u->level > user->level)) {
        write_user(user, "You cannot wake someone who ignoring everything.\n");
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
    name = user->vis ? user->recap : invisname;
    vwrite_user(u,
            "\n%s~BR***~RS %s~RS ~BRsays~RS: ~OL~LI~BRHEY! WAKE UP!!!~RS ~BR***\n\n",
            u->ignbeeps ? "" : "\007", name);
    write_user(user, "Wake up call sent.\n");
}
