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
 * Clear the shout buffer of the talker
 */
void
clear_shouts(void)
{
    int i;

    for (i = 0; i < REVIEW_LINES; ++i) {
        *amsys->shoutbuff[i] = '\0';
    }
    amsys->sbuffline = 0;
}
