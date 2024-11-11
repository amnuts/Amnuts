#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Retire a user--i.e., remove from the wizlist but do not alter level
 */
void
retire_user(UR_OBJECT user)
{
    UR_OBJECT u;
    int on;

    if (word_count < 2) {
        write_user(user, "Usage: retire <user>\n");
        return;
    }
    if (is_retired(word[1])) {
        vwrite_user(user, "%s has already been retired from the wizlist.\n",
                word[1]);
        return;
    }
    u = retrieve_user(user, word[1]);
    if (!u) {
        return;
    }
    on = retrieve_user_type == 1;
    if (u == user) {
        write_user(user, "You cannot retire yourself.\n");
        return;
    }
    if (u->level < WIZ) {
        write_user(user, "You cannot retire anyone under the level WIZ\n");
        return;
    }
    u->retired = 1;
    add_retire_list(u->name);
    vwrite_user(user, "You retire %s from the wizlist.\n", u->name);
    write_syslog(SYSLOG, 1, "%s RETIRED %s\n", user->name, u->name);
    sprintf(text, "Was ~FRretired~RS by %s.\n", user->name);
    add_history(u->name, 1, "%s", text);
    sprintf(text,
            "You have been retired from the wizlist but still retain your level.\n");
    if (!on) {
        send_mail(user, u->name, text, 0);
    } else {
        write_user(u, text);
    }
    if (!on) {
        strcpy(u->site, u->last_site);
        u->socket = -2;
    }
    save_user_details(u, on);
    done_retrieve(u);
}
