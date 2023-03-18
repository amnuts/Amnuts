/****************************************************************************
             Amnuts - Copyright (C) Andrew Collington, 1996-2023
                        Last update: Sometime in 2023

                   talker@amnuts.net - https://amnuts.net/

                                 based on

   NUTS version 3.3.3 (Triple Three :) - Copyright (C) Neil Robertson 1996
 ***************************************************************************/


#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"
#ifndef __SDS_H
#include "./vendors/sds/sds.h"
#endif

#ifdef NETLINKS
/**************************************************************************/

#include "netlinks.h"

/*
 * Construct link object
 */
NL_OBJECT
create_netlink(void)
{
    NL_OBJECT nl;

    nl = (NL_OBJECT) malloc(sizeof *nl);
    if (!nl) {
        write_syslog(NETLOG, 1,
                "NETLINK: Memory allocation failure in create_netlink().\n");
        return NULL;
    }
    memset(nl, 0, (sizeof *nl));
    if (!nl_first) {
        nl_first = nl;
        nl->next = NULL;
        nl->prev = NULL;
    } else {
        nl_last->next = nl;
        nl->next = NULL;
        nl->prev = nl_last;
    }
    nl_last = nl;
    *nl->service = '\0';
    *nl->site = '\0';
    *nl->verification = '\0';
    *nl->mail_to = '\0';
    *nl->mail_from = '\0';
    nl->mailfile = NULL;
    *nl->buffer = '\0';
    nl->ver_major = 0;
    nl->ver_minor = 0;
    nl->ver_patch = 0;
    nl->keepalive_cnt = 0;
    nl->last_recvd = 0;
    *nl->port = '\0';
    nl->socket = 0;
    nl->mesg_user = NULL;
    nl->connect_room = NULL;
    nl->type = UNCONNECTED;
    nl->stage = DOWN;
    nl->connected = 0;
    nl->lastcom = -1;
    nl->allow = ALL;
    nl->warned = 0;
    return nl;
}

/*
 * Destruct a netlink (usually a closed incoming one)
 */
void
destruct_netlink(NL_OBJECT nl)
{
    if (nl != nl_first) {
        nl->prev->next = nl->next;
        if (nl != nl_last) {
            nl->next->prev = nl->prev;
        } else {
            nl_last = nl->prev;
            nl_last->next = NULL;
        }
    } else {
        nl_first = nl->next;
        if (nl != nl_last) {
            nl_first->prev = NULL;
        } else {
            nl_last = NULL;
        }
    }
    memset(nl, 0, (sizeof *nl));
    free(nl);
}


/******************************************************************************
 Connection functions - making a link to external talkers
 *****************************************************************************/

/*
 * Initialise connections to remote servers. Basically this tries to connect
 * to the services listed in the config file and it puts the open sockets in
 * the NL_OBJECT linked list which the talker then uses
 */
void
init_connections(void)
{
    NL_OBJECT nl;
    RM_OBJECT rm;
    int cnt = 0;

    printf("Connecting to remote servers...\n");
    for (rm = room_first; rm; rm = rm->next) {
        nl = rm->netlink;
        if (!nl) {
            continue;
        }
        ++cnt;
        printf("  Trying service %s (%s:%s): ", nl->service, nl->site, nl->port);
        fflush(stdout);
        nl->socket = socket_connect(nl->site, nl->port);
        if (nl->socket < 0) {
            printf("Connect failed: %s.\n",
                    nl->socket != -1 ? "Unknown hostname" : strerror(errno));
            write_syslog(NETLOG, 1, "NETLINK: Failed to connect to %s: %s.\n",
                    nl->service,
                    nl->socket != -1 ? "Unknown hostname" : strerror(errno));
            continue;
        }
        nl->type = OUTGOING;
        nl->stage = VERIFYING;
        nl->last_recvd = time(0);
        nl->connect_room = rm;
        printf("CONNECTED.\n");
        write_syslog(NETLOG, 1, "NETLINK: Connected to %s (%s:%s).\n",
                nl->service, nl->site, nl->port);
    }
    if (cnt) {
        printf("  See netlinks log for any further information.\n");
    } else {
        printf("  No remote connections configured.\n");
    }
}

/*
 * Do the actual connection
 */
int
socket_connect(const char *host, const char *serv)
{
    struct sockaddr_in sa;
    int s;

    sa.sin_family = AF_INET;
    sa.sin_port = htons(atoi(serv));
    sa.sin_addr.s_addr = inet_addr(host);
    if (sa.sin_addr.s_addr == (uint32_t) - 1) {
        struct hostent *he;

        /* XXX: This may hang. */
        he = gethostbyname(host);
        if (!he) {
            return -2;
        }
        memcpy(&sa.sin_addr, he->h_addr, (sizeof sa.sin_addr));
    }
    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == -1) {
        return -1;
    }
    /* XXX: This may hang. */
    if (connect(s, (struct sockaddr *) &sa, (sizeof sa)) == -1) {
        return -1;
    }
    return s;
}


/******************************************************************************
 Automatic event functions that relate to the Netlinks
 *****************************************************************************/

/*
 * See if any net connections are dragging their feet. If they have been idle
 * longer than net_idle_time the drop them. Also send keepalive signals down
 * links, this saves having another function and loop to do it.
 */
void
check_nethangs_send_keepalives(void)
{
    NL_OBJECT nl, next;
    int secs;

    for (nl = nl_first; nl; nl = next) {
        next = nl->next;
        if (nl->type == UNCONNECTED) {
            nl->warned = 0;
            continue;
        }
        /* Send keepalives */
        nl->keepalive_cnt += amsys->heartbeat;
        if (nl->keepalive_cnt >= amsys->keepalive_interval) {
            sprintf(text, "%s\n", netcom[NLC_KEEPALIVE]);
            write_sock(nl->socket, text);
            nl->keepalive_cnt = 0;
        }
        /* Check time outs */
        secs = (int) (time(0) - nl->last_recvd);
        if (nl->warned) {
            if (secs < amsys->net_idle_time - 60) {
                nl->warned = 0;
            } else {
                if (secs < amsys->net_idle_time) {
                    continue;
                }
                sprintf(text,
                        "~OLSYSTEM:~RS Disconnecting hung netlink to %s in the %s.\n",
                        nl->service, nl->connect_room->name);
                write_room(NULL, text);
                shutdown_netlink(nl);
            }
            continue;
        }
        if (secs > amsys->net_idle_time - 60) {
            vwrite_level(ARCH, 1, NORECORD, NULL,
                    "~OLSYSTEM:~RS Netlink to %s in the %s has been hung for %d seconds.\n",
                    nl->service, nl->connect_room->name, secs);
            nl->warned = 1;
        }
    }
    destructed = 0;
}


