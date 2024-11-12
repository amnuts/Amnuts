/****************************************************************************
         Amnuts version 2.3.0 - Copyright (C) Andrew Collington, 2003
                      Last update: 2003-08-04

                              amnuts@talker.com
                          http://amnuts.talker.com/

                                   based on

   NUTS version 3.3.3 (Triple Three :) - Copyright (C) Neil Robertson 1996
 ***************************************************************************/

#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"
#include "telnet.h"

/***************************************************************************/


void
telnet_event_handler(telnet_t *telnet, telnet_event_t *ev, void *user_data)
{
    UR_OBJECT user = (UR_OBJECT)user_data;

    switch (ev->type) {
        /* data received */
        case TELNET_EV_DATA:
            handle_user_input(user, (char *)ev->data.buffer, ev->data.size);
            break;
        /* data must be sent */
        case TELNET_EV_SEND:
            write_sock(user->socket, ev->data.buffer, ev->data.size);
            break;
        /* enable compress2 if accepted by client */
        case TELNET_EV_DO:
            if (ev->neg.telopt == TELNET_TELOPT_COMPRESS2) {
                telnet_begin_compress2(telnet);
            }
            break;
        /* error */
        case TELNET_EV_ERROR:
            write_syslog(ERRLOG, 1, "TELNET: Error during telnet event handler for %s\n", user->name);
            disconnect_user(user);
            break;
        /* ignore */
        default:
            break;
    }
}
