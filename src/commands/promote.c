
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Promote a user
 */
void
promote(UR_OBJECT user)
{
    UR_OBJECT u;
    int on;
    enum lvl_value lvl;

    if (word_count < 2) {
        write_user(user, "Usage: promote <user> [<level>]\n");
        return;
    }
    if (word_count > 3) {
        write_user(user, "Usage: promote <user> [<level>]\n");
        return;
    }
    u = retrieve_user(user, word[1]);
    if (!u) {
        return;
    }
    on = retrieve_user_type == 1;
    /* FIXME: lose .tpromote if .promote fails */
    /* first, gotta reset the user level if they have been temp promoted */
    if (u->real_level < u->level) {
        u->level = u->real_level;
    }
    /* cannot promote jailed users */
    if (u->level == JAILED) {
        vwrite_user(user, "You cannot promote a user of level %s.\n",
                user_level[JAILED].name);
        done_retrieve(u);
        return;
    }
    if (word_count < 3) {
        lvl = (enum lvl_value) (u->level + 1);
    } else {
        strtoupper(word[2]);
        lvl = get_level(word[2]);
        if (lvl == NUM_LEVELS) {
            vwrite_user(user, "You must select a level between %s and %s.\n",
                    user_level[USER].name, user_level[GOD].name);
            done_retrieve(u);
            return;
        }
        if (lvl <= u->level) {
            write_user(user,
                    "You cannot promote a user to a level less than or equal to what they are now.\n");
            done_retrieve(u);
            return;
        }
    }
    if (user->level < lvl) {
        write_user(user,
                "You cannot promote a user to a level higher than your own.\n");
        done_retrieve(u);
        return;
    }
    /* do it */
    u->level = lvl;
    u->unarrest = u->level;
    u->real_level = u->level;
    user_list_level(u->name, u->level);
    strcpy(u->date, (long_date(1)));
    u->accreq = -1;
    u->real_level = u->level;
    strcpy(u->date, long_date(1));
    add_user_date_node(u->name, (long_date(1)));
    sprintf(text, "~FG~OLYou have been promoted to level: ~RS~OL%s.\n",
            user_level[u->level].name);
    if (!on) {
        send_mail(user, u->name, text, 0);
    } else {
        write_user(u, text);
    }
    vwrite_level(u->level, 1, NORECORD, u,
            "~FG~OL%s is promoted to level: ~RS~OL%s.\n", u->bw_recap,
            user_level[u->level].name);
    write_syslog(SYSLOG, 1, "%s PROMOTED %s to level %s.\n", user->name,
            u->name, user_level[u->level].name);
    sprintf(text, "Was ~FGpromoted~RS by %s to level %s.\n", user->name,
            user_level[u->level].name);
    add_history(u->name, 1, "%s", text);
    if (!on) {
        u->socket = -2;
        strcpy(u->site, u->last_site);
    }
    save_user_details(u, on);
    done_retrieve(u);
}