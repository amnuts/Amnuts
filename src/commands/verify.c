
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * verify that mail has been sent to the address supplied
 */
void
verify_email(UR_OBJECT user)
{
    if (word_count < 2) {
        write_user(user, "Usage: verify <verification code>\n");
        return;
    }
    if (!*user->email) {
        write_user(user,
                "You have not yet set your email address.  You must do this first.\n");
        return;
    }
    if (user->mail_verified) {
        write_user(user,
                "You have already verified your current email address.\n");
        return;
    }
    if (strcmp(user->verify_code, word[1]) || !*user->verify_code) {
        write_user(user,
                "That does not match your verification code.  Check your code and try again.\n");
        return;
    }
    *user->verify_code = '\0';
    user->mail_verified = 1;
    write_user(user,
            "\nThe verification code you gave was correct.\nYou may now use the auto-forward functions.\n\n");
}
