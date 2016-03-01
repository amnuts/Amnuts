
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * clears a room topic
 */
void
clear_topic(UR_OBJECT user)
{
    RM_OBJECT rm;
    const char *name;

    if (word_count < 2) {
        rm = user->room;
        *rm->topic = '\0';
        write_user(user, "Topic has been cleared\n");
        name = user->vis ? user->recap : invisname;
        vwrite_room_except(rm, user, "%s~RS ~FY~OLhas cleared the topic.\n",
                name);
        return;
    }
    strtolower(word[1]);
    if (!strcmp(word[1], "all")) {
        if (user->level > (enum lvl_value) command_table[CTOPIC].level
                || user->level >= ARCH) {
            for (rm = room_first; rm; rm = rm->next) {
                *rm->topic = '\0';
                write_room_except(rm, "\n~FY~OLThe topic has been cleared.\n", user);
            }
            write_user(user, "All room topics have now been cleared\n");
            return;
        }
        write_user(user,
                "You can only clear the topic of the room you are in.\n");
        return;
    }
    write_user(user, "Usage: ctopic [all]\n");
}