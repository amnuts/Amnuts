
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * get users which to send copies of smail to
 */
void
copies_to(UR_OBJECT user)
{
    int i, found, x, y;
    char *remote;

    if (com_num == NOCOPIES) {
        for (i = 0; i < MAX_COPIES; ++i) {
            *user->copyto[i] = '\0';
        }
        write_user(user, "Sending no copies of your next smail.\n");
        return;
    }
    if (word_count < 2) {
        *text = '\0';
        found = 0;
        for (i = 0; i < MAX_COPIES; ++i) {
            if (!*user->copyto[i]) {
                continue;
            }
            if (!found++) {
                write_user(user, "Sending copies of your next smail to...\n");
            }
            strcat(text, "   ");
            strcat(text, user->copyto[i]);
        }
        strcat(text, "\n\n");
        if (!found) {
            write_user(user, "You are not sending a copy to anyone.\n");
        } else {
            write_user(user, text);
        }
        return;
    }
    if (word_count > MAX_COPIES + 1) { /* +1 because 1 count for command */
        vwrite_user(user, "You can only copy to a maximum of %d people.\n",
                MAX_COPIES);
        return;
    }
    /* check to see the user is not trying to send duplicates */
    for (x = 1; x < word_count; ++x) {
        for (y = x + 1; y < word_count; ++y) {
            if (!strcasecmp(word[x], word[y])) {
                write_user(user, "Cannot send to same person more than once.\n");
                return;
            }
        }
    }
    write_user(user, "\n");
    for (i = 0; i < MAX_COPIES; ++i) {
        *user->copyto[i] = '\0';
    }
    i = 0;
    for (x = 1; x < word_count; ++x) {
        /* See if its to another site */
        if (*word[x] == '@') {
            vwrite_user(user,
                    "Name missing before @ sign for copy to name \"%s\".\n",
                    word[x]);
            continue;
        }
        remote = strchr(word[x], '@');
        if (!remote) {
            *word[x] = toupper(*word[x]);
            /* See if user exists */
            if (get_user(word[x]) == user && user->level < ARCH) {
                write_user(user, "You cannot send yourself a copy.\n");
                continue;
            }
            if (!find_user_listed(word[x])) {
                vwrite_user(user,
                        "There is no such user with the name \"%s\" to copy to.\n",
                        word[x]);
                continue;
            }
        }
        strcpy(user->copyto[i++], word[x]);
    }
    *text = '\0';
    found = 0;
    for (i = 0; i < MAX_COPIES; ++i) {
        if (!*user->copyto[i]) {
            continue;
        }
        if (!found++) {
            write_user(user, "Sending copies of your next smail to...\n");
        }
        strcat(text, "   ");
        strcat(text, user->copyto[i]);
    }
    strcat(text, "\n\n");
    if (!found) {
        write_user(user, "You are not sending a copy to anyone.\n");
    } else {
        write_user(user, text);
    }
}
