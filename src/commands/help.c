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
 * Show the list of commands, credits, and display the help files for the
 * given command.
 *
 * Phase 4 conversion: only the *framing* parts of the command are lifted
 * into the catalog — the unknown-topic / ambiguous-match messages, the
 * commands-list screens (level- and function-grouped), and the credits
 * screens. The per-topic helpfile path (the `more()` call against
 * files/langs/<locale>/helpfiles/<topic>) is OUT OF SCOPE: that path is
 * already locale-aware from Phase 1 and is paged through the user's
 * pager, so it is left untouched.
 *
 * The commands-list screens keep the existing align_string() pipeline.
 * The plan picked approach (a) for help (catalog-supplied format string
 * fed straight into align_string with `"|"` as the marker) because
 * align_string already produces the byte stream the original sprintf
 * chain emitted — switching to box_open/box_line would risk subtle
 * padding differences at the rails. A themed locale can re-skin each
 * row's inner content via the help.commands.* keys; the rail geometry
 * itself is fixed by the C side.
 */
void
help(UR_OBJECT user)
{
    char filename[80];
    const struct cmd_entry *com, *c;
    size_t len;
    int found;

    if (word_count < 2 || !strcasecmp(word[1], "commands")) {
        switch (user->cmd_type) {
        case 0:
            help_commands_level(user);
            break;
        case 1:
            help_commands_function(user);
            break;
        }
        return;
    }
    if (!strcasecmp(word[1], "credits")) {
        help_amnuts_credits(user);
        return;
    }
    if (!strcasecmp(word[1], "nuts")) {
        help_nuts_credits(user);
        return;
    }
    len = strlen(word[1]);
    com = NULL;
    found = 0;
    *text = '\0';
    for (c = command_table; c->name; ++c) {
        if (!strncasecmp(c->name, word[1], len)) {
            if (strlen(c->name) == len) {
                break;
            }
            /* FIXME: take into account xgcoms, command list dynamic level, etc. */
            if (user->level < (enum lvl_value) c->level) {
                continue;
            }
            strcat(text, found++ % 8 ? "  " : "\n  ~OL");
            strcat(text, c->name);
            com = c;
        }
    }
    if (c->name) {
        found = 1;
        com = c;
    }
    if (found > 1) {
        strcat(text, found % 8 ? "\n\n" : "\n");
        lang_user(user, "help.ambiguous_command", word[1]);
        write_user(user, text);
        *text = '\0';
        return;
    }
    *text = '\0';
    if (!found) {
        lang_user(user, "help.unknown");
        return;
    }
    if (word_count < 3) {
        locale_path(user, filename, sizeof filename, HELPFILES, com->name);
    } else {
        if (com == command_table + SET) {
            const struct set_entry *attr, *a;

            len = strlen(word[2]);
            attr = NULL;
            found = 0;
            *text = '\0';
            for (a = setstr; a->type; ++a) {
                if (!strncasecmp(a->type, word[2], len)) {
                    if (strlen(a->type) == len) {
                        break;
                    }
                    strcat(text, found++ % 8 ? "  " : "\n  ~OL");
                    strcat(text, a->type);
                    attr = a;
                }
            }
            if (a->type) {
                found = 1;
                attr = a;
            }
            if (found > 1) {
                strcat(text, found % 8 ? "\n\n" : "\n");
                lang_user(user, "help.ambiguous_attribute", word[2]);
                write_user(user, text);
                *text = '\0';
                return;
            }
            *text = '\0';
            if (!found) {
                lang_user(user, "help.unknown");
                return;
            }
            if (word_count < 4) {
                char helpname[WORD_LEN * 2 + 2];
                snprintf(helpname, sizeof helpname, "%s_%s", com->name, attr->type);
                locale_path(user, filename, sizeof filename, HELPFILES, helpname);
            } else {
                locale_path(user, filename, sizeof filename, HELPFILES, com->name);
            }
        } else {
            com = command_table + HELP;
            locale_path(user, filename, sizeof filename, HELPFILES, com->name);
        }
    }
    switch (more(user, user->socket, filename)) {
    case 0:
        lang_user(user, "help.unknown");
        break;
    case 1:
        user->misc_op = 2;
        break;
    case 2:
        /* FIXME: take into account xgcoms, command list dynamic level, etc. */
        lang_user(user, "help.level_line", user_level[com->level].name);
        break;
    }
}


/*
 * Show the command available listed by level
 */
