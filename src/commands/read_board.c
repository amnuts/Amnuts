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
 * Read the message board
 */
void
read_board(UR_OBJECT user)
{
    char filename[80];
    const char *name;
    RM_OBJECT rm;
    int ret;

    rm = NULL;
    if (word_count < 2) {
        rm = user->room;
    } else {
        if (word_count >= 3) {
            rm = get_room(word[1]);
            if (!rm) {
                write_user(user, nosuchroom);
                return;
            }
            ret = atoi(word[2]);
            read_board_specific(user, rm, ret);
            return;
        }
        ret = atoi(word[1]);
        if (ret) {
            read_board_specific(user, user->room, ret);
            return;
        }
        rm = get_room(word[1]);
        if (!rm) {
            write_user(user, nosuchroom);
            return;
        }
        if (!has_room_access(user, rm)) {
            write_user(user,
                    "That room is currently private, you cannot read the board.\n");
            return;
        }
    }
    vwrite_user(user, "\n~BB*** The %s message board ***\n\n", rm->name);
    if (is_personal_room(rm)) {
        sprintf(filename, "%s/%s/%s.B", USERFILES, USERROOMS, rm->owner);
    } else {
        char bfile[ROOM_NAME_LEN + 4];
        snprintf(bfile, sizeof bfile, "%s.B", rm->name);
        locale_default_path(filename, sizeof filename, DATAFILES, bfile);
    }
    user->filepos = 0;
    ret = more(user, user->socket, filename);
    if (!ret) {
        write_user(user, "There are no messages on the board.\n\n");
    } else if (ret == 1) {
        user->misc_op = 2;
    }
    name = user->vis ? user->recap : invisname;
    if (rm == user->room) {
        vwrite_room_except(user->room, user, "%s~RS reads the message board.\n",
                name);
    }
}
