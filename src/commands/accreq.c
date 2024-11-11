
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * A newbie is requesting an account. Get his email address off him so we
 * can validate who he is before we promote him and let him loose as a
 * proper user.
 */
void
account_request(UR_OBJECT user, char *inpstr)
{
    if (user->level != NEW) {
        write_user(user,
                "This command is for new users only, you already have a full account.\n");
        return;
    }
    /* stop them from requesting an account twice */
    if (user->accreq & BIT(3)) {
        write_user(user, "You have already requested an account.\n");
        return;
    }
    if (word_count < 2) {
        write_user(user, "Usage: accreq <email> [<text>]\n");
        return;
    }
    if (!validate_email(word[1])) {
        write_user(user,
                "That email address format is incorrect. Correct format: user@network.net\n");
        return;
    }
    write_syslog(REQLOG, 1, "%-*s : %s\n", USER_NAME_LEN, user->name, inpstr);
    vwrite_level(WIZ, 1, RECORD, user,
            "~OLSYSTEM:~RS %s~RS has made an account request with: %s\n",
            user->recap, inpstr);
    write_user(user, "Account request logged.\n");
    add_history(user->name, 1, "Made a request for an account.\n");
    /* permanent record of email address in user history file */
    sprintf(text, "Used email address \"%s\" in the request.\n", word[1]);
    add_history(user->name, 1, "%s", text);
    /* check to see if user should be promoted yet */
    check_autopromote(user, 3);
}