/******************************************************************************
 NUTS Netlink protocols and functions
 *****************************************************************************/

/*
 * Accept incoming server connection
 */
void
accept_server_connection(int lsock)
{
    struct sockaddr_in sa;
    char hostaddr[MAXADDR]; /* XXX: Use NI_MAXHOST, INET_ADDRSTRLEN, INET6_ADDRSTRLEN */
    char hostname[MAXHOST]; /* XXX: Use NI_MAXHOST */
    NL_OBJECT nl, nl2;
    RM_OBJECT rm;
    int sock;
    socklen_t sa_len;

    sa_len = (sizeof sa);
    sock = accept(lsock, (struct sockaddr *) &sa, &sa_len);
    if (sock == -1) {
        return;
    }
    *hostname = *hostaddr = '\0';
    strcpy(hostaddr, inet_ntoa(sa.sin_addr));
    {
        struct hostent *he;

        /* XXX: This may hang. */
        he =
                gethostbyaddr((char *) &sa.sin_addr, (sizeof sa.sin_addr),
                sa.sin_family);
        if (!he) {
            strcpy(hostname, hostaddr);
        } else {
            strcpy(hostname, he->h_name);
            strtolower(hostname);
        }
    }
    /* Send server type id and version number */
    sprintf(text, "NUTS %s\n", NUTSVER);
    write_sock(sock, text);
    write_syslog(NETLOG, 1,
            "NETLINK: Received request connection from site %s.\n",
            hostaddr);
    /* See if legal site, i.e., site is in config sites list. */
    for (nl2 = nl_first; nl2; nl2 = nl2->next) {
        if (!strcmp(nl2->site, hostaddr) || !strcmp(nl2->site, hostname)) {
            break;
        }
    }
    if (!nl2) {
        sprintf(text, "%s CONNECT 1\n", netcom[NLC_DENIED]);
        write_sock(sock, text);
        shutdown(sock, SHUT_WR);
        close(sock);
        write_syslog(NETLOG, 1,
                "NETLINK: Request denied, remote site not in valid sites list.\n");
        return;
    }
    /* Find free room link */
    for (rm = room_first; rm; rm = rm->next) {
        if (!rm->netlink && rm->inlink) {
            break;
        }
    }
    if (!rm) {
        sprintf(text, "%s CONNECT 3\n", netcom[NLC_DENIED]);
        write_sock(sock, text);
        shutdown(sock, SHUT_WR);
        close(sock);
        write_syslog(NETLOG, 1, "NETLINK: Request denied, no free room links.\n");
        return;
    }
    nl = create_netlink();
    if (!nl) {
        sprintf(text, "%s CONNECT 2\n", netcom[NLC_DENIED]);
        write_sock(sock, text);
        shutdown(sock, SHUT_WR);
        close(sock);
        write_syslog(NETLOG, 1,
                "NETLINK: Request denied, unable to create netlink object.\n");
        return;
    }
    rm->netlink = nl;
    nl->socket = sock;
    nl->type = INCOMING;
    nl->stage = VERIFYING;
    nl->connect_room = rm;
    nl->allow = nl2->allow;
    nl->last_recvd = time(0);
    strcpy(nl->service, "<verifying>");
    strcpy(nl->site, hostaddr);
    sprintf(text, "%s CONNECT\n", netcom[NLC_GRANTED]);
    write_sock(sock, text);
    write_syslog(NETLOG, 1, "NETLINK: Request granted.\n");
}

/*
 * Deal with netlink data on link nl
 */
