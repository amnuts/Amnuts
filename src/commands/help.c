
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Show the list of commands, credits, and display the helpfiles for the given command
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

