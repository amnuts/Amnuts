
#include "../includes/defines.h"
#include "../includes/globals.h"
#include "../includes/commands.h"
#include "../includes/prototypes.h"

/*
 * Ban a site, domain or user
 */
void
ban(UR_OBJECT user)
{
    static const char usage[] =
            "Usage: ban site|user|new <site>|<user>|<site>\n";

    if (word_count < 3) {
        write_user(user, usage);
        return;
    }
    strtolower(word[1]);
    if (!strcmp(word[1], "site")) {
        ban_site(user);
        return;
    }
    if (!strcmp(word[1], "user")) {
        ban_user(user);
        return;
    }
    if (!strcmp(word[1], "new")) {
        ban_new(user);
        return;
    }
    write_user(user, usage);
}
