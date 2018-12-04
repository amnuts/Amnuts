
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * ask all the law, (sos), no muzzle restrictions
 */
void
plead(UR_OBJECT user, char *inpstr)
{
    static const char usage[] = "Usage: sos <text>\n";
    UR_OBJECT u;
    const char *type;

    if (word_count < 2) {
        write_user(user, usage);
        return;
    }
    if (user->level >= WIZ) {
        write_user(user, "You are already a wizard!\n");
        return;
    }
    for (u = user_first; u; u = u->next) {
        if (u->level >= WIZ && !u->login) {
            break;
        }
    }
    if (!u) {
        write_user(user, "Sorry, but there are no wizzes currently logged on.\n");
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
        type = "plead";
    }
    vwrite_level(WIZ, 1, RECORD, user,
            "~OL~FG>~RS [~FRSOS~RS] %s~RS ~OL%ss~RS: %s\n", user->recap,
            type, inpstr);
    sprintf(text, "~OL~FG>~RS [~FRSOS~RS] You ~OL%s~RS: %s\n", type, inpstr);
    record_tell(user, user, text);
    write_user(user, text);
}
