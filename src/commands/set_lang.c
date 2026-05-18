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
 * set lang
 * set lang <name>
 * set lang default
 */
void
set_user_lang(UR_OBJECT user)
{
    if (word_count < 3) {
        locale_list(user);
        if (*user->locale) {
            vwrite_user(user, "Your current language: ~OL%s~RS\n",
                        user->locale);
        } else {
            vwrite_user(user, "Your current language: ~OL%s~RS (server default)\n",
                        amsys->default_locale);
        }
        write_user(user, "Usage: set lang <name>|default\n");
        return;
    }

    if (!strcasecmp(word[2], "default")) {
        if (locale_set_user(user, NULL)) {
            vwrite_user(user, "Language reset to server default (~OL%s~RS).\n",
                        amsys->default_locale);
        } else {
            write_user(user, "Failed to reset language.\n");
        }
        return;
    }

    if (!locale_set_user(user, word[2])) {
        vwrite_user(user,
                    "No such language: ~OL%s~RS. Use ~OLset lang~RS for the list.\n",
                    word[2]);
        return;
    }
    vwrite_user(user, "Language set to ~OL%s~RS.\n", user->locale);
}
