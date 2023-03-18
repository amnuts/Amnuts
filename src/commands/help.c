
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Show the list of commands, credits, and display the help files for the given command
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
        vwrite_user(user,
                "~FR~OLCommand name is not unique. \"~FC%s~RS~OL~FR\" also matches:\n\n",
                word[1]);
        write_user(user, text);
        *text = '\0';
        return;
    }
    *text = '\0';
    if (!found) {
        write_user(user, "Sorry, there is no help on that topic.\n");
        return;
    }
    if (word_count < 3) {
        sprintf(filename, "%s/%s", HELPFILES, com->name);
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
                vwrite_user(user,
                        "~FR~OLAttribute name is not unique. \"~FT%s~RS~OL~FR\" also matches:\n",
                        word[2]);
                write_user(user, text);
                *text = '\0';
                return;
            }
            *text = '\0';
            if (!found) {
                write_user(user, "Sorry, there is no help on that topic.\n");
                return;
            }
            if (word_count < 4) {
                sprintf(filename, "%s/%s_%s", HELPFILES, com->name, attr->type);
            } else {
                sprintf(filename, "%s/%s", HELPFILES, com->name);
            }
        } else {
            com = command_table + HELP;
            sprintf(filename, "%s/%s", HELPFILES, com->name);
        }
    }
    switch (more(user, user->socket, filename)) {
    case 0:
        write_user(user, "Sorry, there is no help on that topic.\n");
        break;
    case 1:
        user->misc_op = 2;
        break;
    case 2:
        /* FIXME: take into account xgcoms, command list dynamic level, etc. */
        vwrite_user(user, "~OLLevel   :~RS %s and above\n",
                user_level[com->level].name);
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
    write_user(user,
               "\n+----------------------------------------------------------------------------+\n");
    write_user(user,
               "| All commands start with a \".\" (when in ~FYspeech~RS mode) and can be abbreviated |\n");
    write_user(user,
               "| Remember, a \".\" by itself will repeat your last command or speech          |\n");
    write_user(user,
               "+----------------------------------------------------------------------------+\n");
    write_user(user,
               align_string(ALIGN_CENTRE, 78, 1, "|",
                            "  Commands available to you (level ~OL%s~RS) ",
                            user_level[user->level].name));
    write_user(user,
               "+----------------------------------------------------------------------------+\n");
    total = 0;
    for (lvl = JAILED; lvl < NUM_LEVELS; lvl = (enum lvl_value) (lvl + 1)) {
        if (user->level < lvl) {
            break;
        }
        cnt = 0;
        *text = '\0';
        sprintf(text, "  ~FG~OL%-1.1s)~RS ~FC", user_level[lvl].name);
        highlight = 1;
        /* scroll through all commands, format and print */
        for (cmd = first_command; cmd; cmd = cmd->next) {
            temp1 = sdsempty();
            if (cmd->level != lvl) {
                continue;
            }
            if (has_xcom(user, cmd->id)) {
                temp1 = sdscatfmt(sdsempty(), "~FR%s~RS%s %s", cmd->name, highlight ? "~FC" : "",
                                  cmd->alias);
            } else {
                temp1 = sdscatfmt(sdsempty(), "%s %s", cmd->name, cmd->alias);
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
    write_user(user,
               "+----------------------------------------------------------------------------+\n");
    write_user(user,
               align_string(ALIGN_LEFT, 78, 1, "|",
                            "  There is a total of ~OL%d~RS command%s that you can use ",
                            total, PLTEXT_S(total)));
    write_user(user,
               "+----------------------------------------------------------------------------+\n");
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
    write_user(user,
               "\n+----------------------------------------------------------------------------+\n");
    write_user(user,
               "| All commands start with a \".\" (when in ~FYspeech~RS mode) and can be abbreviated |\n");
    write_user(user,
               "| Remember, a \".\" by itself will repeat your last command or speech          |\n");
    write_user(user,
               "+----------------------------------------------------------------------------+\n");
    write_user(user,
               align_string(ALIGN_CENTRE, 78, 1, "|",
                            "  Commands available to you (level ~OL%s~RS) ",
                            user_level[user->level].name));
    write_user(user,
               "+----------------------------------------------------------------------------+\n");
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
                           align_string(ALIGN_LEFT, 78, 1, "|", "  ~OL~FG%s~RS ",
                                        command_types[function]));
                strcpy(text, "     ");
            }
            if (has_xcom(user, cmd->id)) {
                temp1 = sdscatfmt(sdsempty(), "~FR%s~RS %s", cmd->name, cmd->alias);
            } else {
                temp1 = sdscatfmt(sdsempty(), "%s %s", cmd->name, cmd->alias);
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
    write_user(user,
               "+----------------------------------------------------------------------------+\n");
    write_user(user,
               align_string(ALIGN_LEFT, 78, 1, "|",
                            "  There is a total of ~OL%d~RS command%s that you can use ",
                            total, PLTEXT_S(total)));
    write_user(user,
               "+----------------------------------------------------------------------------+\n");
    stop_pager(user);
}

/*
 * Show NUTS credits
 */
void
help_nuts_credits(UR_OBJECT user)
{
    write_user(user,
               "\n~BB*** NUTS Credits :) (for Amnuts credits, see \".help credits\") ***\n\n");
    vwrite_user(user,
                "~BRNUTS version %s, Copyright (C) Neil Robertson 1996.\n\n",
                NUTSVER);
    write_user(user,
               "~BM             ~BB             ~BC             ~BG             ~BY             ~BR             \n");
    write_user(user,
               "NUTS stands for Neil's Unix Talk Server, a program which started out as a\n");
    write_user(user,
               "university project in autumn 1992 and has progressed from thereon. In no\n");
    write_user(user,
               "particular order thanks go to the following people who helped me develop or\n");
    write_user(user, "debug this code in one way or another over the years:\n");
    write_user(user,
               "   ~FCDarren Seryck, Steve Guest, Dave Temple, Satish Bedi, Tim Bernhardt,\n");
    write_user(user,
               "   ~FCKien Tran, Jesse Walton, Pak Chan, Scott MacKenzie and Bryan McPhail.\n");
    write_user(user,
               "Also thanks must go to anyone else who has emailed me with ideas and/or bug\n");
    write_user(user,
               "reports and all the people who have used NUTS over the intervening years.\n");
    write_user(user,
               "I know I have said this before but this time I really mean it--this is the final\n");
    write_user(user,
               "version of NUTS 3. In a few years NUTS 4 may spring forth but in the meantime\n");
    write_user(user, "that, as they say, is that. :)\n\n");
    write_user(user,
               "If you wish to email me my address is \"~FGneil@ogham.demon.co.uk~RS\" and should\n");
    write_user(user,
               "remain so for the forseeable future.\n\nNeil Robertson - November 1996.\n");
    write_user(user,
               "~BM             ~BB             ~BC             ~BG             ~BY             ~BR             \n\n");
}

/*
 * Show the credits. Add your own credits here if you wish but PLEASE leave
 * my credits intact. Thanks.
 */
void
help_amnuts_credits(UR_OBJECT user)
{
    write_user(user,
               "~BM             ~BB             ~BC             ~BG             ~BY             ~BR             \n\n");
    vwrite_user(user,
                "~OL~FCAmnuts version %s~RS, Copyright (C) Andrew Collington, 2003\n",
                AMNUTSVER);
    write_user(user,
               "Brought to you by the Amnuts Development Group (Andy, Ardant and Uzume)\n\n");
    write_user(user,
               "Amnuts stands for ~OLA~RSndy's ~OLM~RSodified ~OLNUTS~RS, a Unix talker server written in C.\n\n");
    write_user(user,
               "Many thanks to everyone who has helped out with Amnuts.  Special thanks go to\n");
    write_user(user,
               "Ardant, Uzume, Arny (of Paris fame), Silver (of PG+ fame), and anyone else who\n");
    write_user(user,
               "has contributed at all to the development of Amnuts.\n\n");
    write_user(user,
               "If you are interested, you can purchase Amnuts t-shirts, mugs, mousemats, and\n");
    write_user(user,
               "more, from http://www.cafepress.com/amnuts/\n\nWe hope you enjoy the talker!\n\n");
    write_user(user,
               "   -- The Amnuts Development Group\n\n(for NUTS credits, see \".help nuts\")\n");
    write_user(user,
               "\n~BM             ~BB             ~BC             ~BG             ~BY             ~BR             \n\n");
}
