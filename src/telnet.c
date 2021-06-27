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
            user_telnet_input(user, ev->data.buffer, ev->data.size);
            telnet_negotiate(telnet, TELNET_WONT, TELNET_TELOPT_ECHO);
            telnet_negotiate(telnet, TELNET_WILL, TELNET_TELOPT_ECHO);
            break;
        /* data must be sent */
        case TELNET_EV_SEND:
            write_sock(user->socket, (const char *)*ev->data.buffer);

            //user_telnet_send(user->socket, ev->data.buffer, ev->data.size);
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
            destruct_user(user);
            break;
        /* ignore */
        default:
            break;
    }
}


void
linebuffer_push(char *buffer, size_t size, int *linepos, char ch, void *ud) {

    UR_OBJECT user = (UR_OBJECT)ud;

    /* CRLF -- line terminator */
    if (ch == '\n' && *linepos > 0 && buffer[*linepos - 1] == '\r') {
        /* NUL terminate (replaces \r in buffer), notify app, clear */
        buffer[*linepos - 1] = 0;
        *linepos = 0;

        /* CRNUL -- just a CR */
    } else if (ch == 0 && *linepos > 0 && buffer[*linepos - 1] == '\r') {
        /* do nothing, the CR is already in the buffer */

        /* anything else (including technically invalid CR followed by
         * anything besides LF or NUL -- just buffer if we have room
         * \r
         */
    } else if (*linepos != (int)size) {
        buffer[(*linepos)++] = ch;

        /* buffer overflow */
    } else {
        /* terminate (NOTE: eats a byte), notify app, clear buffer */
        buffer[size - 1] = 0;
        *linepos = 0;
    }
    if (user->charmode_echo
        && ((user->login != LOGIN_PASSWD && user->login != LOGIN_CONFIRM)
            || user->show_pass))
        send(user->socket, buffer, (size_t) linepos, 0);
}

void
user_telnet_input(UR_OBJECT user, const char *buffer, size_t size) {
    unsigned int i;
    *user->buff = '\0';
    user->buffpos = 0;
    for (i = 0; i != size; ++i) {
        linebuffer_push(user->buff, sizeof(user->buff), &user->buffpos, (char)buffer[i], user);
    }
}


/* process input line */
void
user_telnet_online(const char *line, size_t overflow, void *user_data) {
    UR_OBJECT user = (UR_OBJECT)user_data;
    //login(user, (char*)line);
    //write_user(user, "Hey!");
}

void user_telnet_send(int sock, const char *buffer, size_t size) {
    int rs;

    /* ignore on invalid socket */
    if (sock == -1)
        return;

    /* send data */
    while (size > 0) {
        if ((rs = send(sock, buffer, (int)size, 0)) == -1) {
            if (errno != EINTR && errno != ECONNRESET) {
                fprintf(stderr, "send() failed: %s\n", strerror(errno));
                exit(1);
            } else {
                return;
            }
        } else if (rs == 0) {
            fprintf(stderr, "send() unexpectedly returned 0\n");
            exit(1);
        }

        /* update pointer and size to see if we've got more to send */
        buffer += rs;
        size -= rs;
    }
}