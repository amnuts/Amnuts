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
 * Clear the tell buffer of the user
 */
void
clear_afk(UR_OBJECT user)
{
    destruct_review_buffer_type(user, rbfAFK, 0);
    write_user_lang(user, "cafk.cleared");
}
