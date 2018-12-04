
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Do AFK
 */
void
afk(UR_OBJECT user, char *inpstr)
{
    if (word_count > 1) {
        if (!strcmp(word[1], "lock")) {
#ifdef NETLINKS
            if (user->type == REMOTE_TYPE) {
                /*
                   This is because they might not have a local account and hence
                   they have no password to use.
                 */
                write_user(user,
                        "Sorry, due to software limitations remote users cannot use the lock option.\n");
                return;
            }
#endif
            inpstr = remove_first(inpstr);
            if (strlen(inpstr) > AFK_MESG_LEN) {
                write_user(user, "AFK message too long.\n");
                return;
            }
            write_user(user,
                    "You are now AFK with the session locked, enter your password to unlock it.\n");
            if (*inpstr) {
                strcpy(user->afk_mesg, inpstr);
                write_user(user, "AFK message set.\n");
            }
            user->afk = 2;
        } else {
            if (strlen(inpstr) > AFK_MESG_LEN) {
                write_user(user, "AFK message too long.\n");
                return;
            }
            write_user(user, "You are now AFK, press <return> to reset.\n");
            if (*inpstr) {
                strcpy(user->afk_mesg, inpstr);
                write_user(user, "AFK message set.\n");
            }
            user->afk = 1;
        }
    } else {
        write_user(user, "You are now AFK, press <return> to reset.\n");
        user->afk = 1;
    }
    if (user->vis) {
        if (*user->afk_mesg) {
            vwrite_room_except(user->room, user, "%s~RS goes AFK: %s\n",
                    user->recap, user->afk_mesg);
        } else {
            vwrite_room_except(user->room, user, "%s~RS goes AFK...\n",
                    user->recap);
        }
    }
    clear_afk(user);
}
