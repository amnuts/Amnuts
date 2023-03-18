
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Set the character mode echo on or off. This is only for users logging in
 * via a character mode client, those using a line mode client (eg unix
 * telnet) will see no effect.
 */
void
toggle_charecho(UR_OBJECT user)
{
    if (!user->charmode_echo) {
        write_user(user, "Echoing for character mode clients ~FGON~RS.\n");
        user->charmode_echo = 1;
    } else {
        write_user(user, "Echoing for character mode clients ~FROFF~RS.\n");
        user->charmode_echo = 0;
    }
    if (!user->room) {
        prompt(user);
    }
}
