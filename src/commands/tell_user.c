#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Tell another user something
 */
void
tell_user(UR_OBJECT user, char *inpstr)
{
    static const char usage[] = "Usage: tell <user> <text>\n";
    static const char qcusage[] = "Usage: ,<text>\n";
    const char *type;
    const char *name;
    UR_OBJECT u;

    /* FIXME: Use sentinel other JAILED */
    if (user->muzzled != JAILED) {
        write_user(user, "You are muzzled, you cannot speak.\n");
        return;
    }
    /* determine whether this is a quick call */
    if (*inpstr == ',') {
        if (!*user->call) {
            write_user(user, "Quick call not set.\n");
            return;
        }
        u = get_user_name(user, user->call);
        /* if quick call with no message */
        if (word_count < 2) {
            write_user(user, qcusage);
            return;
        }
        inpstr = remove_first(inpstr);
    } else {
        /* if tell by itself, review tells */
        if (word_count < 2) {
            revtell(user);
            return;
        }
        u = get_user_name(user, word[1]);
        /* if tell <user> with no message */
        if (word_count < 3) {
            write_user(user, usage);
            return;
        }
        inpstr = remove_first(inpstr);
    }
    if (!u) {
        write_user(user, notloggedon);
        return;
    }
    if (u == user) {
        write_user(user, "Talking to yourself is the first sign of madness.\n");
        return;
    }
    if (check_igusers(u, user) && user->level < GOD) {
        vwrite_user(user, "%s~RS is ignoring tells from you.\n", u->recap);
        return;
    }
    if (u->igntells && (user->level < WIZ || u->level > user->level)) {
        vwrite_user(user, "%s~RS is ignoring tells at the moment.\n", u->recap);
        return;
    }
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
    type = smiley_type(inpstr);
    if (!type) {
        type = "say";
    }
    name = user->vis || u->level >= user->level ? user->recap : invisname;
    sprintf(text, "~OL~FG>~RS %s~RS ~FC%ss~RS: %s\n", name, type, inpstr);
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
    sprintf(text, "~OL~FG>~RS (%s~RS) You ~FC%s~RS: %s\n", u->recap, type,
            inpstr);
    record_tell(user, user, text);
    write_user(user, text);
}
