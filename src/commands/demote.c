
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Demote a user
 */
void
demote(UR_OBJECT user)
{
    UR_OBJECT u;
    int on;
    enum lvl_value lvl;

    if (word_count < 2) {
        write_user(user, "Usage: demote <user> [<level>]\n");
        return;
    }
    if (word_count > 3) {
        write_user(user, "Usage: demote <user> [<level>]\n");
        return;
    }
    u = retrieve_user(user, word[1]);
    if (!u) {
        return;
    }
    on = retrieve_user_type == 1;
    /* FIXME: lose .tpromote if .demote fails */
    /* first, gotta reset the user level if they have been temp promoted */
    if (u->real_level < u->level) {
        u->level = u->real_level;
    }
    /* cannot demote new or jailed users */
    if (u->level <= NEW) {
        vwrite_user(user, "You cannot demote a user of level %s or %s.\n",
                user_level[NEW].name, user_level[JAILED].name);
        done_retrieve(u);
        return;
    }
    if (word_count < 3) {
        lvl = (enum lvl_value) (u->level - 1);
    } else {
        strtoupper(word[2]);
        lvl = get_level(word[2]);
        if (lvl == NUM_LEVELS) {
            vwrite_user(user, "You must select a level between %s and %s.\n",
                    user_level[NEW].name, user_level[ARCH].name);
            done_retrieve(u);
            return;
        }
        /* now runs checks if level option was used */
        if (lvl >= u->level) {
            write_user(user,
                    "You cannot demote a user to a level higher than or equal to what they are now.\n");
            done_retrieve(u);
            return;
        }
        if (lvl == JAILED) {
            vwrite_user(user, "You cannot demote a user to the level %s.\n",
                    user_level[JAILED].name);
            done_retrieve(u);
            return;
        }
    }
    if (u->level >= user->level) {
        write_user(user,
                "You cannot demote a user of an equal or higher level than yourself.\n");
        done_retrieve(u);
        return;
    }
    /* do it */
    if (u->level == WIZ) {
        clean_retire_list(u->name);
    }
    u->level = lvl;
    u->unarrest = u->level;
    u->real_level = u->level;
    user_list_level(u->name, u->level);
    u->vis = 1;
    strcpy(u->date, long_date(1));
    add_user_date_node(u->name, (long_date(1)));
    sprintf(text, "~FR~OLYou have been demoted to level: ~RS~OL%s.\n",
            user_level[u->level].name);
    if (!on) {
        send_mail(user, u->name, text, 0);
    } else {
        write_user(u, text);
    }
    vwrite_level(u->level, 1, NORECORD, u,
            "~FR~OL%s is demoted to level: ~RS~OL%s.\n", u->bw_recap,
            user_level[u->level].name);
    write_syslog(SYSLOG, 1, "%s DEMOTED %s to level %s.\n", user->name, u->name,
            user_level[u->level].name);
    sprintf(text, "Was ~FRdemoted~RS by %s to level %s.\n", user->name,
            user_level[u->level].name);
    add_history(u->name, 1, "%s", text);
    if (!on) {
        u->socket = -2;
        strcpy(u->site, u->last_site);
    }
    save_user_details(u, on);
    done_retrieve(u);
}
