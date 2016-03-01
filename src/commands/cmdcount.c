#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Display how many times a command has been used, and its overall
 * percentage of showings compared to other commands
 */
void
show_command_counts(UR_OBJECT user)
{
    CMD_OBJECT cmd;
    int total_hits, total_cmds, cmds_used, i, x;
    char text2[ARR_SIZE];

    x = i = total_hits = total_cmds = cmds_used = 0;
    *text2 = '\0';
    /* get totals of commands and hits */
    for (cmd = first_command; cmd; cmd = cmd->next) {
        total_hits += cmd->count;
        ++total_cmds;
    }
    start_pager(user);
    write_user(user,
            "\n+----------------------------------------------------------------------------+\n");
    write_user(user,
            "| ~FC~OLCommand usage statistics~RS                                                   |\n");
    write_user(user,
            "+----------------------------------------------------------------------------+\n");
    for (cmd = first_command; cmd; cmd = cmd->next) {
        /* skip if command has not been used so as not to cause crash by trying to / by 0 */
        if (!cmd->count) {
            continue;
        }
        ++cmds_used;
        /* skip if user cannot use that command anyway */
        if (cmd->level > user->level) {
            continue;
        }
        i = ((cmd->count * 10000) / total_hits);
        /* build up first half of the string */
        if (!x) {
            sprintf(text, "| %11.11s %4d %3d%% ", cmd->name, cmd->count, i / 100);
            ++x;
        }            /* build up full line and print to user */
        else if (x == 1) {
            sprintf(text2, "   %11.11s %4d %3d%%   ", cmd->name, cmd->count,
                    i / 100);
            strcat(text, text2);
            write_user(user, text);
            *text = '\0';
            *text2 = '\0';
            ++x;
        } else {
            sprintf(text2, "   %11.11s %4d %3d%%  |\n", cmd->name, cmd->count,
                    i / 100);
            strcat(text, text2);
            write_user(user, text);
            *text = '\0';
            *text2 = '\0';
            x = 0;
        }
    }
    /* If you have only printed first half of the string */
    if (x == 1) {
        strcat(text, "                                                     |\n");
        write_user(user, text);
    }
    if (x == 2) {
        strcat(text, "                          |\n");
        write_user(user, text);
    }
    write_user(user,
            "|                                                                            |\n");
    write_user(user,
            "| Any other commands have not yet been used, or you cannot view them         |\n");
    write_user(user,
            "+----------------------------------------------------------------------------+\n");
    sprintf(text2,
            "Total of ~OL%d~RS commands.    ~OL%d~RS command%s used a total of ~OL%d~RS time%s.",
            total_cmds, cmds_used, PLTEXT_S(cmds_used), total_hits,
            PLTEXT_S(total_hits));
    vwrite_user(user, "| %-92s |\n", text2);
    write_user(user,
            "+----------------------------------------------------------------------------+\n");
    stop_pager(user);
}
