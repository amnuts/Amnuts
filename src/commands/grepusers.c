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
 * This command allows you to do a search for any user names that match
 * a particular pattern.
 *
 * Phase 4 conversion: every server-authored literal lives in grepusers.*
 * in files/langs/en_GB/strings.yml. The 78-col frame, the title row, the
 * empty-state rows, and the trailing summary line are drawn through the
 * Phase 3 box_* pipeline (inner = 76 visible cols), and the four early-
 * exit messages (usage + three bad-pattern warnings) go through
 * write_user_lang.
 *
 * The per-match body rows are NOT handed to box_line: the original
 * layout deliberately overruns the 78-col frame (a paired row is
 * 86 visible cols between the rails; an orphan first half is padded
 * to 82). Reproducing that byte-identically through box_line would
 * mean lying about the inner width per row, so the row pieces are
 * assembled inline and handed to write_user directly. The left/right
 * rail bytes are pulled from ui.box.body.{lside,rside} so themed
 * locales still re-skin the rails consistently. Names and level
 * labels are pre-padded with snprintf.
 */
void
grep_users(UR_OBJECT user)
{
    int found, x;
    char name[USER_NAME_LEN + 1], pat[ARR_SIZE];
    char name_padded[USER_NAME_LEN + 8];
    char level_padded[32];
    char pat_padded[64];
    char title_body[ARR_SIZE];
    char row_part[ARR_SIZE];
    char row_buf[ARR_SIZE * 2];
    BOX box;
    UD_OBJECT entry;

    if (word_count < 2) {
        write_user_lang(user, "grepusers.usage");
        return;
    }
    if (strstr(word[1], "**")) {
        write_user_lang(user, "grepusers.bad_double_star");
        return;
    }
    if (strstr(word[1], "?*")) {
        write_user_lang(user, "grepusers.bad_question_star");
        return;
    }
    if (strstr(word[1], "*?")) {
        write_user_lang(user, "grepusers.bad_star_question");
        return;
    }
    start_pager(user);
    write_user(user, "\n");
    box = box_open(user, 78, NULL);
    if (!box) {
        stop_pager(user);
        return;
    }
    snprintf(pat_padded, sizeof pat_padded, "%-51s", word[1]);
    lang_format(user, title_body, sizeof title_body,
                "grepusers.title", pat_padded);
    box_line(box, "%s", title_body);
    box_separator(box);
    x = 0;
    found = 0;
    *pat = '\0';
    strcpy(pat, word[1]);
    strtolower(pat);
    *row_buf = '\0';
    for (entry = first_user_entry; entry; entry = entry->next) {
        strcpy(name, entry->name);
        *name = tolower(*name);
        if (pattern_match(name, pat)) {
            snprintf(name_padded, sizeof name_padded, "%-*s",
                     USER_NAME_LEN, entry->name);
            snprintf(level_padded, sizeof level_padded, "%-20s",
                     user_level[entry->level].name);
            if (!x) {
                const char *lside = lang(user, "ui.box.body.lside");
                if (!lside) lside = "|";
                snprintf(row_part, sizeof row_part,
                         "%s %s  ~FC%s~RS   ",
                         lside, name_padded, level_padded);
                strcpy(row_buf, row_part);
            } else {
                const char *rside = lang(user, "ui.box.body.rside");
                if (!rside) rside = "|";
                snprintf(row_part, sizeof row_part,
                         "   %s  ~FC%s~RS %s\n",
                         name_padded, level_padded, rside);
                strcat(row_buf, row_part);
                write_user(user, row_buf);
                *row_buf = '\0';
            }
            x = !x;
            ++found;
        }
    }
    if (x) {
        const char *rside = lang(user, "ui.box.body.rside");
        if (!rside) rside = "|";
        char pad[64];
        snprintf(pad, sizeof pad,
                 "                                      %s\n", rside);
        strcat(row_buf, pad);
        write_user(user, row_buf);
    }
    if (!found) {
        box_blank(box);
        const char *empty = lang(user, "grepusers.empty_message");
        if (!empty) empty = " ~OL~FRNo users have that pattern~RS";
        box_line(box, "%s", empty);
        box_blank(box);
        box_close(box);
        stop_pager(user);
        return;
    }
    box_separator(box);
    char footer_body[ARR_SIZE];
    lang_format(user, footer_body, sizeof footer_body,
                "grepusers.footer",
                found, PLTEXT_S(found), word[1]);
    box_line(box, "%s", footer_body);
    box_close(box);
    write_user(user, "\n");
    stop_pager(user);
}
