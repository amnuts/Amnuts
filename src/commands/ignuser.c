
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * set to ignore/listen to a user
 */
void
set_igusers(UR_OBJECT user)
{
    UR_OBJECT u;

    if (word_count < 2) {
        show_igusers(user);
        return;
    }
    /* add or remove ignores */
    u = retrieve_user(user, word[1]);
    if (!u) {
        return;
    }

    if (user == u) {
        write_user(user, "You cannot ignore yourself!\n");
        return;
    }
    /* do it */
    if (check_igusers(user, u)) {
        if (unsetbit_flagged_user_entry(user, u->name, fufIGNORE)) {
            vwrite_user(user, "You will once again listen to %s.\n", u->name);
        } else {
            vwrite_user(user,
                    "Sorry, but you could not unignore %s at this time.\n",
                    u->name);
        }
    } else {
        if (setbit_flagged_user_entry(user, u->name, fufIGNORE)) {
            vwrite_user(user, "You will now ignore speech from %s.\n", u->name);
        } else {
            vwrite_user(user, "Sorry, but you could not ignore %s at this time.\n",
                    u->name);
        }
    }
    /* finish up */
    done_retrieve(u);
}

