
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Delete a user
 */
void
delete_user(UR_OBJECT user, int this_user)
{
    char name[USER_NAME_LEN + 1];
    UR_OBJECT u;

    if (this_user) {
        /*
         * User structure gets destructed in disconnect_user(), need to keep a
         * copy of the name
         */
        strcpy(name, user->name);
        write_user(user, "\n~FR~LI~OLACCOUNT DELETED!\n");
        vwrite_room_except(user->room, user, "~OL~LI%s commits suicide!\n",
                user->name);
        write_syslog(SYSLOG, 1, "%s SUICIDED.\n", name);
        disconnect_user(user);
        clean_files(name);
        rem_user_node(name);
        return;
    }
    if (word_count < 2) {
        write_user(user, "Usage: nuke <user>\n");
        return;
    }
    *word[1] = toupper(*word[1]);
    if (!strcmp(word[1], user->name)) {
        write_user(user,
                "Trying to delete yourself is the eleventh sign of madness.\n");
        return;
    }
    if (get_user(word[1])) {
        /* Safety measure just in case. Will have to .kill them first */
        write_user(user,
                "You cannot delete a user who is currently logged on.\n");
        return;
    }
    u = create_user();
    if (!u) {
        vwrite_user(user, "%s: unable to create temporary user object.\n",
                syserror);
        write_syslog(SYSLOG | ERRLOG, 0,
                "ERROR: Unable to create temporary user object in delete_user().\n");
        return;
    }
    strcpy(u->name, word[1]);
    if (!load_user_details(u)) {
        write_user(user, nosuchuser);
        destruct_user(u);
        destructed = 0;
        return;
    }
    if (u->level >= user->level) {
        write_user(user,
                "You cannot delete a user of an equal or higher level than yourself.\n");
        destruct_user(u);
        destructed = 0;
        return;
    }
    clean_files(u->name);
    rem_user_node(u->name);
    vwrite_user(user, "\07~FR~OL~LIUser %s deleted!\n", u->name);
    write_syslog(SYSLOG, 1, "%s DELETED %s.\n", user->name, u->name);
    destruct_user(u);
    destructed = 0;
}
