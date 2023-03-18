
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Muzzle an annoying user so he cant speak, emote, echo, write, smail
 * or bcast. Muzzles have levels from WIZ to GOD so for instance a wiz
 * cannot remove a muzzle set by a god
 */
void
muzzle(UR_OBJECT user)
{
    UR_OBJECT u;
    int on;

    if (word_count < 2) {
        write_user(user, "Usage: muzzle <user>\n");
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
                "Trying to muzzle yourself is the ninth sign of madness.\n");
        return;
    }
    if (u->level >= user->level) {
        write_user(user,
                "You cannot muzzle a user of equal or higher level than yourself.\n");
        done_retrieve(u);
        return;
    }
    if (u->muzzled >= user->level) {
        vwrite_user(user, "%s~RS is already muzzled.\n", u->recap);
        done_retrieve(u);
        return;
    }
    /* do the muzzle */
    u->muzzled = user->level;
    vwrite_user(user, "~FR~OL%s now has a muzzle of level: ~RS~OL%s.\n",
            u->bw_recap, user_level[user->level].name);
    write_syslog(SYSLOG, 1, "%s muzzled %s (level %d).\n", user->name, u->name,
            user->level);
    add_history(u->name, 1, "Level %d (%s) ~FRmuzzle~RS put on by %s.\n",
            user->level, user_level[user->level].name, user->name);
    sprintf(text, "~FR~OLYou have been muzzled!\n");
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