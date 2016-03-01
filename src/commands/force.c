
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Force a user to do something
 * adapted from Ogham: "Oh God Here's Another MUD" (c) Neil Robertson
 */
void
force(UR_OBJECT user, char *inpstr)
{
    UR_OBJECT u;
    int w;

    if (word_count < 3) {
        write_user(user, "Usage: force <user> <action>\n");
        return;
    }
    *word[1] = toupper(*word[1]);
    u = get_user_name(user, word[1]);
    if (!u) {
        write_user(user, notloggedon);
        return;
    }
    if (u == user) {
        write_user(user, "There is an easier way to do something yourself.\n");
        return;
    }
    if (u->level >= user->level) {
        write_user(user,
                "You cannot force a user of the same or higher level as yourself.\n");
        return;
    }
    inpstr = remove_first(inpstr);
    write_syslog(SYSLOG, 0, "%s FORCED %s to: \"%s\"\n", user->name, u->name,
            inpstr);
    /* shift words down to pass to exec_com */
    word_count -= 2;
    for (w = 0; w < word_count; ++w) {
        strcpy(word[w], word[w + 2]);
    }
    *word[w++] = '\0';
    *word[w++] = '\0';
    vwrite_user(u, "%s~RS forces you to: \"%s\"\n", user->recap, inpstr);
    vwrite_user(user, "You force %s~RS to: \"%s\"\n", u->recap, inpstr);
    if (!exec_com(u, inpstr, COUNT)) {
        vwrite_user(user, "Unable to execute the command for %s~RS.\n", u->recap);
    }
    prompt(u);
}
