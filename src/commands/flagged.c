#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * See if user is banned
 */
void
show_flagged_users(UR_OBJECT user)
{
    FU_OBJECT fu;
    int count;

    if (!user->fu_first) {
        write_user(user, "You do not have any flagged users.\n");
        return;
    }
    count = 0;
    for (fu = user->fu_first; fu; fu = fu->next) {
        *text = '\0';
        if (!count++) {
            write_user(user,
                    "\n+----------------------------------------------------------------------------+\n");
            write_user(user,
                    "| ~OL~FCYour flagged user details are as follows~RS                                   |\n");
            write_user(user,
                    "+----------------------------------------------------------------------------+\n");
        }
        if (fu->flags & fufROOMKEY) {
            strcat(text, "my room key | ");
        } else {
            strcat(text, "            | ");
        }
        if (fu->flags & fufFRIEND) {
            strcat(text, "friend | ");
        } else {
            strcat(text, "       | ");
        }
        if (fu->flags & fufIGNORE) {
            strcat(text, "ignoring");
        } else {
            strcat(text, "        ");
        }
        vwrite_user(user, "| %-20s %53s |\n", fu->name, text);
    }
    write_user(user,
            "+----------------------------------------------------------------------------+\n");
}
