/****************************************************************************
             Amnuts - Copyright (C) Andrew Collington, 1996-2026

                   talker@amnuts.net - https://amnuts.net/

                                 based on

   NUTS version 3.3.3 (Triple Three :) - Copyright (C) Neil Robertson 1996
 ***************************************************************************/

#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Reload the descriptions of one or all rooms from rooms.yaml.
 *
 * Useful when an admin has edited rooms.yaml on disk and wants the
 * change to take effect without a full reboot. Only descriptions are
 * refreshed; topic, access flags, links, and netlink wiring are left
 * as they currently sit in memory so runtime mutations (.topic,
 * .private, etc.) are preserved.
 *
 * Usage: rloadrm -a            -- reload every non-personal room
 *        rloadrm <room name>   -- reload one room by name
 *
 * Personal rooms are skipped because their descriptions live in
 * files/userfiles/rooms/ rather than in rooms.yaml.
 *
 * If rooms.yaml is missing or malformed, the command reports the
 * error to the user and leaves in-memory state untouched -- it does
 * NOT terminate the talker, unlike the boot-time loader.
 */
void
reload_room_description(UR_OBJECT user)
{
    char errbuf[256] = "";
    int n;
    const char *path = DATAFILES "/rooms.yaml";

    if (word_count < 2) {
        write_user(user, "Usage: rloadrm -a/<room name>\n");
        return;
    }
    if (!strcmp(word[1], "-a")) {
        n = yaml_reload_descriptions(path, NULL, errbuf, sizeof errbuf);
        if (n < 0) {
            vwrite_user(user, "Sorry, cannot reload room descriptions.\n");
            if (*errbuf) {
                vwrite_user(user, "  %s\n", errbuf);
            }
            write_syslog(SYSLOG | ERRLOG, 0,
                    "ERROR: rloadrm -a failed: %s\n",
                    *errbuf ? errbuf : "unknown");
            return;
        }
        vwrite_user(user, "Reloaded descriptions for %d room%s.\n",
                n, PLTEXT_S(n));
        write_syslog(SYSLOG, 1,
                "%s reloaded all of the room descriptions.\n", user->name);
        return;
    }
    /* Single-room form. */
    RM_OBJECT rm = get_room(word[1]);
    if (!rm) {
        write_user(user, nosuchroom);
        return;
    }
    if (is_personal_room(rm)) {
        write_user(user,
                "Sorry, but you cannot reload personal room descriptions.\n");
        return;
    }
    n = yaml_reload_descriptions(path, rm->name, errbuf, sizeof errbuf);
    if (n < 0) {
        vwrite_user(user,
                "Sorry, cannot reload the description for the room \"%s\".\n",
                rm->name);
        if (*errbuf) {
            vwrite_user(user, "  %s\n", errbuf);
        }
        write_syslog(SYSLOG | ERRLOG, 0,
                "ERROR: rloadrm %s failed: %s\n",
                rm->name, *errbuf ? errbuf : "unknown");
        return;
    }
    if (n == 0) {
        vwrite_user(user,
                "Room \"%s\" was not found in rooms.yaml.\n", rm->name);
        return;
    }
    vwrite_user(user,
            "You have now reloaded the description for the room \"%s\".\n",
            rm->name);
    write_syslog(SYSLOG, 1,
            "%s reloaded the description for the room %s\n",
            user->name, rm->name);
}
