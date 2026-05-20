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
 * Display colours to user
 */
void
display_colour(UR_OBJECT user)
{
    static const char *const colours[] = {
        "~OL", "~UL", "~LI", "~RV",
        "~FK", "~FR", "~FG", "~FY", "~FB", "~FM", "~FC", "~FW",
        "~BK", "~BR", "~BG", "~BY", "~BB", "~BM", "~BC", "~BW",
        NULL
    };
    size_t i;

    if (!user->room) {
        prompt(user);
        return;
    }
    for (i = 0; colours[i]; ++i) {
        vwrite_user(user, "^%s: %sAmnuts version %s VIDEO TEST\n",
                colours[i], colours[i], AMNUTSVER);
    }
}
