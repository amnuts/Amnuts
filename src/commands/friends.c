#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * take a users name and add it to friends list
 */
void
friends(UR_OBJECT user)
{
    int found, cnt;
    FU_OBJECT fu;
    UR_OBJECT u;
    char text2[ARR_SIZE];

    found = cnt = 0;
    *text2 = '\0';
    /* just display the friend details */
    if (word_count < 2) {
        if (!count_friends(user)) {
            write_user(user, "You have no names on your friends list.\n");
            return;
        }
        for (fu = user->fu_first; fu; fu = fu->next) {
            if (fu->flags & fufFRIEND) {
                if (!found++) {
                    write_user(user,
                            "+----------------------------------------------------------------------------+\n");
                    write_user(user,
                            "| ~FC~OLNames on your friends list are as follows~RS                                  |\n");
                    write_user(user,
                            "+----------------------------------------------------------------------------+\n");
                }
                switch (++cnt) {
                case 1:
                    if (get_user(fu->name)) {
                        sprintf(text, "| %-25s     ~FY~OLONLINE~RS ", fu->name);
                    } else {
                        sprintf(text, "| %-25s            ", fu->name);
                    }
                    strcat(text2, text);
                    break;
                default:
                    if (get_user(fu->name)) {
                        sprintf(text, " %-25s     ~FY~OLONLINE~RS |\n", fu->name);
                    } else {
                        sprintf(text, " %-25s            |\n", fu->name);
                    }
                    strcat(text2, text);
                    write_user(user, text2);
                    cnt = 0;
                    *text2 = '\0';
                    break;
                }
            }
        }
        if (cnt == 1) {
            strcat(text2, "                                      |\n");
            write_user(user, text2);
        }
        write_user(user,
                "+----------------------------------------------------------------------------+\n");
        if (!user->alert) {
            write_user(user,
                    "| ~FCYou are currently not being alerted~RS                                        |\n");
        } else {
            write_user(user,
                    "| ~OL~FCYou are currently being alerted~RS                                            |\n");
        }
        write_user(user,
                "+----------------------------------------------------------------------------+\n");
        return;
    }
    /* add or remove friends */
    u = retrieve_user(user, word[1]);
    if (!u) {
        return;
    }
    if (user_is_friend(user, u)) {
        if (!unsetbit_flagged_user_entry(user, u->name, fufFRIEND)) {
            vwrite_user(user,
                    "Sorry, but %s could not be removed from your friends list at this time.\n",
                    u->name);
        } else {
            vwrite_user(user, "You have now removed %s from your friends list.\n",
                    u->name);
        }
    } else {
        if (!setbit_flagged_user_entry(user, u->name, fufFRIEND)) {
            vwrite_user(user,
                    "Sorry, but %s could not be added to your friends list at this time.\n",
                    u->name);
        } else {
            vwrite_user(user, "You have now added %s to your friends list.\n",
                    u->name);
        }
    }
    done_retrieve(u);
}
