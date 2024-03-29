
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Make a clone emote
 */
void
clone_emote(UR_OBJECT user, char *inpstr)
{
    static const char usage[] = "Usage: cemote <room> <text>\n";
    RM_OBJECT rm;
    UR_OBJECT u;

    /* FIXME: Use sentinel other JAILED */
    if (user->muzzled != JAILED) {
        write_user(user, "You are muzzled, your clone cannot emote.\n");
        return;
    }
    if (word_count < 3) {
        write_user(user, usage);
        return;
    }
    rm = get_room(word[1]);
    if (!rm) {
        write_user(user, nosuchroom);
        return;
    }
    for (u = user_first; u; u = u->next) {
        if (u->type == CLONE_TYPE && u->room == rm && u->owner == user) {
            break;
        }
    }
    if (!u) {
        write_user(user, "You do not have a clone in that room.\n");
        return;
    }
    emote(u, remove_first(inpstr));
}
