
#include "../includes/defines.h"
#include "../includes/globals.h"
#include "../includes/commands.h"
#include "../includes/prototypes.h"

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