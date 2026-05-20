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
#include "telnet.h"

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
        telnet_negotiate(user->telnet, TELNET_WILL, TELNET_TELOPT_ECHO);
    } else {
        write_user(user, "Echoing for character mode clients ~FROFF~RS.\n");
        user->charmode_echo = 0;
        /*
         * Don't send WONT ECHO here. WILL ECHO stays active so the client's
         * local echo remains off. With charmode_echo=0 the server also stops
         * echoing, so nothing is echoed. This is the expected behaviour for
         * character mode clients -- turning charecho off means blind typing.
         */
    }
    if (!user->room) {
        prompt(user);
    }
}
