#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Allows a user to alter the minimum level which can use the command given
 */
void
set_command_level(UR_OBJECT user)
{
    CMD_OBJECT cmd;
    size_t len;
    enum lvl_value lvl;

    if (word_count < 3) {
        write_user(user, "Usage: setcmdlev <command name> <level>|norm\n");
        return;
    }
    /* FIXME: command search order is different than command_table/exec_com()
     * because it uses the alpha sorted command list instead! */
    len = strlen(word[1]);
    for (cmd = first_command; cmd; cmd = cmd->next) {
        if (!strncmp(word[1], cmd->name, len)) {
            break;
        }
    }
    if (!cmd) {
        vwrite_user(user, "The command \"~OL%s~RS\" could not be found.\n",
                word[1]);
        return;
    }
    /* levels and "norm" are checked in upper case */
    strtoupper(word[2]);
    if (!strcmp(word[2], "NORM")) {
        /* FIXME: Permissions are weak setting level via "norm" */
        if (cmd->level == (enum lvl_value) command_table[cmd->id].level) {
            write_user(user, "That command is already at its normal level.\n");
            return;
        }
        cmd->level = (enum lvl_value) command_table[cmd->id].level;
        write_syslog(SYSLOG, 1,
                "%s has returned level to normal for cmd \"%s\"\n",
                user->name, cmd->name);
        write_monitor(user, NULL, 0);
        vwrite_room(NULL,
                "~OL~FR--==<~RS The level for command ~OL%s~RS has been returned to %s ~OL~FR>==--\n",
                cmd->name, user_level[cmd->level].name);
        return;
    }
    lvl = get_level(word[2]);
    if (lvl == NUM_LEVELS) {
        write_user(user, "Usage: setcmdlev <command> <level>|norm\n");
        return;
    }
    if (lvl > user->level) {
        write_user(user,
                "You cannot set a command level to one greater than your own.\n");
        return;
    }
    if (user->level < (enum lvl_value) command_table[cmd->id].level) {
        write_user(user,
                "You are not a high enough level to alter that command level.\n");
        return;
    }
    cmd->level = lvl;
    write_syslog(SYSLOG, 1, "%s has set the level for cmd \"%s\" to %d (%s)\n",
            user->name, cmd->name, cmd->level,
            user_level[cmd->level].name);
    write_monitor(user, NULL, 0);
    vwrite_room(NULL,
            "~OL~FR--==<~RS The level for command ~OL%s~RS has been set to %s ~OL~FR>==--\n",
            cmd->name, user_level[cmd->level].name);
}