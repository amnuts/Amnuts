
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Shout something to other wizes and gods. If the level isnt given it
 * defaults to WIZ level.
 */
void
wizshout(UR_OBJECT user, char *inpstr)
{
    static const char usage[] = "Usage: twiz [<level>] <text>\n";
    const char *type;
    enum lvl_value lev;

    /* FIXME: Use sentinel other JAILED */
    if (user->muzzled != JAILED) {
        write_user(user, "You are muzzled, you cannot speak.\n");
        return;
    }
    if (word_count < 2) {
        write_user(user, usage);
        return;
    }
    strtoupper(word[1]);
    lev = get_level(word[1]);
    if (lev == NUM_LEVELS) {
        lev = WIZ;
    } else {
        if (lev < WIZ || word_count < 3) {
            write_user(user, usage);
            return;
        }
        if (lev > user->level) {
            write_user(user,
                    "You cannot specifically shout to users of a higher level than yourself.\n");
            return;
        }
        inpstr = remove_first(inpstr);
    }
    /* Even wizzes cannot escapde the swear ban! MWHAHahaha.... ahem. */
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
    vwrite_level(lev, 1, RECORD, user,
            "~OL~FG>~RS [~FY%s~RS] %s~RS ~FY%ss~RS: %s\n",
            user_level[lev].name, user->recap, type, inpstr);
    sprintf(text, "~OL~FG>~RS [~FY%s~RS] You ~FY%s~RS: %s\n",
            user_level[lev].name, type, inpstr);
    record_tell(user, user, text);
    write_user(user, text);
}
