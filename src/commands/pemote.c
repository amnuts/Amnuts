
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Do a private emote
 */
void
pemote(UR_OBJECT user, char *inpstr)
{
    static const char usage[] = "Usage: pemote <user> <text>\n";
    const char *name;
    UR_OBJECT u;

    /* FIXME: Use sentinel other JAILED */
    if (user->muzzled != JAILED) {
        write_user(user, "You are muzzled, you cannot emote.\n");
        return;
    }
    if (word_count < 3 && !strchr("</", *inpstr)) {
        write_user(user, usage);
        return;
    }
    if (word_count < 2 && strchr("</", *inpstr)) {
        write_user(user, usage);
        return;
    }
    u = get_user_name(user, word[1]);
    if (!u) {
        write_user(user, notloggedon);
        return;
    }
    if (u == user) {
        write_user(user, "Talking to yourself is the first sign of madness.\n");
        return;
    }
    if (check_igusers(u, user) && user->level < GOD) {
        vwrite_user(user, "%s~RS is ignoring private emotes from you.\n",
                u->recap);
        return;
    }
    if (u->igntells && (user->level < WIZ || u->level > user->level)) {
        vwrite_user(user, "%s~RS is ignoring private emotes at the moment.\n",
                u->recap);
        return;
    }
    inpstr = remove_first(inpstr);
    switch (amsys->ban_swearing) {
    case SBMAX:
        if (contains_swearing(inpstr)) {
            write_user(user, noswearing);
            return;
        }
        break;
    case SBMIN:
        inpstr = censor_swear_words(inpstr);
        break;
    case SBOFF:
    default:
        /* do nothing as ban_swearing is off */
        break;
    }
    name = user->vis || u->level >= user->level ? user->recap : invisname;
    sprintf(text, "~OL~FG>~RS %s~RS%s%s\n", name, *inpstr != '\'' ? " " : "",
            inpstr);
    if (u->afk) {
        record_afk(user, u, text);
        if (*u->afk_mesg) {
            vwrite_user(user, "%s~RS is ~FRAFK~RS, message is: %s\n", u->recap,
                    u->afk_mesg);
        } else {
            vwrite_user(user, "%s~RS is ~FRAFK~RS at the moment.\n", u->recap);
        }
        write_user(user, "Sending message to their afk review buffer.\n");
        return;
    }
    if (u->malloc_start) {
        record_edit(user, u, text);
        vwrite_user(user,
                "%s~RS is in ~FCEDIT~RS mode at the moment (using the line editor).\n",
                u->recap);
        write_user(user, "Sending message to their edit review buffer.\n");
        return;
    }
    if (u->ignall && (user->level < WIZ || u->level > user->level)) {
        vwrite_user(user, "%s~RS is ignoring everyone at the moment.\n",
                u->recap);
        return;
    }
#ifdef NETLINKS
    if (!u->room) {
        vwrite_user(user,
                "%s~RS is offsite and would not be able to reply to you.\n",
                u->recap);
        return;
    }
#endif
    record_tell(user, u, text);
    write_user(u, text);
    sprintf(text, "~FG~OL>~RS (%s~RS) %s~RS%s%s\n", u->recap, name,
            *inpstr != '\'' ? " " : "", inpstr);
    record_tell(user, user, text);
    write_user(user, text);
}
