#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Reloads the description for one or all rooms--incase you have edited the
 * file and do not want to reboot the talker to to make the changes displayed
 */
void
reload_room_description(UR_OBJECT user)
{
    int c, i, error;
    RM_OBJECT rm;
    char filename[80];
    FILE *fp;

    if (word_count < 2) {
        write_user(user, "Usage: rloadrm -a/<room name>\n");
        return;
    }
    /* if reload all of the rooms */
    if (!strcmp(word[1], "-a")) {
        error = 0;
        for (rm = room_first; rm; rm = rm->next) {
            if (is_personal_room(rm)) {
                continue;
            }
            sprintf(filename, "%s/%s.R", DATAFILES, rm->name);
            fp = fopen(filename, "r");
            if (!fp) {
                vwrite_user(user,
                        "Sorry, cannot reload the description file for the room \"%s\".\n",
                        rm->name);
                write_syslog(SYSLOG | ERRLOG, 0,
                        "ERROR: Cannot reload the description file for room %s.\n",
                        rm->name);
                ++error;
                continue;
            }
            i = 0;
            for (c = getc(fp); c != EOF; c = getc(fp)) {
                if (i == ROOM_DESC_LEN) {
                    break;
                }
                rm->desc[i++] = c;
            }
            if (c != EOF) {
                vwrite_user(user,
                        "The description is too long for the room \"%s\".\n",
                        rm->name);
                write_syslog(SYSLOG | ERRLOG, 0,
                        "ERROR: Description too long when reloading for room %s.\n",
                        rm->name);
            }
            rm->desc[i] = '\0';
            fclose(fp);
        }
        if (!error) {
            write_user(user, "You have now reloaded all room descriptions.\n");
        } else {
            write_user(user,
                    "You have now reloaded all room descriptions that you can.\n");
        }
        write_syslog(SYSLOG, 1, "%s reloaded all of the room descriptions.\n",
                user->name);
        return;
    }
    /* if it is just one room to reload */
    rm = get_room(word[1]);
    if (!rm) {
        write_user(user, nosuchroom);
        return;
    }
    /* check first for personal room, and do not reload */
    if (is_personal_room(rm)) {
        write_user(user,
                "Sorry, but you cannot reload personal room descriptions.\n");
        return;
    }
    sprintf(filename, "%s/%s.R", DATAFILES, rm->name);
    fp = fopen(filename, "r");
    if (!fp) {
        vwrite_user(user,
                "Sorry, cannot reload the description file for the room \"%s\".\n",
                rm->name);
        write_syslog(SYSLOG | ERRLOG, 0,
                "ERROR: Cannot reload the description file for room %s.\n",
                rm->name);
        return;
    }
    i = 0;
    for (c = getc(fp); c != EOF; c = getc(fp)) {
        if (i == ROOM_DESC_LEN) {
            break;
        }
        rm->desc[i++] = c;
    }
    if (c != EOF) {
        vwrite_user(user, "The description is too long for the room \"%s\".\n",
                rm->name);
        write_syslog(SYSLOG | ERRLOG, 0,
                "ERROR: Description too long when reloading for room %s.\n",
                rm->name);
    }
    rm->desc[i] = '\0';
    fclose(fp);
    vwrite_user(user,
            "You have now reloaded the description for the room \"%s\".\n",
            rm->name);
    write_syslog(SYSLOG, 1, "%s reloaded the description for the room %s\n",
            user->name, rm->name);
}
