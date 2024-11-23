#include "../includes/defines.h"
#include "../includes/globals.h"
#include "../includes/commands.h"
#include "../includes/prototypes.h"

/*
 * Display how many times a command has been used, and its overall
 * percentage of showings compared to other commands
 */
void
show_command_counts(UR_OBJECT user)
{
    CMD_OBJECT cmd;
    int total_hits = 0, total_cmds = 0, cmds_used = 0, i, x = 0;

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

    sds row = sdsempty();
    for (cmd = first_command; cmd; cmd = cmd->next) {
        if (cmd->count == 0 || cmd->level > user->level) {
            continue;
        }
        ++cmds_used;
        i = (cmd->count * 10000) / total_hits;
        sds entry = sdsempty();
        entry = sdscatprintf(entry, "%12.12s %4d %3d%%", cmd->name, cmd->count, i / 100);

        if (x == 0) {
            row = sdscatprintf(row, "| %s ", entry);
            ++x;
        } else if (x == 1) {
            row = sdscatprintf(row, "   %s ", entry);
            ++x;
        } else {
            row = sdscatprintf(row, "   %s |\n", entry);
            write_user(user, row);
            sdsfree(row);
            row = sdsempty();
            x = 0;
        }
        sdsfree(entry);
    }

    if (x == 1) {
        row = sdscat(row, "                                                    |\n");
        write_user(user, row);
    } else if (x == 2) {
        row = sdscat(row, "                          |\n");
        write_user(user, row);
    }

    sdsfree(row);

    write_user(user,
        "|                                                                            |\n");
    write_user(user,
        "| Any other commands have not yet been used, or you cannot view them         |\n");
    write_user(user,
        "+----------------------------------------------------------------------------+\n");

    char *summary = sdscatprintf(sdsempty(),
        "Total of ~OL%d~RS commands.    ~OL%d~RS command%s used a total of ~OL%d~RS time%s.",
        total_cmds, cmds_used, PLTEXT_S(cmds_used), total_hits, PLTEXT_S(total_hits));
    vwrite_user(user, "| %-92s |\n", summary);
    sdsfree(summary);

    write_user(user,
        "+----------------------------------------------------------------------------+\n");

    stop_pager(user);
}