void
help_commands_level(UR_OBJECT user)
{
    int cnt, total, highlight;
    enum lvl_value lvl;
    sds temp, temp1;
    CMD_OBJECT cmd;

    start_pager(user);
    write_user(user, "\n");
    rule(user, 78, NULL);
    lang_user(user, "help.commands.tip_line1");
    lang_user(user, "help.commands.tip_line2");
    rule(user, 78, NULL);
    write_user(user,
               align_string(ALIGN_CENTRE, 78, 1, "|",
                            lang(user, "help.commands.title"),
                            user_level[user->level].name));
    rule(user, 78, NULL);
    total = 0;
    for (lvl = JAILED; lvl < NUM_LEVELS; lvl = (enum lvl_value) (lvl + 1)) {
        if (user->level < lvl) {
            break;
        }
        cnt = 0;
        *text = '\0';
        lang_format(user, text, sizeof text, "help.commands.level_prefix",
                    user_level[lvl].name);
        highlight = 1;
        /* scroll through all commands, format and print */
        for (cmd = first_command; cmd; cmd = cmd->next) {
            temp1 = sdsempty();
            if (cmd->level != lvl) {
                continue;
            }
            {
                char cell[ARR_SIZE];
                if (has_xcom(user, cmd->id)) {
                    lang_format(user, cell, sizeof cell,
                                "help.commands.row.level_xcom",
                                cmd->name, highlight ? "~FC" : "", cmd->alias);
                } else {
                    lang_format(user, cell, sizeof cell,
                                "help.commands.row.level_plain",
                                cmd->name, cmd->alias);
                }
                temp1 = sdscat(sdsempty(), cell);
            }
            if (++cnt == 5) {
                strcat(text, temp1);
                strcat(text, "~RS");
                write_user(user, align_string(ALIGN_LEFT, 78, 1, "|", "%s", text));
                cnt = 0;
                highlight = 0;
                *text = '\0';
            } else {
                temp = sdscatprintf(sdsempty(), "%-*s  ", 11 + (int) teslen(temp1, 0), temp1);
                strcat(text, temp);
                sdsfree(temp);
            }
            if (!cnt) {
                strcat(text, "     ");
            }
            sdsfree(temp1);
        }
        if (cnt > 0 && cnt < 5)
            write_user(user, align_string(ALIGN_LEFT, 78, 1, "|", "%s", text));
    }
    /* count up total number of commands for user level */
    for (cmd = first_command; cmd; cmd = cmd->next) {
        if (cmd->level > user->level) {
            continue;
        }
        ++total;
    }
    rule(user, 78, NULL);
    write_user(user,
               align_string(ALIGN_LEFT, 78, 1, "|",
                            lang(user, "help.commands.total"),
                            total, PLTEXT_S(total)));
    rule(user, 78, NULL);
    stop_pager(user);
}

/*
 * Show the command available listed by function
 */
void
help_commands_function(UR_OBJECT user)
{
    sds temp, temp1;
    CMD_OBJECT cmd;
    int cnt, total, function, found;

    start_pager(user);
    write_user(user, "\n");
    rule(user, 78, NULL);
    lang_user(user, "help.commands.tip_line1");
    lang_user(user, "help.commands.tip_line2");
    rule(user, 78, NULL);
    write_user(user,
               align_string(ALIGN_CENTRE, 78, 1, "|",
                            lang(user, "help.commands.title"),
                            user_level[user->level].name));
    rule(user, 78, NULL);
    /* scroll through all the commands listing by function */
    total = 0;
    for (function = 0; command_types[function]; ++function) {
        cnt = 0;
        found = 0;
        *text = '\0';
        /* scroll through all commands, format and print */
        for (cmd = first_command; cmd; cmd = cmd->next) {
            temp1 = sdsempty();
            if (cmd->level > user->level || cmd->function != function) {
                continue;
            }
            if (!found++) {
                write_user(user,
                           align_string(ALIGN_LEFT, 78, 1, "|",
                                        lang(user, "help.commands.function_header"),
                                        command_types[function]));
                strcpy(text, "     ");
            }
            {
                char cell[ARR_SIZE];
                if (has_xcom(user, cmd->id)) {
                    lang_format(user, cell, sizeof cell,
                                "help.commands.row.function_xcom",
                                cmd->name, cmd->alias);
                } else {
                    lang_format(user, cell, sizeof cell,
                                "help.commands.row.function_plain",
                                cmd->name, cmd->alias);
                }
                temp1 = sdscat(sdsempty(), cell);
            }
            if (++cnt == 5) {
                strcat(text, temp1);
                strcat(text, "~RS");
                write_user(user, align_string(ALIGN_LEFT, 78, 1, "|", "%s", text));
                cnt = 0;
                *text = '\0';
            } else {
                temp = sdscatprintf(sdsempty(), "%-*s  ", 11 + (int) teslen(temp1, 0), temp1);
                strcat(text, temp);
                sdsfree(temp);
            }
            if (!cnt) {
                strcat(text, "     ");
            }
            sdsfree(temp1);
        }
        if (cnt > 0 && cnt < 5)
            write_user(user, align_string(ALIGN_LEFT, 78, 1, "|", "%s", text));
    }
    /* count up total number of commands for user level */
    for (cmd = first_command; cmd; cmd = cmd->next) {
        if (cmd->level > user->level) {
            continue;
        }
        ++total;
    }
    rule(user, 78, NULL);
    write_user(user,
               align_string(ALIGN_LEFT, 78, 1, "|",
                            lang(user, "help.commands.total"),
                            total, PLTEXT_S(total)));
    rule(user, 78, NULL);
    stop_pager(user);
}

/*
 * Show NUTS credits
 */
void
help_nuts_credits(UR_OBJECT user)
{
    lang_user(user, "help.credits.nuts.header");
    lang_user(user, "help.credits.nuts.version", NUTSVER);
    lang_user(user, "help.credits.nuts.body");
}

/*
 * Show the credits. Add your own credits here if you wish but PLEASE leave
 * my credits intact. Thanks.
 */
void
help_amnuts_credits(UR_OBJECT user)
{
    write_user(user, "~BM             ~BB             ~BC             ~BG             ~BY             ~BR             \n\n");
    lang_user(user, "help.credits.amnuts.version", AMNUTSVER);
    lang_user(user, "help.credits.amnuts.body");
}
