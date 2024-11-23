
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Site a user
 */
void
site(UR_OBJECT user)
{
    UR_OBJECT u;

    if (word_count < 2) {
        write_user(user, "Usage: site <user>\n");
        return;
    }
    u = retrieve_user(user, word[1]);
    if (!u) {
        return;
    }
#ifdef NETLINKS
    /* if the user is remotely connected */
    if (u->type == REMOTE_TYPE) {
        vwrite_user(user, "%s~RS is remotely connected from %s.\n", u->recap,
                u->site);
        done_retrieve(u);
        return;
    }
#endif
    if (retrieve_user_type == 1) {
        vwrite_user(user, "%s~RS is logged in from ~OL~FC%s~RS (%s:%s).\n",
                u->recap, u->site, u->ipsite, u->site_port);
    } else {
        vwrite_user(user, "%s~RS was last logged in from ~OL~FC%s~RS.\n",
                u->recap, u->last_site);
    }
    done_retrieve(u);
}
