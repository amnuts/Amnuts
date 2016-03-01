
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Set the room topic
 */
void
set_topic(UR_OBJECT user, char *inpstr)
{
    RM_OBJECT rm;
    const char *name;

    rm = user->room;
    if (word_count < 2) {
        if (!*rm->topic) {
            write_user(user, "No topic has been set yet.\n");
            return;
        }
        vwrite_user(user, "The current topic is: %s\n", rm->topic);
        return;
    }
    if (strlen(inpstr) > TOPIC_LEN) {
        write_user(user, "Topic too long.\n");
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
        if (!is_personal_room(user->room)) {
            inpstr = censor_swear_words(inpstr);
        }
        break;
    case SBOFF:
    default:
        /* do nothing as ban_swearing is off */
        break;
    }
    vwrite_user(user, "Topic set to: %s\n", inpstr);
    name = user->vis ? user->recap : invisname;
    vwrite_room_except(rm, user, "%s~RS has set the topic to: %s\n", name,
            inpstr);
    strcpy(rm->topic, inpstr);
}
