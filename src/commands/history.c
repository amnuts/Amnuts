
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * shows the history file of a given user
 */
void
user_history(UR_OBJECT user)
{
    char filename[80], name[USER_NAME_LEN + 1];
    UR_OBJECT u;

    if (word_count < 2) {
        write_user(user, "Usage: history <user>\n");
        return;
    }
    u = retrieve_user(user, word[1]);
    if (!u) {
        return;
    }
    strcpy(name, u->name);
    done_retrieve(u);
    /* show file */
    sprintf(filename, "%s/%s/%s.H", USERFILES, USERHISTORYS, name);
    vwrite_user(user,
            "~FG*** The history of user ~OL%s~RS~FG is as follows ***\n\n",
            name);
    switch (more(user, user->socket, filename)) {
    case 0:
        sprintf(text, "%s has no previously recorded history.\n\n", name);
        write_user(user, text);
        break;
    case 1:
        user->misc_op = 2;
        break;
    }
}
