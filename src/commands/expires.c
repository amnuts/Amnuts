
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Set a user to either expire after a set time, or never expire
 */
void
user_expires(UR_OBJECT user)
{
    UR_OBJECT u;

    if (word_count < 2) {
        write_user(user, "Usage: expire <user>\n");
        return;
    }
    u = retrieve_user(user, word[1]);
    if (!u) {
        return;
    }
    /* process */
    if (!u->expire) {
        u->expire = 1;
        vwrite_user(user,
                "You have set it so %s will expire when a purge is run.\n",
                u->name);
        sprintf(text, "%s enables expiration with purge.\n", user->name);
        add_history(u->name, 0, "%s", text);
        write_syslog(SYSLOG, 1, "%s enabled expiration on %s.\n", user->name,
                u->name);
    } else {
        u->expire = 0;
        vwrite_user(user,
                "You have set it so %s will no longer expire when a purge is run.\n",
                u->name);
        sprintf(text, "%s disables expiration with purge.\n", user->name);
        add_history(u->name, 0, "%s", text);
        write_syslog(SYSLOG, 1, "%s disabled expiration on %s.\n", user->name,
                u->name);
    }
    save_user_details(u, retrieve_user_type == 1);
    done_retrieve(u);
}
