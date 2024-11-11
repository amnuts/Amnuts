#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"
#ifndef __SDS_H
#include "../vendors/sds/sds.h"
#endif

/*
 * this function allows admin to control personal rooms
 */
void
personal_room_admin(UR_OBJECT user)
{
    sds rmname, filename;
    RM_OBJECT rm;
    UR_OBJECT u;
    int trsize, rmcount, locked, unlocked;

    if (word_count < 2) {
        write_user(user, "Usage: rmadmin -l / -m / -u <name> / -d <name>\n");
        return;
    }
    if (!amsys->personal_rooms) {
        write_user(user, "Personal room functions are currently disabled.\n");
        return;
    }
    strtolower(word[1]);
    /* just display the amount of memory used by personal rooms */
    if (!strcmp(word[1], "-m")) {
        write_user(user,
                "+----------------------------------------------------------------------------+\n");
        write_user(user,
                "| ~FC~OLPersonal Room Memory Usage~RS                                                 |\n");
        write_user(user,
                "+----------------------------------------------------------------------------+\n");
        rmcount = locked = unlocked = 0;
        for (rm = room_first; rm; rm = rm->next) {
            if (!is_personal_room(rm)) {
                continue;
            }
            ++rmcount;
            if (is_private_room(rm)) {
                ++locked;
            } else {
                ++unlocked;
            }
        }
        trsize = rmcount * (sizeof *rm);
        vwrite_user(user,
                "| %-15.15s: ~OL%2d~RS locked, ~OL%2d~RS unlocked                                    |\n",
                "status", locked, unlocked);
        vwrite_user(user,
                "| %-15.15s: ~OL%5d~RS * ~OL%8d~RS bytes = ~OL%8d~RS total bytes  (%02.3f Mb) |\n",
                "rooms", rmcount, (int) (sizeof *rm), trsize,
                trsize / 1048576.0);
        write_user(user,
                "+----------------------------------------------------------------------------+\n");
        return;
    }
    /* list all the personal rooms in memory together with status */
    if (!strcmp(word[1], "-l")) {
        write_user(user,
                "+----------------------------------------------------------------------------+\n");
        write_user(user,
                "| ~OL~FCPersonal Room Listings~RS                                                     |\n");
        write_user(user,
                "+----------------------------------------------------------------------------+\n");
        rmcount = 0;
        for (rm = room_first; rm; rm = rm->next) {
            if (!is_personal_room(rm)) {
                continue;
            }
            ++rmcount;
            vwrite_user(user,
                    "| Owner : ~OL%-*.*s~RS   Status : ~OL%s~RS   Msg Count : ~OL%2d~RS  People : ~OL%2d~RS |\n",
                    USER_NAME_LEN, USER_NAME_LEN, rm->owner,
                    is_private_room(rm) ? "~FRlocked  " : "~FGunlocked",
                    rm->mesg_cnt, room_visitor_count(rm));
        }
        if (!rmcount) {
            write_user(user,
                    "| ~FRNo personal rooms are currently in memory~RS                                  |\n");
        }
        write_user(user,
                "+----------------------------------------------------------------------------+\n");
        if (rmcount) {
            vwrite_user(user,
                    "| Total personal rooms : ~OL~FM%2d~RS                                                  |\n",
                    rmcount);
            write_user(user,
                    "+----------------------------------------------------------------------------+\n");
        }
        return;
    }
    /* unload a room from memory or delete it totally - all rooms files */
    if (!strcmp(word[1], "-u") || !strcmp(word[1], "-d")) {
        if (word_count < 3) {
            write_user(user, "Usage: rmadmin -l / -m / -u <name> / -d <name>\n");
            return;
        }
        rmname = sdscatfmt(sdsempty(), "(%s)", word[2]);
        strtolower(rmname);
        /* first do checks on the room */
        rm = get_room_full(rmname);
        sdsfree(rmname);
        if (!rm) {
            write_user(user, "That user does not have a personal room built.\n");
            return;
        }
        if (room_visitor_count(rm)) {
            write_user(user, "You cannot remove a room if people are in it.\n");
            return;
        }
        /* okay to remove the room */
        /* remove invites */
        for (u = user_first; u; u = u->next) {
            if (u->invite_room == rm) {
                u->invite_room = NULL;
            }
        }
        /* destroy */
        destruct_room(rm);
        strtolower(word[2]);
        *word[2] = toupper(*word[2]);
        /* delete all files */
        if (!strcmp(word[1], "-d")) {
            filename = sdscatfmt(sdsempty(), "%s/%s/%s.R", USERFILES, USERROOMS, word[2]);
            remove(filename);
            filename = sdscatfmt(sdsempty(), "%s/%s/%s.B", USERFILES, USERROOMS, word[2]);
            remove(filename);
            sdsfree(filename);
            write_syslog(SYSLOG, 1, "%s deleted the personal room of %s.\n",
                    user->name, word[2]);
            vwrite_user(user,
                    "You have now ~OL~FRdeleted~RS the room belonging to %s.\n",
                    word[2]);
            /* remove all the key flags */
            u = retrieve_user(user, word[2]);
            if (u) {
                all_unsetbit_flagged_user_entry(u, fufROOMKEY);
                write_user(user, "All keys to that room have now been destroyed.\n");
                done_retrieve(u);
            }
        } else {
            /* just unload from memory */
            write_syslog(SYSLOG, 1,
                    "%s unloaded the personal room of %s from memory.\n",
                    user->name, word[2]);
            vwrite_user(user,
                    "You have now ~OL~FGunloaded~RS the room belonging to %s from memory.\n",
                    word[2]);
        }
        return;
    }
    /* wrong input given */
    write_user(user, "Usage: rmadmin -l | -m | -u <name> | -d <name>\n");
}
