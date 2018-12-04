#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Unretire a user--i.e., put them back on show on the wizlist
 */
void
unretire_user(UR_OBJECT user)
{
    UR_OBJECT u;
    int on;

    if (word_count < 2) {
        write_user(user, "Usage: unretire <user>\n");
        return;
    }
    if (!is_retired(word[1])) {
        vwrite_user(user, "%s has not been retired from the wizlist.\n", word[1]);
        return;
    }
    u = retrieve_user(user, word[1]);
    if (!u) {
        return;
    }
    on = retrieve_user_type == 1;
    if (u == user) {
        write_user(user, "You cannot unretire yourself.\n");
        return;
    }
    if (u->level < WIZ) {
        write_user(user, "You cannot retire anyone under the level WIZ.\n");
        return;
    }
    u->retired = 0;
    clean_retire_list(u->name);
    vwrite_user(user, "You unretire %s and put them back on the wizlist.\n",
            u->name);
    write_syslog(SYSLOG, 1, "%s UNRETIRED %s\n", user->name, u->name);
    sprintf(text, "Was ~FGunretired~RS by %s.\n", user->name);
    add_history(u->name, 1, "%s", text);
    sprintf(text, "You have been unretired and put back on the wizlist.\n");
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