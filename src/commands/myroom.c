#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * lets a user enter their own room.  It creates the room if !exists
 */
void
personal_room(UR_OBJECT user)
{
    char rmname[ROOM_NAME_LEN + 1], filename[80];
    UR_OBJECT u;
    RM_OBJECT rm;

    if (!amsys->personal_rooms) {
        write_user(user, "Personal room functions are currently disabled.\n");
        return;
    }
    sprintf(rmname, "(%s)", user->name);
    strtolower(rmname);
    rm = get_room_full(rmname);
    /* if the user wants to delete their room */
    if (word_count >= 2) {
        if (strcmp(word[1], "-d")) {
            write_user(user, "Usage: myroom [-d]\n");
            return;
        }
        /* move to the user out of the room if they are in it */
        if (!rm) {
            write_user(user, "You do not have a personal room built.\n");
            return;
        }
        if (room_visitor_count(rm)) {
            write_user(user,
                    "You cannot destroy your room if any people are in it.\n");
            return;
        }
        write_user(user,
                "~OL~FRYou whistle a sharp spell and watch your room crumble into dust.\n");
        /* remove invites */
        for (u = user_first; u; u = u->next) {
            if (u->invite_room == rm) {
                u->invite_room = NULL;
            }
        }
        /* remove all the key flags */
        write_user(user, "~OL~FRAll keys to your room crumble to ashes.\n");
        all_unsetbit_flagged_user_entry(user, fufROOMKEY);
        /* destroy */
        destruct_room(rm);
        /* delete the files */
        sprintf(filename, "%s/%s/%s.R", USERFILES, USERROOMS, user->name);
        remove(filename);
        sprintf(filename, "%s/%s/%s.B", USERFILES, USERROOMS, user->name);
        remove(filename);
        sprintf(filename, "%s/%s/%s.K", USERFILES, USERROOMS, user->name);
        remove(filename);
        write_syslog(SYSLOG, 1, "%s destructed their personal room.\n",
                user->name);
        return;
    }
    /* if the user is moving to their room */
    if (user->lroom == 2) {
        write_user(user, "You have been shackled and cannot move.\n");
        return;
    }
    /* if room does not exist then create it */
    if (!rm) {
        rm = create_room();
        if (!rm) {
            write_user(user,
                    "Sorry, but your room cannot be created at this time.\n");
            write_syslog(SYSLOG | ERRLOG, 0,
                    "ERROR: Cannot create room for in personal_room()\n");
            return;
        }
        write_user(user, "\nYour room does not exists. Building it now...\n\n");
        /* check to see if the room was just unloaded from memory first */
        if (!personal_room_store(user->name, 0, rm)) {
            write_syslog(SYSLOG, 1, "%s creates their own room.\n", user->name);
            if (!personal_room_store(user->name, 1, rm)) {
                write_syslog(SYSLOG | ERRLOG, 1,
                        "ERROR: Unable to save personal room status in personal_room()\n");
            }
        }
    }
    /* if room just created then should not go in his block */
    if (user->room == rm) {
        write_user(user, "You are already in your own room!\n");
        return;
    }
    move_user(user, rm, 1);
}