void
exec_netcom(NL_OBJECT nl, char *inpstr)
{
    char w1[ARR_SIZE], w2[ARR_SIZE], w3[ARR_SIZE], *c, ctemp;
    const char *const *netcom_str;
    enum nlc_value netcom_num;
    int lev;

    /*
       The buffer is large (ARR_SIZE*2) but if a bug occurs with a remote system
       and no newlines are sent for some reason it may overflow and this will
       probably cause a crash. Oh well, such is life.
     */
    if (*nl->buffer) {
        strcat(nl->buffer, inpstr);
        inpstr = nl->buffer;
    }
    nl->last_recvd = time(0);

    /* Go through data */
    while (*inpstr) {
        /* Find first newline */
        c = strchr(inpstr, '\n');
        /* If no newline then input is incomplete, store and return */
        if (!c) {
            if (inpstr != nl->buffer) {
                strcpy(nl->buffer, inpstr);
            }
            return;
        }
        ctemp = c[1];
        c[1] = '\0';
        *w1 = '\0';
        *w2 = '\0';
        *w3 = '\0';
        lev = 0;
        if (*inpstr != '\n') {
            sscanf(inpstr, "%s %s %s %d", w1, w2, w3, &lev);
        }
        /* Get command number */
        for (netcom_str = netcom; *netcom_str; ++netcom_str) {
            if (!strcmp(*netcom_str, w1)) {
                break;
            }
        }
        netcom_num = (enum nlc_value) (netcom_str - netcom);
        /* Deal with initial connects */
        if (nl->stage == VERIFYING) {
            if (nl->type == OUTGOING) {
                if (strcmp(w1, "NUTS")) {
                    write_syslog(NETLOG, 1,
                            "NETLINK: Incorrect connect message from %s.\n",
                            nl->service);
                    shutdown_netlink(nl);
                    return;
                }
                /* Store remote version for compat checks */
                nl->stage = UP;
                w2[10] = '\0';
                sscanf(w2, "%d.%d.%d", &nl->ver_major, &nl->ver_minor,
                        &nl->ver_patch);
                goto NEXT_LINE;
            } else {
                /* Incoming */
                if (netcom_num != NLC_VERIFICATION) {
                    /* No verification, no connection */
                    write_syslog(NETLOG, 1,
                            "NETLINK: No verification sent by site %s.\n",
                            nl->site);
                    shutdown_netlink(nl);
                    return;
                }
                nl->stage = UP;
            }
        }
        /*
           If remote is currently sending a message relay it to user, do not
           interpret it unless its ENDMESSAGE or ERROR
         */
        if (nl->mesg_user && netcom_num != NLC_ENDMESSAGE
                && netcom_num != NLC_ERROR) {
            /* If -1 then user logged off before end of mesg received */
            if (nl->mesg_user != (UR_OBJECT) - 1) {
                write_user(nl->mesg_user, inpstr);
            }
            goto NEXT_LINE;
        }
        /* Same goes for mail except its ENDMAIL or ERROR */
        if (nl->mailfile && netcom_num != NLC_ENDMAIL && netcom_num != NLC_ERROR) {
            fputs(inpstr, nl->mailfile);
            goto NEXT_LINE;
        }
        nl->lastcom = netcom_num;
        switch (netcom_num) {
        case NLC_DISCONNECT:
            if (nl->stage == UP) {
                sprintf(text,
                        "~OLSYSTEM:~FY~RS Disconnecting from service %s in the %s.\n",
                        nl->service, nl->connect_room->name);
                write_room(NULL, text);
            }
            shutdown_netlink(nl);
            break;
        case NLC_TRANSFER:
            nl_transfer(nl, w2, w3, (enum lvl_value) lev, inpstr);
            break;
        case NLC_RELEASE:
            nl_release(nl, w2);
            break;
        case NLC_ACTION:
            nl_action(nl, w2, inpstr);
            break;
        case NLC_GRANTED:
            nl_granted(nl, w2);
            break;
        case NLC_DENIED:
            nl_denied(nl, w2, inpstr);
            break;
        case NLC_MESSAGE:
            nl_mesg(nl, w2);
            break;
        case NLC_ENDMESSAGE:
            nl->mesg_user = NULL;
            break;
        case NLC_PROMPT:
            nl_prompt(nl, w2);
            break;
        case NLC_VERIFICATION:
            nl_verification(nl, w2, w3, 0);
            break;
        case NLC_VERIFY:
            nl_verification(nl, w2, w3, 1);
            break;
        case NLC_REMOVED:
            nl_removed(nl, w2);
            break;
        case NLC_ERROR:
            nl_error(nl);
            break;
        case NLC_EXISTSQUERY:
            nl_checkexist(nl, w2, w3);
            break;
        case NLC_EXISTSNAK:
            nl_user_notexist(nl, w2, w3);
            break;
        case NLC_EXISTSACK:
            nl_user_exist(nl, w2, w3);
            break;
        case NLC_MAIL:
            nl_mail(nl, w2, w3);
            break;
        case NLC_ENDMAIL:
            nl_endmail(nl);
            break;
        case NLC_MAILERROR:
            nl_mailerror(nl, w2, w3);
            break;
        case NLC_KEEPALIVE:
            /* Keepalive signal, do nothing */
            break;
        case NLC_RSTAT:
            nl_rstat(nl, w2);
            break;
        default:
            write_syslog(NETLOG, 1,
                    "NETLINK: Received unknown command \"%s\" from %s.\n", w1,
                    nl->service);
            sprintf(text, "%s\n", netcom[NLC_ERROR]);
            write_sock(nl->socket, text);
            break;
        }
    NEXT_LINE:
        /* See if link has closed */
        if (nl->type == UNCONNECTED) {
            return;
        }
        c[1] = ctemp;
        inpstr = c + 1;
    }
    if (nl) {
        *nl->buffer = '\0';
    }
}

/*
 * Deal with user being transfered over from remote site
 */
void
nl_transfer(NL_OBJECT nl, char *name, char *pass, enum lvl_value lvl,
        char *inpstr)
{
    UR_OBJECT u;

    /* link for outgoing users only */
    if (nl->allow == OUT) {
        sprintf(text, "%s %s 4\n", netcom[NLC_DENIED], name);
        write_sock(nl->socket, text);
        return;
    }
    if (strlen(name) > USER_NAME_LEN) {
        name[USER_NAME_LEN] = '\0';
    }

    /* See if user is banned */
    if (user_banned(name)) {
        if (nl->ver_major == 3 && nl->ver_minor >= 3 && nl->ver_patch >= 3) {
            /* new error for 3.3.3 */
            sprintf(text, "%s %s 9\n", netcom[NLC_DENIED], name);
        } else {
            /* old error to old versions */
            sprintf(text, "%s %s 6\n", netcom[NLC_DENIED], name);
        }
        write_sock(nl->socket, text);
        return;
    }

    /* See if user already on here */
    u = get_user(name);
    if (u) {
        sprintf(text, "%s %s 5\n", netcom[NLC_DENIED], name);
        write_sock(nl->socket, text);
        return;
    }

    /* See if user of this name exists on this system by trying to load up datafile */
    u = create_user();
    if (!u) {
        sprintf(text, "%s %s 6\n", netcom[NLC_DENIED], name);
        write_sock(nl->socket, text);
        return;
    }
    u->type = REMOTE_TYPE;
    strcpy(u->name, name);
    if (load_user_details(u)) {
        /* FIXME: Send unencrypted password for random salt */
        if (strcmp(u->pass, pass)) {
            /* Incorrect password sent */
            sprintf(text, "%s %s 7\n", netcom[NLC_DENIED], name);
            write_sock(nl->socket, text);
            destruct_user(u);
            destructed = 0;
            return;
        }
    } else {
        /* Get the users description */
        if (nl->ver_major <= 3 && nl->ver_minor <= 3 && nl->ver_patch < 1) {
            strcpy(text, remove_first(remove_first(remove_first(inpstr))));
        } else {
            strcpy(text,
                    remove_first(remove_first(remove_first(remove_first(inpstr)))));
        }
        text[USER_DESC_LEN] = '\0';
        terminate(text);
        strcpy(u->desc, text);
        strcpy(u->in_phrase, "enters");
        strcpy(u->out_phrase, "goes");
        strcpy(u->recap, u->name);
        strcpy(u->last_site, "[remote]");
        if (nl->ver_major == 3 && nl->ver_minor >= 3 && nl->ver_patch >= 1) {
            if (lvl > amsys->rem_user_maxlevel) {
                u->level = amsys->rem_user_maxlevel;
            } else {
                u->level = lvl;
            }
        } else {
            u->level = amsys->rem_user_deflevel;
        }
        u->unarrest = u->level;
    }
    /* See if users level is below minlogin level */
    if (u->level < amsys->minlogin_level) {
        if (nl->ver_major == 3 && nl->ver_minor >= 3 && nl->ver_patch >= 3) {
            /* new error for 3.3.3 */
            sprintf(text, "%s %s 8\n", netcom[NLC_DENIED], u->name);
        } else {
            /* old error to old versions */
            sprintf(text, "%s %s 6\n", netcom[NLC_DENIED], u->name);
        }
        write_sock(nl->socket, text);
        destruct_user(u);
        destructed = 1;
        return;
    }
    strcpy(u->site, nl->service);
    sprintf(text, "%s enters from cyberspace.\n", u->name);
    write_room(nl->connect_room, text);
    write_syslog(NETLOG, 1, "NETLINK: Remote user %s received from %s.\n",
            u->name, nl->service);
    u->room = nl->connect_room;
    strcpy(u->logout_room, u->room->name);
    u->netlink = nl;
    u->read_mail = time(0);
    u->last_login = time(0);
    ++amsys->num_of_users;
    sprintf(text, "%s %s\n", netcom[NLC_GRANTED], name);
    write_sock(nl->socket, text);
}

