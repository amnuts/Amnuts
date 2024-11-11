
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * uban a site (or domain) or user
 */
void
unban(UR_OBJECT user)
{
    static const char usage[] =
            "Usage: unban site|user|new <site>|<user>|<site>\n";

    if (word_count < 3) {
        write_user(user, usage);
        return;
    }
    strtolower(word[1]);
    if (!strcmp(word[1], "site")) {
        unban_site(user);
        return;
    }
    if (!strcmp(word[1], "user")) {
        unban_user(user);
        return;
    }
    if (!strcmp(word[1], "new")) {
        unban_new(user);
        return;
    }
    write_user(user, usage);
}
