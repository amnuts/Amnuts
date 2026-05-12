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
 * Show talker system parameters etc.
 *
 * Phase 4 conversion: the most prose-heavy of the frame-heavy commands.
 * Five sections (header, users, netlinks, rooms, memory) ship through one
 * box_open/box_line/box_separator/box_close pipeline at 78 visible cols
 * total (inner = 76). When `-a` is given, all five sections render as a
 * single chained box; for `-u`/`-n`/`-r`/`-m`, only the relevant section
 * is drawn (still inside a freshly-opened-and-closed box of its own).
 *
 * Two divergences from the standard box flow:
 *
 *  1. After each section's title row, the original emits an *inner* `|---|`
 *     separator with `|` caps, not the `+---+` that box_separator produces.
 *     That separator therefore ships as the opaque catalog literal
 *     `system.inner_sep` and is written direct via write_user — it cannot
 *     reuse the Phase 3 separator primitive without changing bytes.
 *
 *  2. Section A's "port info" header has 8 NETLINKS x IDENTD-state x
 *     WIZPORT variants; each ships as a (label, value) pair of keys.
 *     `#ifdef` gating mirrors the pre-conversion arrangement exactly.
 *
 * Every server-authored literal — labels, titles, the inner separator,
 * the trailing "For other options" footer, the "Netlinks not compiled"
 * notice, and the usage line — lives in system.* keys in
 * files/langs/en_GB/strings.yml. Each row's body, after format expansion,
 * is at most 76 visible cols; box_line right-pads with spaces to 76, so
 * any catalog row that ends a hair short still emits byte-identically
 * to the pre-conversion vwrite_user formats (which baked the trailing
 * spaces into the format string).
 *
 * The memory section's "total" row is the one signature quirk: it mixes
 * a megabyte float (`%12.3f`) and a byte count (`%d`). The catalog
 * signature validator rejects `%f`, so the Mb value is snprintf'd to a
 * 12-char string locally and shipped through `%12s`. The visible bytes
 * are identical to the original `%12.3f` output for any non-negative
 * value that fits in 12 cols at three decimal places.
 */

/* Helper: open a new box if one is not already open, else draw an
 * inter-section `+---+` separator. Returns the (possibly new) box. */
static BOX
system_section_open(BOX box, UR_OBJECT user)
{
    if (box) {
        box_separator(box);
        return box;
    }
    return box_open(user, 78, NULL);
}

/* Helper: emit the section's title row (with leading colour escapes baked
 * into the catalog) followed by the inner `|---|` divider. The divider is
 * a raw write_user because its `|` caps cannot be produced by
 * box_separator (which always emits `+` caps). The title keys passed
 * here are static strings (no positional args), so it is safe to hand
 * the lang() return pointer straight to box_line: vsnprintf copies the
 * bytes into the box's body buffer before any other catalog operation
 * runs. */
static void
system_section_title(BOX box, UR_OBJECT user, const char *title_key)
{
    const char *t = lang(user, title_key);
    if (!t) t = "";
    box_line(box, "%s", t);
    const char *sep = lang(user, "system.inner_sep");
    if (!sep) sep = "|----------------------------------------------------------------------------|\n";
    write_user(user, sep);
}

