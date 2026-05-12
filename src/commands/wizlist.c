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
 * Show the wizzes that are currently logged on, and get a list of names
 * from the lists saved.
 *
 * The per-section frame lines are synthesised by rule() from the
 * ui.rule.* primitives; the catalog supplies only the section label
 * text (wizlist.label.*). A themed locale re-skins the divider caps/
 * fill by overriding the ui.* keys, with no command-specific overrides
 * required.
 */
void
wiz_list(UR_OBJECT user)
{
    static const char *const clrs[] ={"~FC", "~FM", "~FG", "~FB", "~OL", "~FR", "~FY"};
    char text2[ARR_SIZE];
    char temp[ARR_SIZE];
    char lvl_padded[16];
    char name_padded[USER_NAME_LEN + 8];
    char text2_padded[ARR_SIZE];
    char rname_padded[ARR_SIZE];
    UR_OBJECT u;
    UD_OBJECT entry;
    int invis, count, inlist;
    int linecnt, rnamecnt;
    enum lvl_value lvl;

    /* show this for everyone */
    rule(user, 78, lang(user, "wizlist.label.wiz_list"));
    write_user(user, "\n");
    for (lvl = GOD; lvl >= WIZ; lvl = (enum lvl_value) (lvl - 1)) {
        *text2 = '\0';
        count = 0;
        inlist = 0;
        snprintf(lvl_padded, sizeof lvl_padded, "%-10s", user_level[lvl].name);
        snprintf(text, sizeof text, "~OL%s%s~RS : ", clrs[lvl % 4], lvl_padded);
        for (entry = first_user_entry; entry; entry = entry->next) {
            if (entry->level < WIZ) {
                continue;
            }
            if (is_retired(entry->name)) {
                continue;
            }
            if (entry->level == lvl) {
                if (count > 3) {
                    strcat(text2, "\n             ");
                    count = 0;
                }
                snprintf(name_padded, sizeof name_padded, "%-*s",
                         USER_NAME_LEN, entry->name);
                snprintf(temp, sizeof temp, "~OL%s%s~RS  ",
                         clrs[rand() % 7], name_padded);
                strcat(text2, temp);
                ++count;
                inlist = 1;
            }
        }
        if (!count && !inlist) {
            lang_format(user, text2, sizeof text2, "wizlist.none_listed");
        }
        strcat(text, text2);
        write_user(user, text);
        if (count) {
            write_user(user, "\n");
        }
    }

    /* show this to just the wizzes */
    if (user->level >= WIZ) {
        write_user(user, "\n");
        rule(user, 78, lang(user, "wizlist.label.retired"));
        write_user(user, "\n");
        for (lvl = GOD; lvl >= WIZ; lvl = (enum lvl_value) (lvl - 1)) {
            *text2 = '\0';
            count = 0;
            inlist = 0;
            snprintf(lvl_padded, sizeof lvl_padded, "%-10s",
                     user_level[lvl].name);
            snprintf(text, sizeof text, "~OL%s%s~RS : ",
                     clrs[lvl % 4], lvl_padded);
            for (entry = first_user_entry; entry; entry = entry->next) {
                if (entry->level < WIZ) {
                    continue;
                }
                if (!is_retired(entry->name)) {
                    continue;
                }
                if (entry->level == lvl) {
                    if (count > 3) {
                        strcat(text2, "\n             ");
                        count = 0;
                    }
                    snprintf(name_padded, sizeof name_padded, "%-*s",
                             USER_NAME_LEN, entry->name);
                    snprintf(temp, sizeof temp, "~OL%s%s~RS  ",
                             clrs[rand() % 7], name_padded);
                    strcat(text2, temp);
                    ++count;
                    inlist = 1;
                }
            }
            if (!count && !inlist) {
                lang_format(user, text2, sizeof text2, "wizlist.none_listed");
            }
            strcat(text, text2);
            write_user(user, text);
            if (count) {
                write_user(user, "\n");
            }
        }
    }
    /* show this to everyone */
    write_user(user, "\n");
    rule(user, 78, lang(user, "wizlist.label.currently_on"));
    write_user(user, "\n");
    invis = 0;
    count = 0;
    for (u = user_first; u; u = u->next)
        if (u->room) {
            if (u->level >= WIZ) {
                if (!u->vis && (user->level < u->level && !(user->level >= ARCH))) {
                    ++invis;
                    continue;
                } else {
                    if (u->vis) {
                        snprintf(text2, sizeof text2, "  %s~RS %s~RS",
                                 u->recap, u->desc);
                    } else {
                        snprintf(text2, sizeof text2, "* %s~RS %s~RS",
                                 u->recap, u->desc);
                    }
                    linecnt = 43 + teslen(text2, 43);
                    rnamecnt = 15 + teslen(u->room->show_name, 15);
                    snprintf(text2_padded, sizeof text2_padded, "%-*.*s",
                             linecnt, linecnt, text2);
                    snprintf(rname_padded, sizeof rname_padded, "%-*.*s",
                             rnamecnt, rnamecnt, u->room->show_name);
                    vwrite_user(user, "%s~RS : %s~RS : (%1.1s) %s\n",
                                text2_padded, rname_padded,
                                user_level[u->level].alias,
                                user_level[u->level].name);
                }
            }
            ++count;
        }
    if (invis) {
        lang_user(user, "wizlist.invisible_count", invis);
    }
    if (!count) {
        lang_user(user, "wizlist.no_wizzes_on");
    }
    write_user(user, "\n");
    rule(user, 78, NULL);
}
