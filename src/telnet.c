/****************************************************************************
             Amnuts - Copyright (C) Andrew Collington, 1996-2024
                        Last update: Sometime in 2024

                   talker@amnuts.net - https://amnuts.net/

                                 based on

   NUTS version 3.3.3 (Triple Three :) - Copyright (C) Neil Robertson 1996
 ***************************************************************************/


#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"
#include "telnet.h"

const telnet_telopt_t telopts[] = {
  { TELNET_TELOPT_BINARY,    TELNET_WONT, TELNET_DO   },
  { TELNET_TELOPT_SGA,       TELNET_WILL, TELNET_DONT },
  { TELNET_TELOPT_COMPRESS2, TELNET_WILL, TELNET_DONT },
  { TELNET_TELOPT_ECHO,      TELNET_WONT, TELNET_DONT },
  { TELNET_TELOPT_MSSP,      TELNET_WONT, TELNET_DO   },
  { TELNET_TELOPT_NAWS,      TELNET_WONT, TELNET_DO   },
  { TELNET_TELOPT_TTYPE,     TELNET_WONT, TELNET_DO   },
  { TELNET_TELOPT_ZMP,       TELNET_WONT, TELNET_DO   },
  { -1, 0, 0 }
};

/***************************************************************************/


int
effective_pager(UR_OBJECT user)
{
  int pager;

  if (user->pager >= MAX_LINES && user->pager <= 999) {
    pager = user->pager;
  } else {
    pager = user->term_height - 1;
    if (pager < MAX_LINES) {
      pager = MAX_LINES;
    } else if (pager > 99) {
      pager = 99;
    }
  }
  return pager;
}


int
effective_wrap(UR_OBJECT user)
{
  if (user->term_width >= 20 && user->term_width <= 500) {
    return user->term_width;
  }
  return SCREEN_WRAP;
}


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
    write_sock_with_size(user->socket, ev->data.buffer, ev->data.size);
    break;
  /* client accepted our WILL — enable features accordingly */
  case TELNET_EV_DO:
    if (ev->neg.telopt == TELNET_TELOPT_COMPRESS2) {
      telnet_begin_compress2(telnet);
    }
    break;
  /* client rejected our WILL — they handle it themselves */
  case TELNET_EV_DONT:
    break;
  /* NAWS and TTYPE subnegotiation */
  case TELNET_EV_SUBNEGOTIATION:
    if (ev->sub.telopt == TELNET_TELOPT_NAWS && ev->sub.size >= 4) {
      int w = ((unsigned char)ev->sub.buffer[0] << 8)
            | (unsigned char)ev->sub.buffer[1];
      int h = ((unsigned char)ev->sub.buffer[2] << 8)
            | (unsigned char)ev->sub.buffer[3];
      if (w >= 20 && w <= 500) {
        user->term_width = w;
      }
      if (h >= 5 && h <= 500) {
        user->term_height = h;
      }
    }
    break;
  /* client will send terminal type — request it */
  case TELNET_EV_WILL:
    if (ev->neg.telopt == TELNET_TELOPT_TTYPE) {
      telnet_ttype_send(telnet);
    }
    break;
  /* terminal type response */
  case TELNET_EV_TTYPE:
    if (ev->ttype.cmd == TELNET_TTYPE_IS && ev->ttype.name) {
      snprintf(user->term_type, sizeof user->term_type, "%s",
               ev->ttype.name);
    }
    break;
  /* recoverable error */
  case TELNET_EV_WARNING:
    write_syslog(ERRLOG, 0, "TELNET: Warning for %s: %s\n",
                 user->name, ev->error.msg);
    break;
  /* fatal error */
  case TELNET_EV_ERROR:
    write_syslog(ERRLOG, 1, "TELNET: Error during telnet event handler for %s\n", user->name);
    disconnect_user(user);
    break;
  /* ignore */
  default:
    break;
  }
}