/*
 * User is leaving this system
 */
void
nl_release(NL_OBJECT nl, char *name)
{
    UR_OBJECT u;

    u = get_user(name);
    if (u && u->type == REMOTE_TYPE) {
        vwrite_room_except(u->room, u, "%s~RS leaves this plain of existence.\n",
                u->recap);
        write_syslog(NETLOG, 1, "NETLINK: Remote user %s released.\n", u->name);
        destroy_user_clones(u);
        destruct_user(u);
        --amsys->num_of_users;
        return;
    }
    write_syslog(NETLOG, 1,
            "NETLINK: Release requested for unknown/invalid user %s from %s.\n",
            name, nl->service);
}

/*
 * Remote user performs an action on this system
 */
void
nl_action(NL_OBJECT nl, char *name, char *inpstr)
{
    UR_OBJECT u;
    char *c, ctemp;

    u = get_user(name);
    if (!u) {
        sprintf(text, "%s %s 8\n", netcom[NLC_DENIED], name);
        write_sock(nl->socket, text);
        return;
    }
    if (u->socket != -1) {
        write_syslog(NETLOG, 1,
                "NETLINK: Action requested for local user %s from %s.\n",
                name, nl->service);
        return;
    }
    inpstr = remove_first(remove_first(inpstr));
    /* remove newline character */
    ctemp = '\0';
    for (c = inpstr; *c; ++c) {
        if (*c == '\n') {
            break;
        }
    }
    if (*c) {
        ctemp = *c;
        *c = '\0';
    }
    u->last_input = time(0);
    if (u->misc_op) {
        if (!strcmp(inpstr, "NL")) {
            /* FIXME: This is very hackish but misc_ops() and friends
             * assume they can alter the passed in inpstr so cannot pass
             * in the constant "\n" */
            strcpy(inpstr, "\n");
            misc_ops(u, inpstr);
            strcpy(inpstr, "NL");
        } else {
            misc_ops(u, inpstr + 4);
        }
        return;
    }
    if (u->afk) {
        write_user(u, "You are no longer AFK.\n");
        if (u->vis) {
            vwrite_room_except(u->room, u, "%s~RS comes back from being AFK.\n",
                    u->recap);
        }
        u->afk = 0;
    }
    word_count = wordfind(inpstr);
    if (!strcmp(inpstr, "NL")) {
        return;
    }
    exec_com(u, inpstr, COUNT);
    if (ctemp) {
        *c = ctemp;
    }
    if (!u->misc_op) {
        prompt(u);
    }
}

/*
 * Grant received from remote system
 */
void
nl_granted(NL_OBJECT nl, char *name)
{
    UR_OBJECT u;
    RM_OBJECT old_room;

    if (!strcmp(name, "CONNECT")) {
        write_syslog(NETLOG, 1, "NETLINK: Connection to %s granted.\n",
                nl->service);
        /* Send our verification and version number */
        sprintf(text, "%s %s %s\n", netcom[NLC_VERIFICATION], amsys->verification,
                NUTSVER);
        write_sock(nl->socket, text);
        return;
    }
    u = get_user(name);
    if (!u) {
        write_syslog(NETLOG, 1,
                "NETLINK: Grant received for unknown user %s from %s.\n",
                name, nl->service);
        return;
    }
    /*
     * This will probably occur if a user tried to go to the other site,
     * got lagged then changed his mind and went elsewhere. Do not worry
     * about it
     */
    if (u->pot_netlink != nl) {
        write_syslog(NETLOG, 1,
                "NETLINK: Unexpected grant for %s received from %s.\n", name,
                nl->service);
        return;
    }
    /* User has been granted permission to move into remote talker */
    write_user(u, "~FB~OLYou traverse cyberspace...\n");
    if (u->vis) {
        vwrite_room_except(u->room, u, "%s~RS %s to the %s.\n", u->recap,
                u->out_phrase, nl->service);
    } else {
        write_room_except(u->room, invisleave, u);
    }
    write_syslog(NETLOG, 1, "NETLINK: %s transfered to %s.\n", u->name,
            nl->service);
    old_room = u->room;
    u->room = NULL; /* Means on remote talker */
    u->netlink = nl;
    u->pot_netlink = NULL;
    u->misc_op = 0;
    u->filepos = 0;
    *u->page_file = '\0';
    reset_access(old_room);
    sprintf(text, "ACT %s look\n", u->name);
    write_sock(nl->socket, text);
}

/*
 * Deny received from remote system
 */
void
nl_denied(NL_OBJECT nl, char *name, char *inpstr)
{
    static const char *const neterr[] = {
        "this site is not in the remote services valid sites list",
        "the remote service is unable to create a link",
        "the remote service has no free room links",
        "the link is for incoming users only",
        "a user with your name is already logged on the remote site",
        "the remote service was unable to create a session for you",
        "incorrect password. Use \".go <service> <password>\"",
        "your level there is below the remote services current minlogin level",
        "you are banned from that service"
    };
    UR_OBJECT u;
    int errnum;

    errnum = 0;
    sscanf(remove_first(remove_first(inpstr)), "%d", &errnum);
    if (!strcmp(name, "CONNECT")) {
        write_syslog(NETLOG, 1, "NETLINK: Connection to %s denied, %s.\n",
                nl->service, neterr[errnum - 1]);
        /* If wiz initiated connect let them know its failed */
        sprintf(text, "~OLSYSTEM:~RS Connection to %s failed, %s.\n", nl->service,
                neterr[errnum - 1]);
        vwrite_level((enum lvl_value) command_table[CONN].level, 1, NORECORD,
                NULL, "~OLSYSTEM:~RS Connection to %s failed, %s.\n",
                nl->service, neterr[errnum - 1]);
        shutdown(nl->socket, SHUT_WR);
        close(nl->socket);
        nl->type = UNCONNECTED;
        nl->stage = DOWN;
        return;
    }
    /* Is for a user */
    u = get_user(name);
    if (!u) {
        write_syslog(NETLOG, 1,
                "NETLINK: Deny for unknown user %s received from %s.\n",
                name, nl->service);
        return;
    }
    write_syslog(NETLOG, 1, "NETLINK: Deny %d for user %s received from %s.\n",
            errnum, name, nl->service);
    sprintf(text, "Sorry, %s.\n", neterr[errnum - 1]);
    write_user(u, text);
    prompt(u);
    u->pot_netlink = NULL;
}

