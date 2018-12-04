
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * show the ranks and commands per level for the talker
 */
void
show_ranks(UR_OBJECT user)
{
    enum lvl_value lvl;
    CMD_OBJECT cmd;
    int total, cnt[NUM_LEVELS];

    for (lvl = JAILED; lvl < NUM_LEVELS; lvl = (enum lvl_value) (lvl + 1)) {
        cnt[lvl] = 0;
    }
    for (cmd = first_command; cmd; cmd = cmd->next) {
        ++cnt[cmd->level];
    }
    write_user(user,
            "+----------------------------------------------------------------------------+\n");
    write_user(user,
            "| ~OL~FCThe ranks (levels) on the talker~RS                                           |\n");
    write_user(user,
            "+----------------------------------------------------------------------------+\n");
    total = 0;
    for (lvl = JAILED; lvl < NUM_LEVELS; lvl = (enum lvl_value) (lvl + 1)) {
        vwrite_user(user,
                "| %s(%1.1s) : %-10.10s : Lev %d : %3d cmds total : %2d cmds this level             ~RS|\n",
                lvl == user->level ? "~FY~OL" : "", user_level[lvl].alias,
                user_level[lvl].name, lvl, total += cnt[lvl], cnt[lvl]);
    }
    write_user(user,
            "+----------------------------------------------------------------------------+\n\n");
}