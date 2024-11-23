
#include "../includes/defines.h"
#include "../includes/globals.h"
#include "../includes/commands.h"
#include "../includes/prototypes.h"

/*
 * Switch prompt on and off
 */
void
toggle_prompt(UR_OBJECT user)
{
    if (user->prompt) {
        write_user(user, "Prompt ~FROFF.\n");
        user->prompt = 0;
        return;
    }
    write_user(user, "Prompt ~FGON.\n");
    user->prompt = 1;
}
