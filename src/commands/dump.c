#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Allows you to dump certain things to files as a record
 */
void
dump_to_file(UR_OBJECT user)
{
    char filename[80], bstr[32];
    FILE *fp;
    UR_OBJECT u;
    UD_OBJECT d;
    RM_OBJECT rm;
    CMD_OBJECT cmd;
    size_t l;
    int ucount, dcount, rmcount, cmdcount, lcount;
    int tsize;
    int drcount;
#ifdef NETLINKS
    NL_OBJECT nl;
    int nlcount;
#endif
    enum lvl_value lvl;
    int days, hours, mins, secs;
    int i, j;

    if (word_count < 2) {
        write_user(user, "Usage: dump -u|-r <rank>|-c|-m|-s\n");
        return;
    }
    strtolower(word[1]);
    /* see if -r switch was used : dump all users of given level */
    if (!strcmp("-r", word[1])) {
        if (word_count < 3) {
            write_user(user, "Usage: dump -r <rank>\n");
            return;
        }
        strtoupper(word[2]);
        lvl = get_level(word[2]);
        if (lvl == NUM_LEVELS) {
            write_user(user, "Usage: dump -r <rank>\n");
            return;
        }
        sprintf(filename, "%s/%s.dump", DUMPFILES, user_level[lvl].name);
        fp = fopen(filename, "w");
        if (!fp) {
            write_user(user,
                    "There was an error trying to open the file to dump to.\n");
            write_syslog(SYSLOG, 0,
                    "Unable to open dump file %s in dump_to_file().\n",
                    filename);
            return;
        }
        fprintf(fp,
                "------------------------------------------------------------------------------\n");
        fprintf(fp, "Users of level %s %s\n", user_level[lvl].name, long_date(1));
        fprintf(fp,
                "------------------------------------------------------------------------------\n");
        drcount = 0;
        for (d = first_user_entry; d; d = d->next) {
            if (d->level != lvl) {
                continue;
            }
            fprintf(fp, "%s\n", d->name);
            ++drcount;
        }
        fprintf(fp,
                "------------------------------------------------------------------------------\n");
        fprintf(fp, "Total users at %s : %d\n", user_level[lvl].name, drcount);
        fprintf(fp,
                "------------------------------------------------------------------------------\n\n");
        fclose(fp);
        sprintf(text,
                "Dumped rank ~OL%s~RS to file.  ~OL%d~RS user%s recorded.\n",
                user_level[lvl].name, drcount, PLTEXT_S(drcount));
        write_user(user, text);
        return;
    }
    /* check to see if -u switch was used : dump all users */
    if (!strcmp("-u", word[1])) {
        sprintf(filename, "%s/users.dump", DUMPFILES);
        fp = fopen(filename, "w");
        if (!fp) {
            write_user(user,
                    "There was an error trying to open the file to dump to.\n");
            write_syslog(SYSLOG, 0,
                    "Unable to open dump file %s in dump_to_file().\n",
                    filename);
            return;
        }
        fprintf(fp,
                "------------------------------------------------------------------------------\n");
        fprintf(fp, "All users %s\n", long_date(1));
        fprintf(fp,
                "------------------------------------------------------------------------------\n");
        dcount = 0;
        for (d = first_user_entry; d; d = d->next) {
            fprintf(fp, "%s\n", d->name);
            ++dcount;
        }
        fprintf(fp,
                "------------------------------------------------------------------------------\n");
        fprintf(fp, "Total users : %d\n", dcount);
        fprintf(fp,
                "------------------------------------------------------------------------------\n\n");
        fclose(fp);
        sprintf(text, "Dumped all users to file.  ~OL%d~RS user%s recorded.\n",
                dcount, PLTEXT_S(dcount));
        write_user(user, text);
        return;
    }
    /* check to see if -c switch was used : dump last few commands used */
    if (!strcmp("-c", word[1])) {
        sprintf(filename, "%s/commands.dump", DUMPFILES);
        fp = fopen(filename, "w");
        if (!fp) {
            write_user(user,
                    "There was an error trying to open the file to dump to.\n");
            write_syslog(SYSLOG, 0,
                    "Unable to open dump file %s in dump_to_file().\n",
                    filename);
            return;
        }
        fprintf(fp,
                "------------------------------------------------------------------------------\n");
        fprintf(fp, "The last 16 commands %s\n", long_date(1));
        fprintf(fp,
                "------------------------------------------------------------------------------\n");
        j = amsys->last_cmd_cnt - 16;
        for (i = j > 0 ? j : 0; i < amsys->last_cmd_cnt; ++i) {
            fprintf(fp, "%s\n", cmd_history[i & 15]);
        }
        fprintf(fp,
                "------------------------------------------------------------------------------\n\n");
        fclose(fp);
        write_user(user, "Dumped the last 16 commands that have been used.\n");
        return;
    }
    /* check to see if -m was used : dump memory currently being used */
    if (!strcmp("-m", word[1])) {
        sprintf(filename, "%s/memory.dump", DUMPFILES);
        fp = fopen(filename, "w");
        if (!fp) {
            write_user(user,
                    "There was an error trying to open the file to dump to.\n");
            write_syslog(SYSLOG, 0,
                    "Unable to open dump file %s in dump_to_file().\n",
                    filename);
            return;
        }
        ucount = 0;
        for (u = user_first; u; u = u->next) {
            ++ucount;
        }
        dcount = 0;
        for (d = first_user_entry; d; d = d->next) {
            ++dcount;
        }
        rmcount = 0;
        for (rm = room_first; rm; rm = rm->next) {
            ++rmcount;
        }
        cmdcount = 0;
        for (cmd = first_command; cmd; cmd = cmd->next) {
            ++cmdcount;
        }
        lcount = 0;
        for (l = 0; l < LASTLOGON_NUM; ++l) {
            ++lcount;
        }
        tsize =
                ucount * (sizeof *u) + rmcount * (sizeof *rm) + dcount * (sizeof *d) +
                cmdcount * (sizeof *cmd) + (sizeof *amsys) +
                lcount * (sizeof *last_login_info);
#ifdef NETLINKS
        nlcount = 0;
        for (nl = nl_first; nl; nl = nl->next) {
            ++nlcount;
        }
        tsize += nlcount * (sizeof *nl);
#endif
        fprintf(fp,
                "------------------------------------------------------------------------------\n");
        fprintf(fp, "Memory Object Allocation %s\n", long_date(1));
        fprintf(fp,
                "------------------------------------------------------------------------------\n");
        fprintf(fp, "  %-16s: %8d * %8d bytes = %8d total bytes\n", "users",
                ucount, (int) (sizeof *u), ucount * (int) (sizeof *u));
        fprintf(fp, "  %-16s: %8d * %8d bytes = %8d total bytes\n", "all users",
                dcount, (int) (sizeof *d), dcount * (int) (sizeof *d));
        fprintf(fp, "  %-16s: %8d * %8d bytes = %8d total bytes\n", "rooms",
                rmcount, (int) (sizeof *rm), rmcount * (int) (sizeof *rm));
        fprintf(fp, "  %-16s: %8d * %8d bytes = %8d total bytes\n", "commands",
                cmdcount, (int) (sizeof *cmd), cmdcount * (int) (sizeof *cmd));
        fprintf(fp, "  %-16s: %8d * %8d bytes = %8d total bytes\n", "system", 1,
                (int) (sizeof *amsys), 1 * (int) (sizeof *amsys));
        fprintf(fp, "  %-16s: %8d * %8d bytes = %8d total bytes\n", "last logins",
                lcount, (int) (sizeof *last_login_info),
                lcount * (int) (sizeof *last_login_info));
#ifdef NETLINKS
        fprintf(fp, "  %-16s: %8d * %8d bytes = %8d total bytes\n",
                "netlinks", nlcount, (int) (sizeof *nl),
                nlcount * (int) (sizeof *nl));
#endif
        fprintf(fp,
                "------------------------------------------------------------------------------\n");
        fprintf(fp, "  %-16s: %12.3f Mb             %8d total bytes\n", "total",
                tsize / 1048576.0, tsize);
        fprintf(fp,
                "------------------------------------------------------------------------------\n");
        fclose(fp);
        write_user(user,
                "Dumped the memory currently being used by the talker.\n");
        return;
    }
    /* check to see if -s switch was used : show system details */
    if (!strcmp("-s", word[1])) {
        sprintf(filename, "%s/system.dump", DUMPFILES);
        fp = fopen(filename, "w");
        if (!fp) {
            write_user(user,
                    "There was an error trying to open the file to dump to.\n");
            write_syslog(SYSLOG, 0,
                    "Unable to open dump file %s in dump_to_file().\n",
                    filename);
            return;
        }
        strftime(bstr, 32, "%a %Y-%m-%d %H:%M:%S", localtime(&amsys->boot_time));
        secs = (int) (time(0) - amsys->boot_time);
        days = secs / 86400;
        hours = (secs % 86400) / 3600;
        mins = (secs % 3600) / 60;
        secs = secs % 60;
        fprintf(fp,
                "------------------------------------------------------------------------------\n");
        fprintf(fp, "System details %s\n", long_date(1));
        fprintf(fp,
                "------------------------------------------------------------------------------\n");
        fprintf(fp, "Node name   : %s\n", amsys->uts.nodename);
        fprintf(fp, "Running on  : %s %s %s %s\n", amsys->uts.machine,
                amsys->uts.sysname, amsys->uts.release, amsys->uts.version);
        fprintf(fp, "Talker PID  : %u\n", getpid());
        fprintf(fp, "Booted      : %s\n", bstr);
        fprintf(fp,
                "Uptime      : %d day%s, %d hour%s, %d minute%s, %d second%s\n",
                days, PLTEXT_S(days), hours, PLTEXT_S(hours), mins,
                PLTEXT_S(mins), secs, PLTEXT_S(secs));
#ifdef NETLINKS
        fprintf(fp, "Netlinks    : Compiled and running\n");
#else
        fprintf(fp, "Netlinks    : Not currently compiled or running\n");
#endif
        switch (amsys->resolve_ip) {
        default:
            fprintf(fp, "IP Resolve  : Off\n");
            break;
        case 1:
            fprintf(fp, "IP Resolve  : On via automatic library\n");
            break;
#ifdef MANDNS
        case 2:
            fprintf(fp, "IP Resolve  : On via manual program\n");
            break;
#endif
#ifdef IDENTD
        case 3:
            fprintf(fp, "IP Resolve  : On via ident daemon\n");
            break;
#endif
        }
        fprintf(fp,
                "------------------------------------------------------------------------------\n\n");
        fclose(fp);
        write_user(user, "Dumped the system details.\n");
        return;
    }
    write_user(user, "Usage: dump -u|-r <rank>|-c|-m|-s\n");
}
