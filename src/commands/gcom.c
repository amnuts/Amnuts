#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * stop a user from using a certain command
 */
void
user_gcom(UR_OBJECT user)
{
    CMD_OBJECT cmd;
    UR_OBJECT u;
    size_t i, x;

    if (word_count < 2) {
        write_user(user, "Usage: gcom <user> [<command>]\n");
        return;
    }
    u = get_user_name(user, word[1]);
    if (!u) {
        write_user(user, notloggedon);
        return;
    }
    if (u == user) {
        write_user(user, "You cannot give yourself any commands.\n");
        return;
    }
    /* if no command is given, then just view given commands */
    if (word_count < 3) {
        write_user(user,
                "+----------------------------------------------------------------------------+\n");
        vwrite_user(user, "~OL~FCGiven commands for user~RS \"%s~RS\"\n",
                u->recap);
        write_user(user,
                "+----------------------------------------------------------------------------+\n");
        x = 0;
        for (i = 0; i < MAX_GCOMS; ++i) {
            if (u->gcoms[i] == -1) {
                continue;
            }
            for (cmd = first_command; cmd; cmd = cmd->next) {
                if (cmd->id == u->gcoms[i]) {
                    break;
                }
            }
            if (!cmd) {
                /* XXX: Maybe emit some sort of error? */
                continue;
            }
            vwrite_user(user, "~OL%s~RS (level %d)\n", cmd->name, cmd->level);
            ++x;
        }
        if (!x) {
            write_user(user, "User has no given commands.\n");
        }
        write_user(user,
                "+----------------------------------------------------------------------------+\n\n");
        return;
    }
    if (u->level >= user->level) {
        write_user(user,
                "You cannot give commands to a user with the same or higher level as yourself.\n");
        return;
    }
    /* FIXME: command search order is different than command_table/exec_com()
     * because it uses the alpha sorted command list instead! */
    i = strlen(word[2]);
    for (cmd = first_command; cmd; cmd = cmd->next) {
        if (!strncmp(word[2], cmd->name, i)) {
            break;
        }
    }
    if (!cmd) {
        write_user(user, "That command does not exist.\n");
        return;
    }
    if (u->level >= cmd->level) {
        vwrite_user(user, "%s can already use that command.\n", u->name);
        return;
    }
    if (user->level < cmd->level) {
        write_user(user,
                "You cannot use that command, so you cannot give it to others.\n");
        return;
    }
    /* check to see if the user has previously been banned from using the command */
    if (has_xcom(u, cmd->id)) {
        write_user(user,
                "You cannot give a command to a user that already has it banned.\n");
        return;
    }
    if (has_gcom(u, cmd->id)) {
        /* user already has the command given, so ungive it */
        if (!set_xgcom(user, u, cmd->id, 0, 0)) {
            /* XXX: Maybe emit some sort of error? */
            return;
        }
        vwrite_user(user, "You have removed the given command \"%s\" for %s~RS\n",
                word[2], u->recap);
        vwrite_user(u,
                "Access to the given command \"%s\" has now been taken away from you.\n",
                word[2]);
        sprintf(text, "%s ~FRUNGCOM~RS'd the command \"%s\"\n", user->name,
                word[2]);
        add_history(u->name, 1, "%s", text);
        write_syslog(SYSLOG, 1, "%s UNGCOM'd the command \"%s\" for %s\n",
                user->name, word[2], u->name);
    } else {
        /* user does not have the command given, so give it */
        if (!set_xgcom(user, u, cmd->id, 0, 1)) {
            /* XXX: Maybe emit some sort of error? */
            return;
        }
        vwrite_user(user, "You have given the \"%s\" command for %s\n", word[2],
                u->name);
        vwrite_user(u, "You have been given access to the command \"%s\".\n",
                word[2]);
        sprintf(text, "%s ~FGGCOM~RS'd the command \"%s\"\n", user->name,
                word[2]);
        add_history(u->name, 1, "%s", text);
        write_syslog(SYSLOG, 1, "%s GCOM'd the command \"%s\" for %s\n",
                user->name, word[2], u->name);
    }
}
