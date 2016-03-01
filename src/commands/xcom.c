#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * stop a user from using a certain command
 */
void
user_xcom(UR_OBJECT user)
{
    CMD_OBJECT cmd;
    UR_OBJECT u;
    size_t i, x;

    if (word_count < 2) {
        write_user(user, "Usage: xcom <user> [<command>]\n");
        return;
    }
    u = get_user_name(user, word[1]);
    if (!u) {
        write_user(user, notloggedon);
        return;
    }
    if (u == user) {
        write_user(user, "You cannot ban any commands of your own.\n");
        return;
    }
    /* if no command is given, then just view banned commands */
    if (word_count < 3) {
        write_user(user,
                "+----------------------------------------------------------------------------+\n");
        vwrite_user(user, "~OL~FCBanned commands for user \"%s~RS\"\n", u->recap);
        write_user(user,
                "+----------------------------------------------------------------------------+\n");
        x = 0;
        for (i = 0; i < MAX_XCOMS; ++i) {
            if (u->xcoms[i] == -1) {
                continue;
            }
            for (cmd = first_command; cmd; cmd = cmd->next) {
                if (cmd->id == u->xcoms[i]) {
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
            write_user(user, "User has no banned commands.\n");
        }
        write_user(user,
                "+----------------------------------------------------------------------------+\n\n");
        return;
    }
    if (u->level >= user->level) {
        write_user(user,
                "You cannot ban the commands of a user with the same or higher level as yourself.\n");
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
    if (u->level < cmd->level) {
        vwrite_user(user,
                "%s is not of a high enough level to use that command anyway.\n",
                u->name);
        return;
    }
    /* check to see is the user has previously been given the command */
    if (has_gcom(u, cmd->id)) {
        write_user(user,
                "You cannot ban a command that a user has been specifically given.\n");
        return;
    }
    if (has_xcom(u, cmd->id)) {
        /* user already has the command banned, so unban it */
        if (!set_xgcom(user, u, cmd->id, 1, 0)) {
            /* XXX: Maybe emit some sort of error? */
            return;
        }
        vwrite_user(user, "You have unbanned the \"%s\" command for %s\n",
                word[2], u->name);
        vwrite_user(u,
                "The command \"%s\" has been unbanned and you can use it again.\n",
                word[2]);
        sprintf(text, "%s ~FGUNXCOM~RS'd the command \"%s\"\n", user->name,
                word[2]);
        add_history(u->name, 1, "%s", text);
        write_syslog(SYSLOG, 1, "%s UNXCOM'd the command \"%s\" for %s\n",
                user->name, word[2], u->name);
    } else {
        /* user does not have the command banned, so ban it */
        if (!set_xgcom(user, u, cmd->id, 1, 1)) {
            /* XXX: Maybe emit some sort of error? */
            return;
        }
        vwrite_user(user, "You have banned the \"%s\" command for %s\n", word[2],
                u->name);
        vwrite_user(u, "You have been banned from using the command \"%s\".\n",
                word[2]);
        sprintf(text, "%s ~FRXCOM~RS'd the command \"%s\"\n", user->name,
                word[2]);
        add_history(u->name, 1, "%s", text);
        write_syslog(SYSLOG, 1, "%s XCOM'd the command \"%s\" for %s\n",
                user->name, word[2], u->name);
    }
}