/*
 * Text received to display to a user on here
 */
void
nl_mesg(NL_OBJECT nl, char *name)
{
    UR_OBJECT u;

    u = get_user(name);
    if (!u) {
        write_syslog(NETLOG, 1,
                "NETLINK: Message received for unknown user %s from %s.\n",
                name, nl->service);
        nl->mesg_user = (UR_OBJECT) - 1;
        return;
    }
    nl->mesg_user = u;
}

/*
 * Remote system asking for prompt to be displayed
 */
void
nl_prompt(NL_OBJECT nl, char *name)
{
    UR_OBJECT u;

    u = get_user(name);
    if (!u) {
        write_syslog(NETLOG, 1,
                "NETLINK: Prompt received for unknown user %s from %s.\n",
                name, nl->service);
        return;
    }
    if (u->type == REMOTE_TYPE) {
        write_syslog(NETLOG, 1,
                "NETLINK: Prompt received for remote user %s from %s.\n",
                name, nl->service);
        return;
    }
    prompt(u);
}

/*
 * Verification received from remote site
 */
void
nl_verification(NL_OBJECT nl, char *w2, char *w3, int com)
{
    NL_OBJECT nl2;

    if (!com) {
        /* We are verifiying a remote site */
        if (!*w2) {
            shutdown_netlink(nl);
            return;
        }
        for (nl2 = nl_first; nl2; nl2 = nl2->next) {
            if (!strcmp(nl->site, nl2->site) && !strcmp(w2, nl2->verification)) {
                switch (nl->allow) {
                case IN:
                    sprintf(text, "%s OK IN\n", netcom[NLC_VERIFY]);
                    write_sock(nl->socket, text);
                    break;
                case OUT:
                    sprintf(text, "%s OK OUT\n", netcom[NLC_VERIFY]);
                    write_sock(nl->socket, text);
                    break;
                case ALL:
                    sprintf(text, "%s OK ALL\n", netcom[NLC_VERIFY]);
                    write_sock(nl->socket, text);
                    break;
                }
                strcpy(nl->service, nl2->service);
                /* Only 3.2.0 and above send version number with verification */
                sscanf(w3, "%d.%d.%d", &nl->ver_major, &nl->ver_minor,
                        &nl->ver_patch);
                write_syslog(NETLOG, 1, "NETLINK: Connected to %s in the %s.\n",
                        nl->service, nl->connect_room->name);
                sprintf(text,
                        "~OLSYSTEM:~RS New connection to service %s in the %s.\n",
                        nl->service, nl->connect_room->name);
                write_room(NULL, text);
                return;
            }
        }
        sprintf(text, "%s BAD\n", netcom[NLC_VERIFY]);
        write_sock(nl->socket, text);
        shutdown_netlink(nl);
        return;
    }

    /* The remote site has verified us */
    if (!strcmp(w2, "BAD")) {
        write_syslog(NETLOG, 1,
                "NETLINK: Connection to %s has bad verification.\n",
                nl->service);
        /* Let wizes know its failed, may be wiz initiated */
        sprintf(text,
                "~OLSYSTEM:~RS Connection to %s failed, bad verification.\n",
                nl->service);
        vwrite_level((enum lvl_value) command_table[CONN].level, 1, NORECORD,
                NULL,
                "~OLSYSTEM:~RS Connection to %s failed, bad verification.\n",
                nl->service);
        shutdown_netlink(nl);
        return;
    }
    if (strcmp(w2, "OK")) {
        write_syslog(NETLOG, 1, "NETLINK: Unknown verify return code from %s.\n",
                nl->service);
        shutdown_netlink(nl);
    }
    /* Set link permissions */
    if (!strcmp(w3, "OUT")) {
        if (nl->allow == OUT) {
            write_syslog(NETLOG, 1,
                    "NETLINK: WARNING - Permissions deadlock, both sides are outgoing only.\n");
        } else {
            nl->allow = IN; /* FIXME: Should have remote allow too */
        }
    } else if (!strcmp(w3, "IN")) {
        if (nl->allow == IN) {
            write_syslog(NETLOG, 1,
                    "NETLINK: WARNING - Permissions deadlock, both sides are incoming only.\n");
        } else {
            nl->allow = OUT; /* FIXME: Should have remote allow too */
        }
    }
    write_syslog(NETLOG, 1, "NETLINK: Connection to %s verified.\n",
            nl->service);
    sprintf(text, "~OLSYSTEM:~RS New connection to service %s in the %s.\n",
            nl->service, nl->connect_room->name);
    write_room(NULL, text);
}

/*
 * Remote site only sends removed notification if user on remote site
 * tries to .go back to his home site or user is booted off. Home site
 * does not bother sending reply since remote site will remove user no
 * matter what.
 */
void
nl_removed(NL_OBJECT nl, char *name)
{
    UR_OBJECT u;

    u = get_user(name);
    if (!u) {
        write_syslog(NETLOG, 1,
                "NETLINK: Removed notification for unknown user %s received from %s.\n",
                name, nl->service);
        return;
    }
    if (u->room) {
        write_syslog(NETLOG, 1,
                "NETLINK: Removed notification of local user %s received from %s.\n",
                name, nl->service);
        return;
    }
    write_syslog(NETLOG, 1, "NETLINK: %s returned from %s.\n", u->name,
            u->netlink->service);
    u->room = u->netlink->connect_room;
    u->netlink = NULL;
    if (u->vis) {
        vwrite_room_except(u->room, u, "%s~RS %s\n", u->recap, u->in_phrase);
    } else {
        write_room_except(u->room, invisenter, u);
    }
    look(u);
    prompt(u);
}

/*
 * Got an error back from site, deal with it
 */
