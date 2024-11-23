
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * allows a user to add to another users history list
 */
void
manual_history(UR_OBJECT user, char *inpstr)
{
    if (word_count < 3) {
        write_user(user, "Usage: addhistory <user> <text>\n");
        return;
    }
    *word[1] = toupper(*word[1]);
    if (!strcmp(user->name, word[1])) {
        write_user(user, "You cannot add to your own history list.\n");
        return;
    }
    if (!find_user_listed(word[1])) {
        write_user(user, nosuchuser);
        return;
    }
    inpstr = remove_first(inpstr);
    sprintf(text, "%-*s : %s\n", USER_NAME_LEN, user->name, inpstr);
    add_history(word[1], 1, "%s", text);
    vwrite_user(user, "You have added to %s's history list.\n", word[1]);
}