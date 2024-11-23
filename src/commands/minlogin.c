
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Set minlogin level
 */
void
minlogin(UR_OBJECT user)
{
    static const char usage[] = "Usage: minlogin -a|-n|<level>\n";
    const char *levstr;
    const char *name;
    UR_OBJECT u, next;
    int cnt;
    enum lvl_value lvl;

    if (word_count < 2) {
        write_user(user, usage);
        if (amsys->stop_logins) {
            write_user(user, "Currently set to ALL\n");
        } else if (amsys->minlogin_level == NUM_LEVELS) {
            write_user(user, "Currently set to NONE\n");
        } else {
            vwrite_user(user, "Currently set to %s\n",
                    user_level[amsys->minlogin_level].name);
        }
        return;
    }
    if (!strcasecmp(word[1], "-a")) {
        amsys->stop_logins = 1;
        write_user(user, "You have now stopped all logins on the user port.\n");
        return;
    }
    if (!strcasecmp(word[1], "-n")) {
        amsys->stop_logins = 0;
        amsys->minlogin_level = NUM_LEVELS;
        write_user(user, "You have now removed the minlogin level.\n");
        return;
    }
    lvl = get_level(word[1]);
    if (lvl == NUM_LEVELS) {
        write_user(user, usage);
        if (amsys->stop_logins) {
            write_user(user, "Currently set to ALL\n");
        } else if (amsys->minlogin_level == NUM_LEVELS) {
            write_user(user, "Currently set to NONE\n");
        } else {
            vwrite_user(user, "Currently set to %s\n",
                    user_level[amsys->minlogin_level].name);
        }
        return;
    } else {
        levstr = user_level[lvl].name;
    }
    if (lvl > user->level) {
        write_user(user,
                "You cannot set minlogin to a higher level than your own.\n");
        return;
    }
    if (amsys->minlogin_level == lvl) {
        write_user(user, "It is already set to that level.\n");
        return;
    }
    amsys->minlogin_level = lvl;
    vwrite_user(user, "Minlogin level set to: ~OL%s.\n", levstr);
    name = user->vis ? user->name : invisname;
    vwrite_room_except(NULL, user, "%s has set the minlogin level to: ~OL%s.\n",
            name, levstr);
    write_syslog(SYSLOG, 1, "%s set the minlogin level to %s.\n", user->name,
            levstr);
    /* Now boot off anyone below that level */
    cnt = 0;
    if (amsys->boot_off_min) {
        for (u = user_first; u; u = next) {
            next = u->next;
            if (!u->login && u->type != CLONE_TYPE && u->level < lvl) {
                write_user(u,
                        "\n~FY~OLYour level is now below the minlogin level, disconnecting you...\n");
                disconnect_user(u);
                ++cnt;
            }
        }
    }
    if (cnt) {
        vwrite_user(user, "Total of ~OL%d~RS users were disconnected.\n", cnt);
    }
    destructed = 0;
}