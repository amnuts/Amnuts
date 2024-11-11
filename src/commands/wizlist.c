
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Show the wizzes that are currently logged on, and get a list of names from the lists saved
 */
void
wiz_list(UR_OBJECT user)
{
    static const char *const clrs[] ={"~FC", "~FM", "~FG", "~FB", "~OL", "~FR", "~FY"};
    char text2[ARR_SIZE];
    char temp[ARR_SIZE];
    UR_OBJECT u;
    UD_OBJECT entry;
    int invis, count, inlist;
    int linecnt, rnamecnt;
    enum lvl_value lvl;

    /* show this for everyone */
    write_user(user,
            "+----- ~FGWiz List~RS -------------------------------------------------------------+\n\n");
    for (lvl = GOD; lvl >= WIZ; lvl = (enum lvl_value) (lvl - 1)) {
        *text2 = '\0';
        count = 0;
        inlist = 0;
        sprintf(text, "~OL%s%-10s~RS : ", clrs[lvl % 4], user_level[lvl].name);
        for (entry = first_user_entry; entry; entry = entry->next) {
            if (entry->level < WIZ) {
                continue;
            }
            if (is_retired(entry->name)) {
                continue;
            }
            if (entry->level == lvl) {
                if (count > 3) {
                    strcat(text2, "\n             ");
                    count = 0;
                }
                sprintf(temp, "~OL%s%-*s~RS  ", clrs[rand() % 7], USER_NAME_LEN,
                        entry->name);
                strcat(text2, temp);
                ++count;
                inlist = 1;
            }
        }
        if (!count && !inlist) {
            sprintf(text2, "~FR[none listed]\n~RS");
        }
        strcat(text, text2);
        write_user(user, text);
        if (count) {
            write_user(user, "\n");
        }
    }

    /* show this to just the wizzes */
    if (user->level >= WIZ) {
        write_user(user,
                "\n+----- ~FGRetired Wiz List~RS -----------------------------------------------------+\n\n");
        for (lvl = GOD; lvl >= WIZ; lvl = (enum lvl_value) (lvl - 1)) {
            *text2 = '\0';
            count = 0;
            inlist = 0;
            sprintf(text, "~OL%s%-10s~RS : ", clrs[lvl % 4], user_level[lvl].name);
            for (entry = first_user_entry; entry; entry = entry->next) {
                if (entry->level < WIZ) {
                    continue;
                }
                if (!is_retired(entry->name)) {
                    continue;
                }
                if (entry->level == lvl) {
                    if (count > 3) {
                        strcat(text2, "\n             ");
                        count = 0;
                    }
                    sprintf(temp, "~OL%s%-*s~RS  ", clrs[rand() % 7], USER_NAME_LEN,
                            entry->name);
                    strcat(text2, temp);
                    ++count;
                    inlist = 1;
                }
            }
            if (!count && !inlist) {
                sprintf(text2, "~FR[none listed]\n~RS");
            }
            strcat(text, text2);
            write_user(user, text);
            if (count) {
                write_user(user, "\n");
            }
        }
    }
    /* show this to everyone */
    write_user(user,
            "\n+----- ~FGThose currently on~RS ---------------------------------------------------+\n\n");
    invis = 0;
    count = 0;
    for (u = user_first; u; u = u->next)
        if (u->room) {
            if (u->level >= WIZ) {
                if (!u->vis && (user->level < u->level && !(user->level >= ARCH))) {
                    ++invis;
                    continue;
                } else {
                    if (u->vis) {
                        sprintf(text2, "  %s~RS %s~RS", u->recap, u->desc);
                    } else {
                        sprintf(text2, "* %s~RS %s~RS", u->recap, u->desc);
                    }
                    linecnt = 43 + teslen(text2, 43);
                    rnamecnt = 15 + teslen(u->room->show_name, 15);
                    vwrite_user(user, "%-*.*s~RS : %-*.*s~RS : (%1.1s) %s\n", linecnt,
                            linecnt, text2, rnamecnt, rnamecnt, u->room->show_name,
                            user_level[u->level].alias, user_level[u->level].name);
                }
            }
            ++count;
        }
    if (invis) {
        vwrite_user(user, "Number of the wiz invisible to you : %d\n", invis);
    }
    if (!count) {
        write_user(user, "Sorry, no wizzes are on at the moment...\n");
    }
    write_user(user, "\n");
    write_user(user,
            "+----------------------------------------------------------------------------+\n");
}
