#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * this function allows users to give access to others even if their personal room
 * has been locked
 */
void
personal_room_key(UR_OBJECT user)
{
    RM_OBJECT rm;
    FU_OBJECT fu;

    if (!amsys->personal_rooms) {
        write_user(user, "Personal room functions are currently disabled.\n");
        return;
    }

    /* if no name was given then display keys given */
    if (word_count < 2) {
        char text2[ARR_SIZE];
        int found = 0, cnt = 0;

        *text2 = '\0';
        for (fu = user->fu_first; fu; fu = fu->next) {
            if (fu->flags & fufROOMKEY) {
                if (!found++) {
                    write_user(user,
                            "+----------------------------------------------------------------------------+\n");
                    write_user(user,
                            "| ~OL~FCYou have given the following people a key to your room~RS                     |\n");
                    write_user(user,
                            "+----------------------------------------------------------------------------+\n");
                }
                switch (++cnt) {
                case 1:
                    sprintf(text, "| %-24s", fu->name);
                    strcat(text2, text);
                    break;
                case 2:
                    sprintf(text, " %-24s", fu->name);
                    strcat(text2, text);
                    break;
                default:
                    sprintf(text, " %-24s |\n", fu->name);
                    strcat(text2, text);
                    write_user(user, text2);
                    cnt = 0;
                    *text2 = '\0';
                    break;
                }
            }
        }
        if (!found) {
            write_user(user,
                    "You have not given anyone a personal room key yet.\n");
            return;
        }
        if (cnt == 1) {
            strcat(text2, "                                                   |\n");
            write_user(user, text2);
        } else if (cnt == 2) {
            strcat(text2, "                          |\n");
            write_user(user, text2);
        }
        write_user(user,
                "+----------------------------------------------------------------------------+\n");
        return;
    }

    {
        char rmname[ROOM_NAME_LEN + 1];

        /* see if user has a room created */
        sprintf(rmname, "(%s)", user->name);
        strtolower(rmname);
        rm = get_room_full(rmname);
        if (!rm) {
            write_user(user,
                    "Sorry, but you have not created a personal room yet.\n");
            return;
        }
    }

    /* actually add/remove a user */
    strtolower(word[1]);
    *word[1] = toupper(*word[1]);
    if (!strcmp(user->name, word[1])) {
        write_user(user, "You already have access to your own room!\n");
        return;
    }
    /*
     * check to see if the user is already listed before the adding part.
     * This is to ensure you can remove a user even if they have, for
     * instance, suicided.
     */
    if (has_room_key(word[1], rm)) {
        if (!personal_key_remove(user, word[1])) {
            write_user(user,
                    "There was an error taking the key away from that user.\n");
            return;
        }
        vwrite_user(user,
                "You take your personal room key away from ~FC~OL%s~RS.\n",
                word[1]);
        vwrite_user(get_user(word[1]), "%s takes back their personal room key.\n",
                user->name);
    } else {
        /* see if there is such a user */
        if (!find_user_listed(word[1])) {
            write_user(user, nosuchuser);
            return;
        }
        /* give them a key */
        if (!personal_key_add(user, word[1])) {
            write_user(user, "There was an error giving the key to that user.\n");
            return;
        }
        vwrite_user(user, "You give ~FC~OL%s~RS a key to your personal room.\n",
                word[1]);
        vwrite_user(get_user(word[1]), "%s gives you a key to their room.\n",
                user->name);
    }
}
