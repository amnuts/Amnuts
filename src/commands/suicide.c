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
 * Allow a user to delete their own account
 */
void
suicide(UR_OBJECT user)
{
    if (word_count < 2) {
        lang_user(user, "suicide.usage");
        return;
    }
    if (strcmp(user->pass, crypt(word[1], user->pass))) {
        lang_user(user, "suicide.password_incorrect");
        return;
    }
    lang_user(user, "suicide.confirm_prompt");
    user->misc_op = 6;
    no_prompt = 1;
}
