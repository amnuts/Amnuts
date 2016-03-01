
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Switch between command and speech mode
 */
void
toggle_mode(UR_OBJECT user)
{
    if (user->command_mode) {
        write_user(user, "Now in ~FGSPEECH~RS mode.\n");
        user->command_mode = 0;
        return;
    }
    write_user(user, "Now in ~FCCOMMAND~RS mode.\n");
    user->command_mode = 1;
}
