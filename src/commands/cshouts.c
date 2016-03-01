
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