void
nl_error(NL_OBJECT nl)
{
    if (nl->mesg_user) {
        nl->mesg_user = NULL;
    }
    if (*nl->mail_to) {
        sds filename;
        fclose(nl->mailfile);
        filename = sdscatfmt(sdsempty(), "%s/IN_%s_%s@%s", MAILSPOOL, nl->mail_to, nl->mail_from, nl->service);
        remove(filename);
        sdsfree(filename);
        *nl->mail_to = '\0';
        *nl->mail_from = '\0';
    }
    /*
       lastcom value may be misleading, the talker may have sent off a whole load
       of commands before it gets a response due to lag, any one of them could
       have caused the error
     */
    write_syslog(NETLOG, 1, "NETLINK: Received %s from %s, lastcom = %d.\n",
            netcom[NLC_ERROR], nl->service, nl->lastcom);
}

/*
 * Does user exist? This is a question sent by a remote mailer to
 * verifiy mail IDs.
 */
void
nl_checkexist(NL_OBJECT nl, char *to, char *from)
{
    sprintf(text, "%s %s %s\n",
            netcom[find_user_listed(to) ? NLC_EXISTSACK : NLC_EXISTSNAK], to,
            from);
    write_sock(nl->socket, text);
}

/*
 * Remote user doesnt exist
 */
void
nl_user_notexist(NL_OBJECT nl, char *to, char *from)
{
    char text2[ARR_SIZE];
    sds filename;
    UR_OBJECT user;

    user = get_user(from);
    if (user) {
        vwrite_user(user,
                "~OLSYSTEM:~RS User %s does not exist at %s, your mail bounced.\n",
                to, nl->service);
    } else {
        sprintf(text2, "There is no user named %s at %s, your mail bounced.\n",
                to, nl->service);
        send_mail(NULL, from, text2, 0);
    }
    filename = sdscatfmt(sdsempty(), "%s/OUT_%s_%s@%s", MAILSPOOL, from, to, nl->service);
    remove(filename);
    sdsfree(filename);
}

/*
 * Remote users exists, send him some mail
 */
void
nl_user_exist(NL_OBJECT nl, char *to, char *from)
{
    sds filename;
    char text2[ARR_SIZE], line[82], *s;
    FILE *fp;
    UR_OBJECT user;

    filename = sdscatfmt(sdsempty(), "%s/OUT_%s_%s@%s", MAILSPOOL, from, to, nl->service);
    fp = fopen(filename, "r");
    if (!fp) {
        user = get_user(from);
        if (user) {
            sprintf(text,
                    "~OLSYSTEM:~RS An error occured during mail delivery to %s@%s.\n",
                    to, nl->service);
            write_user(user, text);
        } else {
            sprintf(text2, "An error occured during mail delivery to %s@%s.\n", to,
                    nl->service);
            send_mail(NULL, from, text2, 0);
        }
        sdsfree(filename);
        return;
    }
    sprintf(text, "%s %s %s\n", netcom[NLC_MAIL], to, from);
    write_sock(nl->socket, text);
    for (s = fgets(line, 80, fp); s; s = fgets(line, 80, fp)) {
        write_sock(nl->socket, s);
    }
    fclose(fp);
    sprintf(text, "\n%s\n", netcom[NLC_ENDMAIL]);
    write_sock(nl->socket, text);
    remove(filename);
    sdsfree(filename);
}

/*
 * Got some mail coming in
 */
void
nl_mail(NL_OBJECT nl, char *to, char *from)
{
    sds filename;

    write_syslog(NETLOG, 1, "NETLINK: Mail received for %s from %s.\n", to, nl->service);
    filename = sdscatfmt(sdsempty(), "%s/IN_%s_%s@%s", MAILSPOOL, to, from, nl->service);
    nl->mailfile = fopen(filename, "w");
    if (!nl->mailfile) {
        write_syslog(SYSLOG, 0,
                "ERROR: Cannot open file %s to write in nl_mail().\n",
                filename);
        sprintf(text, "%s %s %s\n", netcom[NLC_MAILERROR], to, from);
        write_sock(nl->socket, text);
        sdsfree(filename);
        return;
    }
    strcpy(nl->mail_to, to);
    strcpy(nl->mail_from, from);
    sdsfree(filename);
}

/*
 * End of mail message being sent from remote site
 */
void
nl_endmail(NL_OBJECT nl)
{
    struct stat stbuf;
    char infile[80], mailfile[80];
    FILE *infp, *outfp;
    int c;
    int amount, size, tmp1, tmp2;

    fclose(nl->mailfile);
    nl->mailfile = NULL;
    sprintf(mailfile, "%s/IN_%s_%s@%s", MAILSPOOL, nl->mail_to, nl->mail_from,
            nl->service);
    /* Copy to users mail file to a tempfile */
    outfp = fopen("tempfile", "w");
    if (!outfp) {
        write_syslog(SYSLOG, 0,
                "ERROR: Cannot open tempfile in netlink_endmail().\n");
        sprintf(text, "%s %s %s\n", netcom[NLC_MAILERROR], nl->mail_to, nl->mail_from);
        write_sock(nl->socket, text);
        *nl->mail_to = '\0';
        *nl->mail_from = '\0';
        remove(mailfile);
        return;
    }
    /* Copy old mail file to tempfile */
    sprintf(infile, "%s/%s/%s.M", USERFILES, USERMAILS, nl->mail_to);
    /* first get old file size if any new mail, and also new mail count */
    amount = mail_sizes(nl->mail_to, 1);
    if (!amount) {
        if (stat(infile, &stbuf) == -1) {
            size = 0;
        } else {
            size = stbuf.st_size;
        }
    } else {
        size = mail_sizes(nl->mail_to, 2);
    }
    /* write size of file and amount of new mail */
    fprintf(outfp, "%d %d\r", ++amount, size);
    infp = fopen(infile, "r");
    if (infp) {
        fscanf(infp, "%d %d\r", &tmp1, &tmp2);
        for (c = getc(infp); c != EOF; c = getc(infp)) {
            putc(c, outfp);
        }
        fclose(infp);
    }
    /* Copy received file */
    infp = fopen(mailfile, "r");
    if (!infp) {
        write_syslog(SYSLOG, 0,
                "ERROR: Cannot open file %s to read in netlink_endmail().\n",
                mailfile);
        sprintf(text, "%s %s %s\n", netcom[NLC_MAILERROR], nl->mail_to, nl->mail_from);
        write_sock(nl->socket, text);
        fclose(outfp);
        *nl->mail_to = '\0';
        *nl->mail_from = '\0';
        remove("tempfile");
        remove(mailfile);
        return;
    }
    fprintf(outfp, "~OLFrom: %s@%s  %s", nl->mail_from, nl->service,
            long_date(0));
    for (c = getc(infp); c != EOF; c = getc(infp)) {
        putc(c, outfp);
    }
    fclose(infp);
    fclose(outfp);
    rename("tempfile", infile);
    remove(mailfile);
    write_user(get_user(nl->mail_to), "\07~FC~OL~LI** YOU HAVE NEW MAIL **\n");
    *nl->mail_to = '\0';
    *nl->mail_from = '\0';
}

