
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Set what a clone will hear, either all speach , just bad language
 * or nothing.
 */
void
clone_hear(UR_OBJECT user)
{
    static const char usage[] = "Usage: chear <room> all|swears|nothing\n";
    RM_OBJECT rm;
    UR_OBJECT u;

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
    strtolower(word[2]);
    if (!strcmp(word[2], "all")) {
        u->clone_hear = CLONE_HEAR_ALL;
        write_user(user, "Clone will now hear everything.\n");
        return;
    }
    if (!strcmp(word[2], "swears")) {
        u->clone_hear = CLONE_HEAR_SWEARS;
        write_user(user, "Clone will now only hear swearing.\n");
        return;
    }
    if (!strcmp(word[2], "nothing")) {
        u->clone_hear = CLONE_HEAR_NOTHING;
        write_user(user, "Clone will now hear nothing.\n");
        return;
    }
    write_user(user, usage);
}