void
system_details(UR_OBJECT user)
{
    static const char *const ca[] = {"NONE", "SHUTDOWN", "REBOOT", "SEAMLESS"};
    static const char *const rip[] = {"OFF", "AUTO", "MANUAL", "IDENTD"};
    char bstr[32];
    char title_text[ARR_SIZE];
    char title_padded[ARR_SIZE];
    char title_body[ARR_SIZE];
    char uptime_text[128];
    char heartbeat_text[64];
    char autopurge_text[64];
    char login_idle_text[32], user_idle_text[32];
    char user_purge_text[32], newbie_purge_text[32];
    char mesg_life_text[32];
    char mb_text[32];
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
    BOX box = NULL;

    /* Validate switch up front (skipped on word_count < 2 which is the
     * "all the basics" no-arg case). */
    if (word_count >= 2
        && strcasecmp("-a", word[1])
        && strcasecmp("-u", word[1])
        && strcasecmp("-n", word[1])
        && strcasecmp("-r", word[1])
        && strcasecmp("-m", word[1])) {
        lang_user(user, "system.usage");
        return;
    }

    if (word_count < 2 || !strcasecmp("-a", word[1])) {
        /* The original emits a leading "\n" before Section A's top frame
         * (Section A is shown for the no-arg case and for -a). Standalone
         * -u/-n/-r/-m do not get this leading newline. */
        write_user(user, "\n");
        /* Section A — header / general / port info / stats. */
        box = system_section_open(box, user);
        if (!box) return;

        /* Title (74-col padded inside ~OL~FC..~RS). */
        lang_format(user, title_text, sizeof title_text,
                    "system.title.text.header", TALKER_NAME, AMNUTSVER);
        snprintf(title_padded, sizeof title_padded, "%-74.74s", title_text);
        lang_format(user, title_body, sizeof title_body,
                    "system.title.header", title_padded);
        box_line(box, "%s", title_body);
        const char *sep = lang(user, "system.inner_sep");
        if (!sep) sep = "|----------------------------------------------------------------------------|\n";
        write_user(user, sep);

        /* Get uptime values. */
        strftime(bstr, 32, "%a %Y-%m-%d %H:%M:%S", localtime(&amsys->boot_time));
        secs = (int) (time(0) - amsys->boot_time);
        days = secs / 86400;
        hours = (secs % 86400) / 3600;
        mins = (secs % 3600) / 60;
        secs = secs % 60;

        /* Port-info header — 8 variants (NETLINKS x IDENTD-state x WIZPORT).
         * Each variant ships as a (label, value) pair. Both rows render via
         * box_line(box, "%s", body) so the |…| rails are produced by the
         * Phase 3 builder. lang_format with zero args copies the catalog
         * value verbatim into our local buffer (vsnprintf with no specifier
         * substitutions); this avoids holding a lang() pointer across the
         * subsequent box_line call. */
        char port_label_body[ARR_SIZE];
        char port_value_body[ARR_SIZE];
#ifdef NETLINKS
#ifdef IDENTD
        if (amsys->ident_state) {
#ifdef WIZPORT
            lang_format(user, port_label_body, sizeof port_label_body,
                        "system.ports.label.nl_id_wp");
            lang_format(user, port_value_body, sizeof port_value_body,
                        "system.ports.value.nl_id_wp",
                        getpid(), amsys->ident_pid, amsys->mport_port,
                        amsys->wport_port, amsys->nlink_port);
#else
            lang_format(user, port_label_body, sizeof port_label_body,
                        "system.ports.label.nl_id");
            lang_format(user, port_value_body, sizeof port_value_body,
                        "system.ports.value.nl_id",
                        getpid(), amsys->ident_pid, amsys->mport_port,
                        amsys->nlink_port);
#endif
            box_line(box, "%s", port_label_body);
            box_line(box, "%s", port_value_body);
        } else
#endif
        {
#ifdef WIZPORT
            lang_format(user, port_label_body, sizeof port_label_body,
                        "system.ports.label.nl_wp");
            lang_format(user, port_value_body, sizeof port_value_body,
                        "system.ports.value.nl_wp",
                        getpid(), amsys->mport_port, amsys->wport_port,
                        amsys->nlink_port);
#else
            lang_format(user, port_label_body, sizeof port_label_body,
                        "system.ports.label.nl");
            lang_format(user, port_value_body, sizeof port_value_body,
                        "system.ports.value.nl",
                        getpid(), amsys->mport_port, amsys->nlink_port);
#endif
            box_line(box, "%s", port_label_body);
            box_line(box, "%s", port_value_body);
        }
#else /* !NETLINKS */
#ifdef IDENTD
        if (amsys->ident_state) {
#ifdef WIZPORT
            lang_format(user, port_label_body, sizeof port_label_body,
                        "system.ports.label.id_wp");
            lang_format(user, port_value_body, sizeof port_value_body,
                        "system.ports.value.id_wp",
                        getpid(), amsys->ident_pid, amsys->mport_port,
                        amsys->wport_port);
#else
            lang_format(user, port_label_body, sizeof port_label_body,
                        "system.ports.label.id");
            lang_format(user, port_value_body, sizeof port_value_body,
                        "system.ports.value.id",
                        getpid(), amsys->ident_pid, amsys->mport_port);
#endif
            box_line(box, "%s", port_label_body);
            box_line(box, "%s", port_value_body);
        } else
#endif
        {
#ifdef WIZPORT
            lang_format(user, port_label_body, sizeof port_label_body,
                        "system.ports.label.wp");
            lang_format(user, port_value_body, sizeof port_value_body,
                        "system.ports.value.wp",
                        getpid(), amsys->mport_port, amsys->wport_port);
#else
            lang_format(user, port_label_body, sizeof port_label_body,
                        "system.ports.label.plain");
            lang_format(user, port_value_body, sizeof port_value_body,
                        "system.ports.value.plain",
                        getpid(), amsys->mport_port);
#endif
            box_line(box, "%s", port_label_body);
            box_line(box, "%s", port_value_body);
        }
#endif
        box_separator(box);

        /* Stat rows. */
        lang_format(user, title_body, sizeof title_body,
                    "system.row.booted", bstr);
        box_line(box, "%s", title_body);

        snprintf(uptime_text, sizeof uptime_text,
                 "%d day%s, %d hour%s, %d minute%s, %d second%s",
                 days, PLTEXT_S(days), hours, PLTEXT_S(hours),
                 mins, PLTEXT_S(mins), secs, PLTEXT_S(secs));
        lang_format(user, title_body, sizeof title_body,
                    "system.row.uptime", uptime_text);
        box_line(box, "%s", title_body);

        lang_format(user, title_body, sizeof title_body,
                    "system.row.logging_flood",
                    offon[(amsys->logging) ? 1 : 0],
                    offon[amsys->flood_protect]);
        box_line(box, "%s", title_body);

        lang_format(user, title_body, sizeof title_body,
                    "system.row.sigterms_crash",
                    noyes[amsys->ignore_sigterm],
                    ca[amsys->crash_action]);
        box_line(box, "%s", title_body);

        snprintf(heartbeat_text, sizeof heartbeat_text, "every %d sec%s",
                 amsys->heartbeat, PLTEXT_S(amsys->heartbeat));
        lang_format(user, title_body, sizeof title_body,
                    "system.row.heartbeat_resolve",
                    heartbeat_text, rip[amsys->resolve_ip]);
        box_line(box, "%s", title_body);

        lang_format(user, title_body, sizeof title_body,
                    "system.row.swear_ban", minmax[amsys->ban_swearing]);
        box_line(box, "%s", title_body);

        if (word_count < 2) {
            /* No-arg: emit "For other options" footer mini-section and
             * close the frame. */
            box_separator(box);
            const char *foo_msg = lang(user, "system.row.for_other_options");
            box_line(box, "%s", foo_msg ? foo_msg : "");
            box_close(box);
            write_user(user, "\n");
            return;
        }
    }

    /* Section U — user stats. */
    if (word_count >= 2
        && (!strcasecmp("-u", word[1]) || !strcasecmp("-a", word[1]))) {
        uccount = 0;
        for (u = user_first; u; u = u->next) {
            if (u->type == CLONE_TYPE) {
                ++uccount;
            }
        }
        box = system_section_open(box, user);
        if (!box) return;
        system_section_title(box, user, "system.title.users");

        for (lvl = JAILED; lvl < NUM_LEVELS;
             lvl = (enum lvl_value) (lvl + 1)) {
            lang_format(user, title_body, sizeof title_body,
                        "system.row.users_at_level",
                        user_level[lvl].name, amsys->level_count[lvl]);
            box_line(box, "%s", title_body);
        }
        box_separator(box);

        lang_format(user, title_body, sizeof title_body,
                    "system.row.online_max",
                    amsys->num_of_users, amsys->max_users);
        box_line(box, "%s", title_body);

        lang_format(user, title_body, sizeof title_body,
                    "system.row.new_returning",
                    amsys->logons_new, amsys->logons_old);
        box_line(box, "%s", title_body);

        lang_format(user, title_body, sizeof title_body,
                    "system.row.clones_max", uccount, amsys->max_clones);
        box_line(box, "%s", title_body);

        snprintf(login_idle_text, sizeof login_idle_text, "%d sec%s",
                 amsys->login_idle_time, PLTEXT_S(amsys->login_idle_time));
        snprintf(user_idle_text, sizeof user_idle_text, "%d sec%s",
                 amsys->user_idle_time, PLTEXT_S(amsys->user_idle_time));
        lang_format(user, title_body, sizeof title_body,
                    "system.row.idle_timeouts",
                    login_idle_text, user_idle_text);
        box_line(box, "%s", title_body);

        lang_format(user, title_body, sizeof title_body,
                    "system.row.timeout_maxlevel_afks",
                    user_level[amsys->time_out_maxlevel].name,
                    noyes[amsys->time_out_afks]);
        box_line(box, "%s", title_body);

        lang_format(user, title_body, sizeof title_body,
                    "system.row.minlogin",
                    amsys->minlogin_level == NUM_LEVELS
                        ? "NONE"
                        : user_level[amsys->minlogin_level].name,
                    noyes[amsys->boot_off_min]);
        box_line(box, "%s", title_body);

        lang_format(user, title_body, sizeof title_body,
                    "system.row.newbie_prompt_colour",
                    offon[amsys->prompt_def], offon[amsys->colour_def]);
        box_line(box, "%s", title_body);

        lang_format(user, title_body, sizeof title_body,
                    "system.row.newbie_echo",
                    offon[amsys->charecho_def],
                    offon[amsys->passwordecho_def]);
        box_line(box, "%s", title_body);

        lang_format(user, title_body, sizeof title_body,
                    "system.row.recaps_forwarding",
                    noyes[amsys->allow_recaps], offon[amsys->forwarding]);
        box_line(box, "%s", title_body);

        strftime(autopurge_text, sizeof autopurge_text,
                 "%a %Y-%m-%d %H:%M:%S", localtime(&amsys->auto_purge_date));
        lang_format(user, title_body, sizeof title_body,
                    "system.row.autopurge",
                    noyes[amsys->auto_purge_date != -1], autopurge_text);
        box_line(box, "%s", title_body);

        snprintf(user_purge_text, sizeof user_purge_text, "%d day%s",
                 USER_EXPIRES, PLTEXT_S(USER_EXPIRES));
        snprintf(newbie_purge_text, sizeof newbie_purge_text, "%d day%s",
                 NEWBIE_EXPIRES, PLTEXT_S(NEWBIE_EXPIRES));
        lang_format(user, title_body, sizeof title_body,
                    "system.row.purge_lengths",
                    newbie_purge_text, user_purge_text);
        box_line(box, "%s", title_body);

#ifdef WIZPORT
        lang_format(user, title_body, sizeof title_body,
                    "system.row.wizport_level",
                    user_level[amsys->wizport_level].name);
        box_line(box, "%s", title_body);
#endif

        if (!strcasecmp("-u", word[1])) {
            box_close(box);
            write_user(user, "\n");
            return;
        }
    }

    /* Section N — netlinks. */
    if (word_count >= 2
        && (!strcasecmp("-n", word[1]) || !strcasecmp("-a", word[1]))) {
        box = system_section_open(box, user);
        if (!box) return;
        system_section_title(box, user, "system.title.netlinks");
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
        lang_format(user, title_body, sizeof title_body,
                    "system.row.total_netlinks", nlcount, " ");
        box_line(box, "%s", title_body);

        lang_format(user, title_body, sizeof title_body,
                    "system.row.idle_keepalive",
                    amsys->net_idle_time, amsys->keepalive_interval);
        box_line(box, "%s", title_body);

        lang_format(user, title_body, sizeof title_body,
                    "system.row.accept_live", rmnlicount, nlupcount);
        box_line(box, "%s", title_body);

        lang_format(user, title_body, sizeof title_body,
                    "system.row.incoming_outgoing", nlicount, nlocount);
        box_line(box, "%s", title_body);

        lang_format(user, title_body, sizeof title_body,
                    "system.row.rem_user_levels",
                    user_level[amsys->rem_user_maxlevel].name,
                    user_level[amsys->rem_user_deflevel].name);
        box_line(box, "%s", title_body);

        lang_format(user, title_body, sizeof title_body,
                    "system.row.netlink_memory",
                    (int) (sizeof *nl), nlcount * (int) (sizeof *nl));
        box_line(box, "%s", title_body);
#else
        const char *nlmsg = lang(user, "system.row.netlinks_not_compiled");
        box_line(box, "%s", nlmsg ? nlmsg : "");
#endif
        if (!strcasecmp("-n", word[1])) {
            box_close(box);
            write_user(user, "\n");
            return;
        }
    }

    /* Section R — rooms. */
    if (word_count >= 2
        && (!strcasecmp("-r", word[1]) || !strcasecmp("-a", word[1]))) {
        rmcount = rmpcount = 0;
        for (rm = room_first; rm; rm = rm->next) {
            ++rmcount;
            if (is_personal_room(rm)) {
                ++rmpcount;
            }
        }
        box = system_section_open(box, user);
        if (!box) return;
        system_section_title(box, user, "system.title.rooms");

        lang_format(user, title_body, sizeof title_body,
                    "system.row.gatecrash_private",
                    user_level[amsys->gatecrash_level].name,
                    amsys->min_private_users);
        box_line(box, "%s", title_body);

        snprintf(mesg_life_text, sizeof mesg_life_text, "%d day%s",
                 amsys->mesg_life, PLTEXT_S(amsys->mesg_life));
        lang_format(user, title_body, sizeof title_body,
                    "system.row.mesg_life_check",
                    mesg_life_text,
                    amsys->mesg_check_hour, amsys->mesg_check_min);
        box_line(box, "%s", title_body);

        lang_format(user, title_body, sizeof title_body,
                    "system.row.rooms_count", rmpcount, rmcount);
        box_line(box, "%s", title_body);

        lang_format(user, title_body, sizeof title_body,
                    "system.row.rooms_memory",
                    (int) (sizeof *rm), rmcount * (int) (sizeof *rm));
        box_line(box, "%s", title_body);

        if (!strcasecmp("-r", word[1])) {
            box_close(box);
            write_user(user, "\n");
            return;
        }
    }

    /* Section M — memory. */
    if (word_count >= 2
        && (!strcasecmp("-m", word[1]) || !strcasecmp("-a", word[1]))) {
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
        tsize = ucount * (sizeof *u) + dcount * (sizeof *d)
                + rmcount * (sizeof *rm) + cmdcount * (sizeof *cmd)
                + (sizeof *amsys) + lcount * (sizeof *last_login_info);
#ifdef NETLINKS
        nlcount = 0;
        for (nl = nl_first; nl; nl = nl->next) {
            ++nlcount;
        }
        tsize += nlcount * (sizeof *nl);
#endif
        box = system_section_open(box, user);
        if (!box) return;
        system_section_title(box, user, "system.title.memory");

        lang_format(user, title_body, sizeof title_body,
                    "system.row.mem_object",
                    "users", ucount, (int) (sizeof *u),
                    ucount * (int) (sizeof *u));
        box_line(box, "%s", title_body);

        lang_format(user, title_body, sizeof title_body,
                    "system.row.mem_object",
                    "all users", dcount, (int) (sizeof *d),
                    dcount * (int) (sizeof *d));
        box_line(box, "%s", title_body);

        lang_format(user, title_body, sizeof title_body,
                    "system.row.mem_object",
                    "rooms", rmcount, (int) (sizeof *rm),
                    rmcount * (int) (sizeof *rm));
        box_line(box, "%s", title_body);

        lang_format(user, title_body, sizeof title_body,
                    "system.row.mem_object",
                    "commands", cmdcount, (int) (sizeof *cmd),
                    cmdcount * (int) (sizeof *cmd));
        box_line(box, "%s", title_body);

        lang_format(user, title_body, sizeof title_body,
                    "system.row.mem_object",
                    "system", 1, (int) (sizeof *amsys),
                    1 * (int) (sizeof *amsys));
        box_line(box, "%s", title_body);

        lang_format(user, title_body, sizeof title_body,
                    "system.row.mem_object",
                    "last logins", lcount, (int) (sizeof *last_login_info),
                    lcount * (int) (sizeof *last_login_info));
        box_line(box, "%s", title_body);
#ifdef NETLINKS
        lang_format(user, title_body, sizeof title_body,
                    "system.row.mem_object",
                    "netlinks", nlcount, (int) (sizeof *nl),
                    nlcount * (int) (sizeof *nl));
        box_line(box, "%s", title_body);
#endif
        box_separator(box);
        /* Mb value is %f in the original; catalog signature validator
         * rejects %f, so pre-format to a 12-char string and ship as %s. */
        snprintf(mb_text, sizeof mb_text, "%12.3f", tsize / 1048576.0);
        lang_format(user, title_body, sizeof title_body,
                    "system.row.mem_total", mb_text, tsize);
        box_line(box, "%s", title_body);

        /* -m or -a both fall through to the final close below. */
    }

    /* Final close for -a (and standalone -m, which falls through here). */
    if (box) {
        box_close(box);
        write_user(user, "\n");
    }
}