/*
 * An error occured at remote site
 */
void
nl_mailerror(NL_OBJECT nl, char *to, char *from)
{
    UR_OBJECT user;

    user = get_user(from);
    if (user) {
        sprintf(text,
                "~OLSYSTEM:~RS An error occured during mail delivery to %s@%s.\n",
                to, nl->service);
        write_user(user, text);
    } else {
        sprintf(text, "An error occured during mail delivery to %s@%s.\n", to,
                nl->service);
        send_mail(NULL, from, text, 0);
    }
}

/*
 * Send statistics of this server to requesting user on remote site
 */
void
nl_rstat(NL_OBJECT nl, char *to)
{
    if (nl->ver_major <= 3 && nl->ver_minor < 2) {
        sprintf(text, "%s %s\n\n*** Remote statistics ***\n\n",
                netcom[NLC_MESSAGE], to);
    } else {
        sprintf(text, "%s %s\n\n~BB*** Remote statistics ***\n\n",
                netcom[NLC_MESSAGE], to);
    }
    write_sock(nl->socket, text);
    sprintf(text, "NUTS version         : %s\nHost                 : %s\n",
            NUTSVER, amsys->uts.nodename);
    write_sock(nl->socket, text);
#ifdef WIZPORT
    sprintf(text, "Ports (Main/Wiz/Link): %s, %s, %s\n", amsys->mport_port,
            amsys->wport_port, amsys->nlink_port);
#else
    sprintf(text, "Ports (Main/Link): %s, %s\n", amsys->mport_port,
            amsys->nlink_port);
#endif
    write_sock(nl->socket, text);
    sprintf(text, "Number of users      : %d\nRemote user maxlevel : %s\n",
            amsys->num_of_users, user_level[amsys->rem_user_maxlevel].name);
    write_sock(nl->socket, text);
    sprintf(text, "Remote user deflevel : %s\n\n%s\n%s %s\n",
            user_level[amsys->rem_user_deflevel].name, netcom[NLC_ENDMESSAGE],
            netcom[NLC_PROMPT], to);
    write_sock(nl->socket, text);
}

/*
 * Shutdown the netlink and pull any remote users back home
 */
void
shutdown_netlink(NL_OBJECT nl)
{
    char mailfile[80];
    UR_OBJECT u;

    if (nl->type == UNCONNECTED) {
        return;
    }
    /* See if any message halfway through being sent */
    if (nl->mesg_user) {
        sprintf(text, "%s\n", netcom[NLC_ERROR]);
        write_sock(nl->socket, text);
        nl->mesg_user = NULL;
    }
    /* See if any mail halfway through being sent */
    if (*nl->mail_to) {
        sprintf(text, "%s %s %s\n", netcom[NLC_MAILERROR], nl->mail_to, nl->mail_from);
        write_sock(nl->socket, text);
        fclose(nl->mailfile);
        sprintf(mailfile, "%s/IN_%s_%s@%s", MAILSPOOL, nl->mail_to, nl->mail_from,
                nl->service);
        remove(mailfile);
        *nl->mail_to = '\0';
        *nl->mail_from = '\0';
    }
    sprintf(text, "%s\n", netcom[NLC_DISCONNECT]);
    write_sock(nl->socket, text);
    shutdown(nl->socket, SHUT_WR);
    close(nl->socket);
    for (u = user_first; u; u = u->next) {
        if (u->pot_netlink == nl) {
            u->pot_netlink = NULL;
        }
        if (u->netlink == nl) {
            if (!u->room) {
                write_user(u,
                        "~FB~OLYou feel yourself dragged back across the ether...\n");
                u->room = u->netlink->connect_room;
                u->netlink = NULL;
                if (u->vis) {
                    vwrite_room_except(u->room, u, "%s~RS %s\n", u->recap,
                            u->in_phrase);
                } else {
                    write_room_except(u->room, invisenter, u);
                }
                look(u);
                prompt(u);
                write_syslog(NETLOG, 1, "NETLINK: %s recovered from %s.\n", u->name,
                        nl->service);
                continue;
            }
            if (u->type == REMOTE_TYPE) {
                vwrite_room(u->room, "%s~RS vanishes!\n", u->recap);
                destruct_user(u);
                --amsys->num_of_users;
            }
        }
    }
    if (nl->stage == UP) {
        write_syslog(NETLOG, 1, "NETLINK: Disconnected from %s.\n", nl->service);
    } else {
        write_syslog(NETLOG, 1, "NETLINK: Disconnected from site %s.\n",
                nl->site);
    }
    if (nl->type == INCOMING) {
        nl->connect_room->netlink = NULL;
        destruct_netlink(nl);
        return;
    }
    nl->type = UNCONNECTED;
    nl->stage = DOWN;
    nl->warned = 0;
}

/******************************************************************************
 User executed functions that relate to the Netlinks
 *****************************************************************************/

