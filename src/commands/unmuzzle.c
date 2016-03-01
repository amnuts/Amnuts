
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Unmuzzle the bastard now he has apologised and grovelled enough via email
 */
void
unmuzzle(UR_OBJECT user)
{
    UR_OBJECT u;
    int on;

    if (word_count < 2) {
        write_user(user, "Usage: unmuzzle <user>\n");
        return;
    }
    u = retrieve_user(user, word[1]);
    if (!u) {
        return;
    }
    on = retrieve_user_type == 1;
    /* error checks */
    if (u == user) {
        write_user(user,
                "Trying to unmuzzle yourself is the tenth sign of madness.\n");
        done_retrieve(u);
        return;
    }
    /* FIXME: Use sentinel other JAILED */
    if (u->muzzled == JAILED) {
        vwrite_user(user, "%s~RS is not muzzled.\n", u->recap);
        done_retrieve(u);
        return;
    }
    if (u->muzzled > user->level) {
        vwrite_user(user,
                "%s~RS's muzzle is set to level %s, you do not have the power to remove it.\n",
                u->recap, user_level[u->muzzled].name);
        done_retrieve(u);
        return;
    }
    /* do the unmuzzle */
    u->muzzled = JAILED; /* FIXME: Use sentinel other JAILED */
    vwrite_user(user, "~FG~OLYou remove %s~RS's muzzle.\n", u->recap);
    write_syslog(SYSLOG, 1, "%s unmuzzled %s.\n", user->name, u->name);
    add_history(u->name, 0, "~FGUnmuzzled~RS by %s, level %d (%s).\n",
            user->name, user->level, user_level[user->level].name);
    sprintf(text, "~FG~OLYou have been unmuzzled!\n");
    if (!on) {
        send_mail(user, u->name, text, 0);
    } else {
        write_user(u, text);
    }
    /* finish up */
    if (!on) {
        strcpy(u->site, u->last_site);
        u->socket = -2;
    }
    save_user_details(u, on);
    done_retrieve(u);
}