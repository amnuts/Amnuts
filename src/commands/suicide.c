
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Allow a user to delete their own account
 */
void
suicide(UR_OBJECT user)
{
    if (word_count < 2) {
        write_user(user, "Usage: suicide <your password>\n");
        return;
    }
    if (strcmp(user->pass, crypt(word[1], user->pass))) {
        write_user(user, "Password incorrect.\n");
        return;
    }
    write_user(user,
            "\n\07~FR~OL~LI*** WARNING - This will delete your account! ***\n\nAre you sure about this (y|n)? ");
    user->misc_op = 6;
    no_prompt = 1;
}
