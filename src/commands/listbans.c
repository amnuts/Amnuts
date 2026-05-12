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
 * List banned sites or users.
 *
 * Phase 4 conversion: every server-authored literal in this command lives
 * in the listbans.* keys of files/langs/en_GB/strings.yml. Each subcommand
 * (sites, users, swears, new) keeps its un-railed banner — the original
 * layout was a free "~BB*** … ***" header followed by either a paged file
 * or a free-text body, no |…| body rails — so the headers ship as opaque
 * catalog frame values rather than being synthesised through box_open.
 * That preserves the byte-identical en_GB contract.
 *
 * The more() calls page files via the pre-existing locale-aware
 * locale_default_path resolver. They are intentionally left untouched:
 * paged file content is not part of the catalog surface.
 */
void
listbans(UR_OBJECT user)
{
    char filename[80];
    int i;

    strtolower(word[1]);
    if (!strcmp(word[1], "sites")) {
        lang_user(user, "listbans.sites.header");
        locale_default_path(filename, sizeof filename, DATAFILES, SITEBAN);
        switch (more(user, user->socket, filename)) {
        case 0:
            lang_user(user, "listbans.sites.empty");
            return;
        case 1:
            user->misc_op = 2;
            break;
        }
        return;
    }
    if (!strcmp(word[1], "users")) {
        lang_user(user, "listbans.users.header");
        locale_default_path(filename, sizeof filename, DATAFILES, USERBAN);
        switch (more(user, user->socket, filename)) {
        case 0:
            lang_user(user, "listbans.users.empty");
            return;
        case 1:
            user->misc_op = 2;
            break;
        }
        return;
    }
    if (!strcmp(word[1], "swears")) {
        lang_user(user, "listbans.swears.header");
        for (i = 0; swear_words[i]; ++i) {
            lang_user(user, "listbans.swears.row", swear_words[i]);
        }
        if (!i) {
            lang_user(user, "listbans.swears.empty");
        }
        if (amsys->ban_swearing) {
            lang_user(user, "listbans.swears.trailer_on");
        } else {
            lang_user(user, "listbans.swears.trailer_off");
        }
        return;
    }
    if (strcmp(word[1], "new")) {
        lang_user(user, "listbans.new.header");
        locale_default_path(filename, sizeof filename, DATAFILES, NEWBAN);
        switch (more(user, user->socket, filename)) {
        case 0:
            lang_user(user, "listbans.new.empty");
            return;
        case 1:
            user->misc_op = 2;
            break;
        }
        return;
    }
    lang_user(user, "listbans.usage");
}