int
transfer_nl(UR_OBJECT user)
{
    NL_OBJECT nl;
    RM_OBJECT rm;

    nl = user->room->netlink;
    if (!nl || strncasecmp(nl->service, word[1], strlen(word[1]))) {
        return 0;
    }
    if (user->pot_netlink == nl) {
        write_user(user,
                "The remote service may be lagged, please be patient...\n");
        return 1;
    }
    rm = user->room;
    if (nl->stage != UP) {
        write_user(user, "The netlink is inactive.\n");
        return 1;
    }
    if (nl->allow == IN && user->netlink != nl) {
        /* Link for incoming users only */
        write_user(user, "Sorry, link is for incoming users only.\n");
        return 1;
    }
    /* If site is users home site then tell home system that we have removed him. */
    if (user->netlink == nl) {
        write_user(user, "~FB~OLYou traverse cyberspace...\n");
        sprintf(text, "%s %s\n", netcom[NLC_REMOVED], user->name);
        write_sock(nl->socket, text);
        if (user->vis) {
            vwrite_room_except(rm, user, "%s~RS goes to the %s\n", user->recap,
                    nl->service);
        } else {
            write_room_except(rm, invisleave, user);
        }
        write_syslog(NETLOG, 1, "NETLINK: Remote user %s removed.\n", user->name);
        destroy_user_clones(user);
        destruct_user(user);
        reset_access(rm);
        --amsys->num_of_users;
        no_prompt = 1;
        return 1;
    }
    /*
     * Cannot let remote user jump to yet another remote site because
     * this will reset his user->netlink value and so we will lose
     * his original link.  2 netlink pointers are needed in the user
     * structure to allow this but it means way too much rehacking of
     * the code and I do not have the time or inclination to do it
     */
    if (user->type == REMOTE_TYPE) {
        write_user(user,
                "Sorry, due to software limitations you can only traverse one netlink.\n");
        return 1;
    }
    /* FIXME: Send unencrypted password for random salt */
    if (nl->ver_major <= 3 && nl->ver_minor <= 3 && nl->ver_patch < 1) {
        if (!*word[2]) {
            sprintf(text, "%s %s %s %s\n", netcom[NLC_TRANSFER], user->name,
                    user->pass, user->desc);
        } else {
            sprintf(text, "%s %s %s %s\n", netcom[NLC_TRANSFER], user->name,
                    crypt(word[2], crypt_salt), user->desc);
        }
    } else {
        if (!*word[2]) {
            sprintf(text, "%s %s %s %d %s\n", netcom[NLC_TRANSFER], user->name,
                    user->pass, user->level, user->desc);
        } else {
            sprintf(text, "%s %s %s %d %s\n", netcom[NLC_TRANSFER], user->name,
                    crypt(word[2], crypt_salt), user->level, user->desc);
        }
    }
    write_sock(nl->socket, text);
    user->pot_netlink = nl; /* potential netlink */
    no_prompt = 1;
    return 1;
}

int
release_nl(UR_OBJECT user)
{
    NL_OBJECT nl;

    if (!user->pot_netlink && user->room) {
        return 0;
    }
    if (user->pot_netlink) {
        sprintf(text, "%s %s\n", netcom[NLC_RELEASE], user->name);
        write_sock(user->pot_netlink->socket, text);
        user->pot_netlink = NULL;
    }
    if (!user->room) {
        sprintf(text, "%s %s\n", netcom[NLC_RELEASE], user->name);
        write_sock(user->netlink->socket, text);
        for (nl = nl_first; nl; nl = nl->next) {
            if (nl->mesg_user == user) {
                nl->mesg_user = (UR_OBJECT) - 1;
                break;
            }
        }
        user->room = user->netlink->connect_room;
        user->netlink = NULL;
    }
    return 1;
}

int
action_nl(UR_OBJECT user, const char *command, const char *inpstr)
{
    sds text;

    if (!user || user->type != REMOTE_TYPE) {
        return 0;
    }
    if (user->room) {
        return 0;
    }
    if (!command || !*command) {
        command = "NL";
    }
    if (!inpstr || !*inpstr) {
        text = sdscatfmt(sdsempty(), "%s %s %s\n", netcom[NLC_ACTION], user->name, command);
    } else {
        text = sdscatfmt(sdsempty(), "%s %s %s %s\n", netcom[NLC_ACTION], user->name, command, inpstr);
    }
    write_sock(user->netlink->socket, text);
    sdsfree(text);
    return 1;
}

int
message_nl(UR_OBJECT user, const char *str)
{
    char buff[OUT_BUFF_SIZE];

    if (!user || user->type != REMOTE_TYPE) {
        return 0;
    }
    /* FIXME: Can netlinks 1.2 take color because ver_minor >= 2!? */
    if (user->netlink->ver_major <= 3 && user->netlink->ver_minor < 2) {
        str = colour_com_strip(str);
    }
    /* FIXME: Bounds Checking! */
    if (str[strlen(str) - 1] != '\n') {
        sprintf(buff, "%s %s\n%s\n%s\n", netcom[NLC_MESSAGE], user->name, str,
                netcom[NLC_ENDMESSAGE]);
    } else {
        sprintf(buff, "%s %s\n%s%s\n", netcom[NLC_MESSAGE], user->name, str,
                netcom[NLC_ENDMESSAGE]);
    }
    write_sock(user->netlink->socket, buff);
    return 1;
}

int
prompt_nl(UR_OBJECT user)
{
    if (!user || user->type != REMOTE_TYPE) {
        return 0;
    }
    sprintf(text, "%s %s\n", netcom[NLC_PROMPT], user->name);
    write_sock(user->netlink->socket, text);
    return 1;
}

int
remove_nl(UR_OBJECT user)
{
    if (!user || user->type != REMOTE_TYPE) {
        return 0;
    }
    sprintf(text, "%s %s\n", netcom[NLC_REMOVED], user->name);
    write_sock(user->netlink->socket, text);
    return 1;
}

int
mail_nl(UR_OBJECT user, char *to, const char *mesg)
{
    char *service;
    NL_OBJECT nl;
    char filename[80];
    FILE *fp;

    /* See if remote mail */
    service = strchr(to, '@');
    if (!service) {
        return 0;
    }
    *service++ = '\0';
    for (nl = nl_first; nl; nl = nl->next) {
        if (!strcmp(nl->service, service)) {
            break;
        }
    }
    if (!nl || nl->stage != UP) {
        vwrite_user(user, "Service %s unavailable.\n", service);
        return -1;
    }
    /* Write out to spool file first */
    sprintf(filename, "%s/OUT_%s_%s@%s", MAILSPOOL, user->name, to,
            nl->service);
    fp = fopen(filename, "a");
    if (!fp) {
        vwrite_user(user, "%s: unable to spool mail.\n", syserror);
        write_syslog(SYSLOG | ERRLOG, 0,
                "ERROR: Cannot open file %s to append in mail_nl().\n",
                filename);
        return -1;
    }
    putc('\n', fp);
    fputs(mesg, fp);
    fclose(fp);
    /* Ask for verification of users existence */
    sprintf(text, "%s %s %s\n", netcom[NLC_EXISTSQUERY], to, user->name);
    write_sock(nl->socket, text);
    /* Rest of delivery process now up to netlink functions */
    write_user(user, "Mail sent to external talker.\n");
    return 1;
}

#else
#define NO_NETLINKS
#endif
