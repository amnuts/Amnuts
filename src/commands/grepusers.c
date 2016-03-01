#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * This command allows you to do a search for any user names that match
 * a particular pattern
 */
void
grep_users(UR_OBJECT user)
{
    int found, x;
    char name[USER_NAME_LEN + 1], pat[ARR_SIZE];
    UD_OBJECT entry;

    if (word_count < 2) {
        write_user(user, "Usage: grepu <pattern>\n");
        return;
    }
    if (strstr(word[1], "**")) {
        write_user(user, "You cannot have ** in your pattern.\n");
        return;
    }
    if (strstr(word[1], "?*")) {
        write_user(user, "You cannot have ?* in your pattern.\n");
        return;
    }
    if (strstr(word[1], "*?")) {
        write_user(user, "You cannot have *? in your pattern.\n");
        return;
    }
    start_pager(user);
    write_user(user,
            "\n+----------------------------------------------------------------------------+\n");
    sprintf(text, "| ~FC~OLUser grep for pattern:~RS ~OL%-51s~RS |\n", word[1]);
    write_user(user, text);
    write_user(user,
            "+----------------------------------------------------------------------------+\n");
    x = 0;
    found = 0;
    *pat = '\0';
    strcpy(pat, word[1]);
    strtolower(pat);
    for (entry = first_user_entry; entry; entry = entry->next) {
        strcpy(name, entry->name);
        *name = tolower(*name);
        if (pattern_match(name, pat)) {
            if (!x) {
                vwrite_user(user, "| %-*s  ~FC%-20s~RS   ", USER_NAME_LEN,
                        entry->name, user_level[entry->level].name);
            } else {
                vwrite_user(user, "   %-*s  ~FC%-20s~RS |\n", USER_NAME_LEN,
                        entry->name, user_level[entry->level].name);
            }
            x = !x;
            ++found;
        }
    }
    if (x) {
        write_user(user, "                                      |\n");
    }
    if (!found) {
        write_user(user,
                "|                                                                            |\n");
        write_user(user,
                "| ~OL~FRNo users have that pattern~RS                                                 |\n");
        write_user(user,
                "|                                                                            |\n");
        write_user(user,
                "+----------------------------------------------------------------------------+\n");
        stop_pager(user);
        return;
    }
    write_user(user,
            "+----------------------------------------------------------------------------+\n");
    write_user(user,
            align_string(0, 78, 1, "|",
            "  ~OL%d~RS user%s had the pattern ~OL%s~RS ",
            found, PLTEXT_S(found), word[1]));
    write_user(user,
            "+----------------------------------------------------------------------------+\n\n");
    stop_pager(user);
}
