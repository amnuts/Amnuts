
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Show talker system parameters etc
 */
void
system_details(UR_OBJECT user)
{
    static const char *const ca[] ={"NONE", "SHUTDOWN", "REBOOT", "SEAMLESS"};
    static const char *const rip[] = {"OFF", "AUTO", "MANUAL", "IDENTD"};
    char bstr[32], foo[ARR_SIZE];
    UR_OBJECT u;
    UD_OBJECT d;
    RM_OBJECT rm;
    CMD_OBJECT cmd;
    size_t l;
    int ucount, dcount, rmcount, cmdcount, lcount;
    int tsize;
    int uccount, rmpcount;
#ifdef NETLINKS
    NL_OBJECT nl;
    int nlcount, nlupcount, nlicount, nlocount, rmnlicount;
#endif
    enum lvl_value lvl;
    int days, hours, mins, secs;

    if (word_count < 2 || !strcasecmp("-a", word[1])) {
        write_user(user,
                "\n+----------------------------------------------------------------------------+\n");
        sprintf(text, "System Details for %s (Amnuts version %s)", TALKER_NAME,
                AMNUTSVER);
        vwrite_user(user, "| ~OL~FC%-74.74s~RS |\n", text);
        write_user(user,
                "|----------------------------------------------------------------------------|\n");
        /* Get some values */
        strftime(bstr, 32, "%a %Y-%m-%d %H:%M:%S", localtime(&amsys->boot_time));
        secs = (int) (time(0) - amsys->boot_time);
        days = secs / 86400;
        hours = (secs % 86400) / 3600;
        mins = (secs % 3600) / 60;
        secs = secs % 60;
        /* Show header parameters */
#ifdef NETLINKS
#ifdef IDENTD
        if (amsys->ident_state) {
#ifdef WIZPORT
            write_user(user,
                    "| talker pid      identd pid      main port      wiz port      netlinks port |\n");
            vwrite_user(user,
                    "|   %-5u           %-5u           %-5.5s         %-5.5s            %-5.5s     |\n",
                    getpid(), amsys->ident_pid, amsys->mport_port,
                    amsys->wport_port, amsys->nlink_port);
#else
            write_user(user,
                    "| talker pid      identd pid      main port      netlinks port               |\n");
            vwrite_user(user,
                    "|   %-5u           %-5u           %-5.5s         %-5.5s                      |\n",
                    getpid(), amsys->ident_pid, amsys->mport_port,
                    amsys->nlink_port);
#endif
            write_user(user,
                    "+----------------------------------------------------------------------------+\n");
        } else
#endif
        {
#ifdef WIZPORT
            write_user(user,
                    "| talker pid           main port            wiz port           netlinks port |\n");
            vwrite_user(user,
                    "|   %-5u               %-5.5s                %-5.5s                 %-5.5s     |\n",
                    getpid(), amsys->mport_port, amsys->wport_port,
                    amsys->nlink_port);
#else
            write_user(user,
                    "| talker pid           main port            netlinks port                    |\n");
            vwrite_user(user,
                    "|   %-5u               %-5.5s                %-5.5s                           |\n",
                    getpid(), amsys->mport_port, amsys->nlink_port);
#endif
            write_user(user,
                    "+----------------------------------------------------------------------------+\n");
        }
#else
#ifdef IDENTD
        if (amsys->ident_state) {
#ifdef WIZPORT
            write_user(user,
                    "| talker pid            identd pid             main port            wiz port |\n");
            vwrite_user(user,
                    "|   %-5u                  %-5u                 %-5.5s                %-5.5s  |\n",
                    getpid(), amsys->ident_pid, amsys->mport_port,
                    amsys->wport_port);
#else
            write_user(user,
                    "| talker pid            identd pid             main port                     |\n");
            vwrite_user(user,
                    "|   %-5u                  %-5u                 %-5.5s                       |\n",
                    getpid(), amsys->ident_pid, amsys->mport_port);
#endif
            write_user(user,
                    "+----------------------------------------------------------------------------+\n");
        } else
#endif
        {
#ifdef WIZPORT
            write_user(user,
                    "| talker pid                       main port                        wiz port |\n");
            vwrite_user(user,
                    "|   %-5u                            %-5.5s                           %-5.5s   |\n",
                    getpid(), amsys->mport_port, amsys->wport_port);
#else
            write_user(user,
                    "| talker pid                       main port                                 |\n");
            vwrite_user(user,
                    "|   %-5u                            %-5.5s                                   |\n",
                    getpid(), amsys->mport_port);
#endif
            write_user(user,
                    "+----------------------------------------------------------------------------+\n");
        }
#endif
        vwrite_user(user, "| %-17.17s : %-54.54s |\n", "talker booted", bstr);
        sprintf(text, "%d day%s, %d hour%s, %d minute%s, %d second%s", days,
                PLTEXT_S(days), hours, PLTEXT_S(hours), mins, PLTEXT_S(mins),
                secs, PLTEXT_S(secs));
        vwrite_user(user, "| %-17.17s : %-54.54s |\n", "uptime", text);
        vwrite_user(user,
                "| %-17.17s : %-6.6s                %-17.17s : %-10.10s   |\n",
                "system logging", offon[(amsys->logging) ? 1 : 0],
                "flood protection", offon[amsys->flood_protect]);
        vwrite_user(user,
                "| %-17.17s : %-6.6s                %-17.17s : %-10.10s   |\n",
                "ignoring sigterms", noyes[amsys->ignore_sigterm],
                "crash action", ca[amsys->crash_action]);
        sprintf(text, "every %d sec%s", amsys->heartbeat,
                PLTEXT_S(amsys->heartbeat));
        vwrite_user(user,
                "| %-17.17s : %-20.20s  %-17.17s : %-10.10s   |\n",
                "heartbeat", text, "resolving IP", rip[amsys->resolve_ip]);
        vwrite_user(user, "| %-17.17s : %-54.54s |\n",
                "swear ban", minmax[amsys->ban_swearing]);
        if (word_count < 2) {
            write_user(user,
                    "+----------------------------------------------------------------------------+\n");
            write_user(user,
                    "| For other options, see: system -m, -n, -r, -u, -a                          |\n");
            write_user(user,
                    "+----------------------------------------------------------------------------+\n\n");
            return;
        }
    }
    /* user option */
    if (!strcasecmp("-u", word[1]) || !strcasecmp("-a", word[1])) {
        uccount = 0;
        for (u = user_first; u; u = u->next) {
            if (u->type == CLONE_TYPE) {
                ++uccount;
            }
        }
        write_user(user,
                "+----------------------------------------------------------------------------+\n");
        write_user(user,
                "| ~OL~FCSystem Details - Users~RS                                                     |\n");
        write_user(user,
                "|----------------------------------------------------------------------------|\n");
        for (lvl = JAILED; lvl < NUM_LEVELS; lvl = (enum lvl_value) (lvl + 1)) {
            vwrite_user(user,
                    "| users at level %-8.8s : %-5d                                            |\n",
                    user_level[lvl].name, amsys->level_count[lvl]);
        }
        write_user(user,
                "+----------------------------------------------------------------------------+\n");
        vwrite_user(user,
                "| %-24.24s: %-5d       %-24.24s: %-5d      |\n",
                "online now", amsys->num_of_users, "max allowed online",
                amsys->max_users);
        vwrite_user(user, "| %-24.24s: %-5d       %-24.24s: %-5d      |\n",
                "new this boot", amsys->logons_new, "returning this boot",
                amsys->logons_old);
        vwrite_user(user, "| %-24.24s: %-5d       %-24.24s: %-5d      |\n",
                "clones now on", uccount, "max allowed clones",
                amsys->max_clones);
        sprintf(text, "%d sec%s", amsys->login_idle_time,
                PLTEXT_S(amsys->login_idle_time));
        sprintf(foo, "%d sec%s", amsys->user_idle_time,
                PLTEXT_S(amsys->user_idle_time));
        vwrite_user(user, "| %-24.24s: %-10.10s  %-24.24s: %-10.10s |\n",
                "login idle time out", text, "user idle time out", foo);
        vwrite_user(user, "| %-24.24s: %-10.10s  %-24.24s: %-10.10s |\n",
                "time out maxlevel",
                user_level[amsys->time_out_maxlevel].name, "time out afks",
                noyes[amsys->time_out_afks]);
        vwrite_user(user, "| %-24.24s: %-10.10s  %-24.24s: %-10.10s |\n",
                "current minlogin level",
                amsys->minlogin_level ==
                NUM_LEVELS ? "NONE" : user_level[amsys->minlogin_level].name,
                "min login disconnect", noyes[amsys->boot_off_min]);
        vwrite_user(user, "| %-24.24s: %-10.10s  %-24.24s: %-10.10s |\n",
                "newbie prompt default", offon[amsys->prompt_def],
                "newbie colour default", offon[amsys->colour_def]);
        vwrite_user(user, "| %-24.24s: %-10.10s  %-24.24s: %-10.10s |\n",
                "newbie charecho default", offon[amsys->charecho_def],
                "echoing password default", offon[amsys->passwordecho_def]);
        vwrite_user(user, "| %-24.24s: %-10.10s  %-24.24s: %-10.10s |\n",
                "name recaps allowed", noyes[amsys->allow_recaps],
                "smail auto-forwarding", offon[amsys->forwarding]);
        strftime(text, ARR_SIZE * 2, "%a %Y-%m-%d %H:%M:%S",
                localtime(&amsys->auto_purge_date));
        vwrite_user(user, "| %-24.24s: %-4s  %-15.15s: %-25s |\n", "autopurge on",
                noyes[amsys->auto_purge_date != -1], "next autopurge", text);
        sprintf(text, "%d day%s", USER_EXPIRES, PLTEXT_S(USER_EXPIRES));
        sprintf(foo, "%d day%s", NEWBIE_EXPIRES, PLTEXT_S(NEWBIE_EXPIRES));
        vwrite_user(user, "| %-24.24s: %-10.10s  %-24.24s: %-10.10s |\n",
                "purge length (newbies)", foo, "purge length (users)", text);
#ifdef WIZPORT
        vwrite_user(user,
                "| %-24.24s: %-10.10s                                       |\n",
                "wizport min login level",
                user_level[amsys->wizport_level].name);
#endif
        if (!strcasecmp("-u", word[1])) {
            write_user(user,
                    "+----------------------------------------------------------------------------+\n\n");
            return;
        }
    }
    /* Netlinks Option */
    if (!strcasecmp("-n", word[1]) || !strcasecmp("-a", word[1])) {
        write_user(user,
                "+----------------------------------------------------------------------------+\n");
        write_user(user,
                "| ~OL~FCSystem Details - Netlinks~RS                                                  |\n");
        write_user(user,
                "|----------------------------------------------------------------------------|\n");
#ifdef NETLINKS
        rmnlicount = 0;
        for (rm = room_first; rm; rm = rm->next) {
            if (rm->inlink) {
                ++rmnlicount;
            }
        }
        nlcount = nlupcount = nlicount = nlocount = 0;
        for (nl = nl_first; nl; nl = nl->next) {
            ++nlcount;
            if (nl->type != UNCONNECTED && nl->stage == UP) {
                ++nlupcount;
            }
            if (nl->type == INCOMING) {
                ++nlicount;
            }
            if (nl->type == OUTGOING) {
                ++nlocount;
            }
        }
        vwrite_user(user, "| %-21.21s: %5d %45s |\n",
                "total netlinks", nlcount, " ");
        vwrite_user(user, "| %-21.21s: %5d secs     %-21.21s: %5d secs    |\n",
                "idle time out", amsys->net_idle_time,
                "keepalive interval", amsys->keepalive_interval);
        vwrite_user(user, "| %-21.21s: %5d          %-21.21s: %5d         |\n",
                "accepting connects", rmnlicount,
                "live connects", nlupcount);
        vwrite_user(user, "| %-21.21s: %5d          %-21.21s: %5d         |\n",
                "incoming connections", nlicount,
                "outgoing connections", nlocount);
        vwrite_user(user, "| %-21.21s: %-13.13s  %-21.21s: %-13.13s |\n",
                "remote user maxlevel", user_level[amsys->rem_user_maxlevel].name,
                "remote user deflevel", user_level[amsys->rem_user_deflevel].name);
        vwrite_user(user, "| %-21.21s: %5d bytes    %-21.21s: %5d bytes   |\n",
                "netlink structure", (int) (sizeof *nl),
                "total memory", nlcount * (int) (sizeof *nl));
#else
        write_user(user,
                "| Netlinks are not currently compiled into the talker.                       |\n");
#endif
        if (!strcasecmp("-n", word[1])) {
            write_user(user,
                    "+----------------------------------------------------------------------------+\n\n");
            return;
        }
    }
    /* Room Option */
    if (!strcasecmp("-r", word[1]) || !strcasecmp("-a", word[1])) {
        rmcount = rmpcount = 0;
        for (rm = room_first; rm; rm = rm->next) {
            ++rmcount;
            if (is_personal_room(rm)) {
                ++rmpcount;
            }
        }
        write_user(user,
                "+----------------------------------------------------------------------------+\n");
        write_user(user,
                "| ~OL~FCSystem Details - Rooms~RS                                                     |\n");
        write_user(user,
                "|----------------------------------------------------------------------------|\n");
        vwrite_user(user,
                "| %-16.16s: %-15.15s      %-20.20s: %5d         |\n",
                "gatecrash level", user_level[amsys->gatecrash_level].name,
                "min private count", amsys->min_private_users);
        sprintf(text, "%d day%s", amsys->mesg_life, PLTEXT_S(amsys->mesg_life));
        vwrite_user(user,
                "| %-16.16s: %-15.15s      %-20.20s: %.2d:%.2d         |\n",
                "message life", text, "message check time",
                amsys->mesg_check_hour, amsys->mesg_check_min);
        vwrite_user(user,
                "| %-16.16s: %5d                %-20.20s: %5d         |\n",
                "personal rooms", rmpcount, "total rooms", rmcount);
        vwrite_user(user, "| %-16.16s: %7d bytes        %-20.20s: %7d bytes |\n",
                "room structure", (int) (sizeof *rm), "total memory",
                rmcount * (int) (sizeof *rm));
        if (!strcasecmp("-r", word[1])) {
            write_user(user,
                    "+----------------------------------------------------------------------------+\n\n");
            return;
        }
    }
    /* Memory Option */
    if (!strcasecmp("-m", word[1]) || !strcasecmp("-a", word[1])) {
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
                ucount * (sizeof *u) + dcount * (sizeof *d) + rmcount * (sizeof *rm) +
                cmdcount * (sizeof *cmd) + (sizeof *amsys) +
                lcount * (sizeof *last_login_info);
#ifdef NETLINKS
        nlcount = 0;
        for (nl = nl_first; nl; nl = nl->next) {
            ++nlcount;
        }
        tsize += nlcount * (sizeof *nl);
#endif
        write_user(user,
                "+----------------------------------------------------------------------------+\n");
        write_user(user,
                "| ~OL~FCSystem Details - Memory Object Allocation~RS                                  |\n");
        write_user(user,
                "|----------------------------------------------------------------------------|\n");
        vwrite_user(user,
                "| %-16.16s: %8d * %8d bytes = %8d total bytes         |\n",
                "users", ucount, (int) (sizeof *u),
                ucount * (int) (sizeof *u));
        vwrite_user(user,
                "| %-16.16s: %8d * %8d bytes = %8d total bytes         |\n",
                "all users", dcount, (int) (sizeof *d),
                dcount * (int) (sizeof *d));
        vwrite_user(user,
                "| %-16.16s: %8d * %8d bytes = %8d total bytes         |\n",
                "rooms", rmcount, (int) (sizeof *rm),
                rmcount * (int) (sizeof *rm));
        vwrite_user(user,
                "| %-16.16s: %8d * %8d bytes = %8d total bytes         |\n",
                "commands", cmdcount, (int) (sizeof *cmd),
                cmdcount * (int) (sizeof *cmd));
        vwrite_user(user,
                "| %-16.16s: %8d * %8d bytes = %8d total bytes         |\n",
                "system", 1, (int) (sizeof *amsys),
                1 * (int) (sizeof *amsys));
        vwrite_user(user,
                "| %-16.16s: %8d * %8d bytes = %8d total bytes         |\n",
                "last logins", lcount, (int) (sizeof *last_login_info),
                lcount * (int) (sizeof *last_login_info));
#ifdef NETLINKS
        vwrite_user(user,
                "| %-16.16s: %8d * %8d bytes = %8d total bytes         |\n",
                "netlinks", nlcount, (int) (sizeof *nl),
                nlcount * (int) (sizeof *nl));
#endif
        write_user(user,
                "+----------------------------------------------------------------------------+\n");
        vwrite_user(user,
                "| %-16.16s: %12.3f Mb             %8d total bytes         |\n",
                "total", tsize / 1048576.0, tsize);
        if (!strcasecmp("-m", word[1])) {
            write_user(user,
                    "+----------------------------------------------------------------------------+\n\n");
            return;
        }
    }
    if (!strcasecmp("-a", word[1])) {
        write_user(user,
                "+----------------------------------------------------------------------------+\n\n");
    } else {
        write_user(user, "Usage: system [-m|-n|-r|-u|-a]\n");
    }
}
