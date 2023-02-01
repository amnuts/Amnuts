/****************************************************************************
         Amnuts version 2.3.0 - Copyright (C) Andrew Collington, 2003
                      Last update: 2003-08-04

                              amnuts@talker.com
                          http://amnuts.talker.com/

                                   based on

   NUTS version 3.3.3 (Triple Three :) - Copyright (C) Neil Robertson 1996
 ***************************************************************************/

#ifndef __MAIN_FILE__
#define __MAIN_FILE__
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"
#undef __MAIN_FILE__
#endif

/**************************************************************************
 The setting up of the talker configs and the main program loop
 **************************************************************************/

int
main(int argc, char **argv)
{
    fd_set readmask;
    struct timeval timeout;
    time_t beat;
    int len;
    char inpstr[ARR_SIZE], future[ARR_SIZE], *next_str, *curstr, *last_ptr;
#ifdef IDENTD
    sds buffer;
    UR_OBJECT u;
#endif
    UR_OBJECT user, next;
#ifdef NETLINKS
    NL_OBJECT nl, nlnext;
#endif

    strcpy(progname, argv[0]);
    if (argc < 2) {
        strcpy(confile, CONFIGFILE);
    } else {
        strcpy(confile, argv[1]);
    }

    /* Startup and do initial counts and parses */
    init_signals();
    create_system();
    write_syslog(SYSLOG, 0,
            "------------------------------------------------------------------------------\nSERVER BOOTING\n");
    printf
            ("\n------------------------------------------------------------------------------\n");
    printf("Amnuts %s server booting %s\n", AMNUTSVER, long_date(1));
    printf
            ("------------------------------------------------------------------------------\n");
    printf("Node name   : %s\n", amsys->uts.nodename);
    printf("Running on  : %s %s %s %s\n", amsys->uts.machine,
            amsys->uts.sysname, amsys->uts.release, amsys->uts.version);
#ifdef NETLINKS
    printf("Netlinks    : Enabled\n");
#else
    printf("Netlinks    : Disabled\n");
#endif

    load_and_parse_config();

    printf("Flood protection is %s.\n", offon[amsys->flood_protect]);
    if (amsys->personal_rooms) {
        if (amsys->startup_room_parse) {
            parse_user_rooms();
        } else {
            printf("Personal rooms are active, but not being parsed at startup.\n");
        }
    } else {
        printf("Personal rooms disabled\n");
    }
    printf("Checking user directory structure\n");
    check_directories();
    printf("Processing user list\n");
    process_users();
    printf("Parsing command structure\n");
    parse_commands();

    if (amsys->auto_purge_date == -1) {
        write_syslog(SYSLOG, 0, "PURGE: Auto-purge is turned off.\n");
        printf("PURGE: Auto-purge is turned off\n");
    } else {
        purge(0, NULL, 0);
        printf("PURGE: Checked %d user%s, %d %s deleted due to lack of use.\n",
                amsys->purge_count, PLTEXT_S(amsys->purge_count),
                amsys->users_purged, PLTEXT_WAS(amsys->users_purged));
    }

    check_messages(NULL, 1);
    count_suggestions();
    printf("There %s %d suggestion%s.\n", PLTEXT_WAS(amsys->suggestion_count),
            amsys->suggestion_count, PLTEXT_S(amsys->suggestion_count));
    count_motds(0);
    printf("There %s %d login motd%s and %d post-login motd%s\n",
            PLTEXT_WAS(amsys->motd1_cnt), amsys->motd1_cnt,
            PLTEXT_S(amsys->motd1_cnt), amsys->motd2_cnt,
            PLTEXT_S(amsys->motd2_cnt));

    /* open the talker after everything else has parsed */
    if (!possibly_reboot()) {
#ifdef NETLINKS
#ifdef WIZPORT
        printf("Initialising sockets on ports: %s, %s, %s\n", amsys->mport_port,
                amsys->wport_port, amsys->nlink_port);
#else
        printf("Initialising sockets on ports: %s, %s\n", amsys->mport_port,
                amsys->nlink_port);
#endif
#else
#ifdef WIZPORT
        printf("Initialising sockets on ports: %s, %s\n", amsys->mport_port,
                amsys->wport_port);
#else
        printf("Initialising sockets on ports: %s\n", amsys->mport_port);
#endif
#endif
        amsys->mport_socket = socket_listen(NULL, amsys->mport_port);
        if (amsys->mport_socket < 0) {
            boot_exit(0 - amsys->mport_socket);
        }
#ifdef WIZPORT
        amsys->wport_socket = socket_listen(NULL, amsys->wport_port);
        if (amsys->mport_socket < 0) {
            boot_exit(1 - amsys->mport_socket);
        }
#endif
#ifdef NETLINKS
        amsys->nlink_socket = socket_listen(NULL, amsys->nlink_port);
        if (amsys->mport_socket < 0) {
            boot_exit(2 - amsys->mport_socket);
        }
#endif
    }

    /* Run in background automatically. */
    switch (fork()) {
    case -1:
        /* fork failure */
        boot_exit(11);
        break;
    case 0:
        /* child continues */
        break;
    default:
        /* parent dies */
        _exit(0);
    }
    /* XXX: Add setsid() and redirect stdio to /dev/null somewhere */

#ifdef NETLINKS
    if (amsys->auto_connect) {
        init_connections();
    } else {
        printf("Skipping connect stage.\n");
    }
#endif

    /* finish off the boot-up process */
    printf
            ("------------------------------------------------------------------------------\n");
    printf("Booted with PID %u\n", getpid());
    printf
            ("------------------------------------------------------------------------------\n\n");
    write_syslog(SYSLOG, 0,
            "------------------------------------------------------------------------------\nSERVER BOOTED with PID %u %s\n",
            getpid(), long_date(1));
    write_syslog(SYSLOG, 0,
            "------------------------------------------------------------------------------\n\n");


    /******************************************************************************
     Main program loop
     *****************************************************************************/


    beat = time(0) + amsys->heartbeat - 1;
    beat -= beat % amsys->heartbeat;
    for (;;) {
        /* dispatch heartbeat timer events */
        for (; beat <= time(0); beat += amsys->heartbeat) {
            check_reboot_shutdown();
            check_idle_and_timeout();
            /* Make this idempotent */
            if (amsys->mesg_check_done <= time(0)) {
                check_messages(NULL, 0);
                amsys->mesg_check_done += 86400;
            }
            if (amsys->auto_purge_date != -1 && amsys->auto_purge_date <= time(0)) {
                purge(0, NULL, 0);
                amsys->auto_purge_date += 86400;
            }
#ifdef NETLINKS
            check_nethangs_send_keepalives();
#endif
#ifdef GAMES
            check_credit_updates();
#endif
        }
        /* set up mask then wait */
        len = 1 + setup_readmask(&readmask);
        timeout.tv_sec = amsys->heartbeat;
        timeout.tv_usec = 0;
        len = select(len, &readmask, NULL, NULL, &timeout);
        if (len < 1) {
            continue;
        }
        /* check for connection to listen sockets */
        if (FD_ISSET(amsys->mport_socket, &readmask)) {
            accept_connection(amsys->mport_socket);
        }
#ifdef WIZPORT
        if (FD_ISSET(amsys->wport_socket, &readmask)) {
            accept_connection(amsys->wport_socket);
        }
#endif
#ifdef NETLINKS
        if (FD_ISSET(amsys->nlink_socket, &readmask)) {
            accept_server_connection(amsys->nlink_socket);
        }
#endif
#ifdef IDENTD
        if (amsys->ident_state == 1 && FD_ISSET(amsys->ident_socket, &readmask)) {
            len = recv(amsys->ident_socket, inpstr, (sizeof inpstr) - 3, 0);
            if (len < -1) {
                abort();
            }
            if (len == -1) {
                switch (errno) {
                case ECONNRESET:
                case ETIMEDOUT:
                    len = 0;
                    break;
                default:
                    continue;
                    break;
                }
            }
            if (!len) {
                write_level(WIZ, 1, NORECORD, "~FY<ArIdent Daemon Disconnected>\n",
                        NULL);
                amsys->ident_state = 0;
                continue;
            }
            inpstr[len] = '\0';
            if (!strncmp(inpstr, "RETURN:", 7)) {
                wordfind(inpstr);
                for (u = user_first; u; u = next) {
                    next = u->next;
                    if (!strncmp(u->site, word[1], strlen(word[1]) - 1)) {
                        *u->site = '\0';
                        strncat(u->site, word[2], (sizeof u->site) - 1);
                        if (site_banned(u->site, 0)) {
                            write_user(u,
                                    "\nSorry, logins from your site have been banned.\n\n");
                            disconnect_user(u);
                        }
                    }
                }
            }
            if (!strncmp(inpstr, "ARETURN:", 8)) {
                wordfind(inpstr);
                for (u = user_first; u; u = next) {
                    next = u->next;
                    if (!strcmp(word[1], u->site_port) && atoi(word[2]) == u->socket) {
                        if (!strchr(u->site, '@')) {
                            buffer = sdscatfmt(sdsempty(), "%s@%s", word[3], u->site);
                            *u->site = '\0';
                            strncat(u->site, buffer, (sizeof u->site) - 1);
                            sdsfree(buffer);
                            if (site_banned(u->site, 0)) {
                                write_user(u,
                                        "\nSorry, logins from your site have been banned.\n\n");
                                disconnect_user(u);
                            }
                        }
                    }
                }
            }
            if (!strncmp(inpstr, "PRETURN:", 8)) {
                wordfind(inpstr);
                amsys->ident_pid = atoi(word[1]);
            }
        }
#endif
#ifdef NETLINKS
        /* Cycle through client-server connections to other talkers */
        for (nl = nl_first; nl; nl = nlnext) {
            nlnext = nl->next;
            no_prompt = 0;
            if (nl->type == UNCONNECTED || !FD_ISSET(nl->socket, &readmask)) {
                continue;
            }
            /* See if remote site has disconnected */
            len = recv(nl->socket, inpstr, (sizeof inpstr) - 3, 0);
            if (len < -1) {
                abort();
            }
            if (len == -1) {
                switch (errno) {
                case ECONNRESET:
                case ETIMEDOUT:
                    len = 0;
                    break;
                default:
                    continue;
                    break;
                }
            }
            if (!len) {
                write_syslog(NETLOG, 1, "NETLINK: Remote disconnect by %s.\n",
                        (nl->stage == UP) ? nl->service : nl->site);
                vwrite_room(NULL, "~OLSYSTEM:~RS Lost link to %s in the %s.\n",
                        nl->service, nl->connect_room->name);
                shutdown_netlink(nl);
                continue;
            }
            inpstr[len] = '\0';
            exec_netcom(nl, inpstr);
        }
#endif
        /*
           Cycle through users. Save user->next first because
           user structure may be destructed during loop in which case we
           may lose the user->next link.
         */
        for (user = user_first; user; user = next) {
            /* store in case user object is destructed */
            for (next = user->next; next; next = next->next) {
                if (next->type != CLONE_TYPE) {
                    break; /* bug fix for clone destruction crash */
                }
            }
            /* If remote user or clone ignore */
            if (user->type != USER_TYPE) {
                continue;
            }
            /* see if any data on socket else continue */
            if (!FD_ISSET(user->socket, &readmask)) {
                continue;
            }
            /* see if client (eg telnet) has closed socket */
            *inpstr = '\0';
            len = recv(user->socket, inpstr, (sizeof inpstr), 0);
            if (len < -1) {
                abort();
            }
            if (len == -1) {
                switch (errno) {
                case ECONNRESET:
                case ETIMEDOUT:
                    len = 0;
                    break;
                default:
                    continue;
                    break;
                }
            }
            if (!len) {
                disconnect_user(user);
                continue;
            }
            /* ignore control code replies */
            if (*inpstr == '\xff') {
                continue;
            }
            /*
               Deal with input chars. If the following if test succeeds we
               are dealing with a character mode client so call function.
             */
            if (!iscntrl((int) inpstr[len - 1]) || user->buffpos) {
                if (!get_charclient_line(user, inpstr, len)) {
                    continue;
                }
            }
            curstr = next_str = inpstr;
            last_ptr = inpstr + len;
            for (;;) {
                curstr = next_str;
                if (!curstr) {
                    break;
                }
                strcpy(future, curstr);
                terminate(future);
                next_str = strlen(future) + curstr + 1;
                while (*next_str == '\n' || *next_str == '\r') {
                    ++next_str;
                }
                curstr = future;
                if (next_str >= last_ptr - 2) {
                    next_str = NULL;
                }

                no_prompt = 0;
                force_listen = 0;
                destructed = 0;
                *user->buff = '\0';
                user->buffpos = 0;
                user->last_input = time(0);
                if (user->login > 0) {
                    login(user, curstr);
                    continue;
                }
                /*
                 * If a dot on its own then execute last inpstr unless its a
                 * misc op or the user is on a remote site
                 */
                if (!user->misc_op) {
                    if ((!strcmp(curstr, ".")) && *user->inpstr_old) {
                        strcpy(curstr, user->inpstr_old);
                        vwrite_user(user, "%s\n", curstr);
                    }/* else save current one for next time */
                    else {
                        if (*curstr) {
                            *user->inpstr_old = '\0';
                            strncat(user->inpstr_old, curstr, REVIEW_LEN);
                        }
                    }
                }
                /* Main input check */
                clear_words();
                check_macros(user, curstr);
                word_count = wordfind(curstr);
                if (user->afk) {
                    if (user->afk == 2) {
                        if (!word_count) {
                            if (user->command_mode) {
                                prompt(user);
                            }
                            continue;
                        }
                        if (strcmp(user->pass, crypt(word[0], user->pass))) {
                            write_user(user, "Incorrect password.\n");
                            prompt(user);
                            continue;
                        }
                        cls(user);
                        write_user(user, "Session unlocked, you are no longer AFK.\n");
                    } else {
                        write_user(user, "You are no longer AFK.\n");
                    }
                    *user->afk_mesg = '\0';
                    if (has_review(user, rbfAFK)) {
                        write_user(user,
                                "\nYou have some tells in your afk review buffer.  Use ~FCrevafk~RS to view them.\n\n");
                    }
                    if (user->vis) {
                        vwrite_room_except(user->room, user,
                                "%s~RS comes back from being AFK.\n",
                                user->recap);
                    }
                    if (user->afk == 2) {
                        user->afk = 0;
                        prompt(user);
                        continue;
                    }
                    user->afk = 0;
                }
                if (!word_count) {
                    if (misc_ops(user, curstr)) {
                        continue;
                    }
#ifdef NETLINKS
                    action_nl(user, "", NULL);
#endif
                    if (user->command_mode) {
                        prompt(user);
                    }
                    continue;
                }
                if (misc_ops(user, curstr)) {
                    continue;
                }
                if (!word_count) {
                    if (user->command_mode) {
                        prompt(user);
                    }
                    continue;
                }
                com_num = COUNT;
                exec_com(user, curstr, user->command_mode ? COUNT : SAY);
                if (!destructed) {
                    if (user->room) {
                        prompt(user);
                    } else {
                        switch (com_num) {
#ifdef NETLINKS
                        case HOME:
#endif
                        case QUIT:
                        case MODE:
                        case PROMPT:
                        case SUICIDE:
                        case REBOOT:
                        case SHUTDOWN:
                            prompt(user);
                            break;
                        default: /* Not in enumerated values - Unknown command */
                            break;
                        }
                    }
                }

            }
        }
    }
    return 0; /* This does not seem to be possible */
}



/******************************************************************************
 General functions used by the talker
 *****************************************************************************/

/*
 * Check to see if the directory structure in USERFILES is correct, ie, there
 * is one directory for each of the level names given in *user_level[]
 * Also, check if level names are unique.
 */
void
check_directories(void)
{
    char dirname[80];
    struct stat stbuf;
    int i, j;

    /* Check for unique directory names */
    for (i = 0; i < NUM_LEVELS; ++i) {
        for (j = i + 1; j < NUM_LEVELS; ++j) {
            if (!strcmp(user_level[i].name, user_level[j].name)) {
                fprintf(stderr, "Amnuts: Level names are not unique.\n");
                boot_exit(14);
            }
        }
    }
    i = 0;
    /* check the directories needed exist */
    strcpy(dirname, USERFILES);
    if (stat(dirname, &stbuf) == -1) {
        fprintf(stderr,
                "Amnuts: Directory stat failure in check_directories().\n");
        boot_exit(15);
    }
    if (!S_ISDIR(stbuf.st_mode)) {
        fprintf(stderr, "Amnuts: Directory structure is incorrect.\n");
        boot_exit(16);
    }
    sprintf(dirname, "%s/%s", USERFILES, USERMAILS);
    if (stat(dirname, &stbuf) == -1) {
        fprintf(stderr,
                "Amnuts: Directory stat failure in check_directories().\n");
        boot_exit(15);
    }
    if (!S_ISDIR(stbuf.st_mode)) {
        fprintf(stderr, "Amnuts: Directory structure is incorrect.\n");
        boot_exit(16);
    }
    sprintf(dirname, "%s/%s", USERFILES, USERPROFILES);
    if (stat(dirname, &stbuf) == -1) {
        fprintf(stderr,
                "Amnuts: Directory stat failure in check_directories().\n");
        boot_exit(15);
    }
    if (!S_ISDIR(stbuf.st_mode)) {
        fprintf(stderr, "Amnuts: Directory structure is incorrect.\n");
        boot_exit(16);
    }
    sprintf(dirname, "%s/%s", USERFILES, USERHISTORYS);
    if (stat(dirname, &stbuf) == -1) {
        fprintf(stderr,
                "Amnuts: Directory stat failure in check_directories().\n");
        boot_exit(15);
    }
    if (!S_ISDIR(stbuf.st_mode)) {
        fprintf(stderr, "Amnuts: Directory structure is incorrect.\n");
        boot_exit(16);
    }
    sprintf(dirname, "%s/%s", USERFILES, USERCOMMANDS);
    if (stat(dirname, &stbuf) == -1) {
        fprintf(stderr,
                "Amnuts: Directory stat failure in check_directories().\n");
        boot_exit(15);
    }
    if (!S_ISDIR(stbuf.st_mode)) {
        fprintf(stderr, "Amnuts: Directory structure is incorrect.\n");
        boot_exit(16);
    }
    sprintf(dirname, "%s/%s", USERFILES, USERMACROS);
    if (stat(dirname, &stbuf) == -1) {
        fprintf(stderr,
                "Amnuts: Directory stat failure in check_directories().\n");
        boot_exit(15);
    }
    if (!S_ISDIR(stbuf.st_mode)) {
        fprintf(stderr, "Amnuts: Directory structure is incorrect.\n");
        boot_exit(16);
    }
    sprintf(dirname, "%s/%s", USERFILES, USERROOMS);
    if (stat(dirname, &stbuf) == -1) {
        fprintf(stderr,
                "Amnuts: Directory stat failure in check_directories().\n");
        boot_exit(15);
    }
    if (!S_ISDIR(stbuf.st_mode)) {
        fprintf(stderr, "Amnuts: Directory structure is incorrect.\n");
        boot_exit(16);
    }
}

/*
 * find out if a user is listed in the user linked list
 */
int
find_user_listed(const char *name)
{
    UD_OBJECT entry;

    for (entry = first_user_entry; entry; entry = entry->next) {
        if (!strcasecmp(entry->name, name)) {
            break;
        }
    }
    return !!entry;
}

/*
 * Checks to see if a user with the given name is currently logged on
 */
int
user_logged_on(const char *name)
{
    UR_OBJECT u;

    for (u = user_first; u; u = u->next) {
        if (u->login) {
            continue;
        }
        if (!strcasecmp(name, u->name)) {
            break;
        }
    }
    return !!u;
}





/******************************************************************************
 Setting up of the sockets
 *****************************************************************************/

/*
 * Set up readmask for select
 */
int
setup_readmask(fd_set * mask)
{
    UR_OBJECT user;
#ifdef NETLINKS
    NL_OBJECT nl;
#endif
    int fdmax;

    FD_ZERO(mask);
    FD_SET(amsys->mport_socket, mask);
    fdmax = amsys->mport_socket;
#ifdef WIZPORT
    FD_SET(amsys->wport_socket, mask);
    if (fdmax < amsys->wport_socket) {
        fdmax = amsys->wport_socket;
    }
#endif
    /* Do users */
    for (user = user_first; user; user = user->next) {
        if (user->type != USER_TYPE) {
            continue;
        }
        FD_SET(user->socket, mask);
        if (fdmax < user->socket) {
            fdmax = user->socket;
        }
    }
    /* Do client-server stuff */
#ifdef NETLINKS
    FD_SET(amsys->nlink_socket, mask);
    if (fdmax < amsys->nlink_socket) {
        fdmax = amsys->nlink_socket;
    }
    for (nl = nl_first; nl; nl = nl->next) {
        if (nl->type != UNCONNECTED) {
            FD_SET(nl->socket, mask);
            if (fdmax < nl->socket) {
                fdmax = nl->socket;
            }
        }
    }
#endif
#ifdef IDENTD
    /* Do ident stuff */
    if (amsys->resolve_ip == 3 && amsys->ident_state) {
        FD_SET(amsys->ident_socket, mask);
        if (fdmax < amsys->ident_socket) {
            fdmax = amsys->ident_socket;
        }
    }
#endif
    return fdmax;
}

/*
 * Accept incoming connections on listen sockets
 */
void
accept_connection(int lsock)
{
#ifdef IDENTD
    sds buffer;
#endif
    char hostaddr[MAXADDR]; /* XXX: Use NI_MAXHOST, INET_ADDRSTRLEN, INET6_ADDRSTRLEN */
    char hostname[MAXHOST]; /* XXX: Use NI_MAXHOST */
    char motdname[80];
    struct sockaddr_in sa;
    UR_OBJECT user;
    int accept_sock;
    socklen_t sa_len;

    sa_len = (sizeof sa);
    accept_sock = accept(lsock, (struct sockaddr *) &sa, &sa_len);
    if (accept_sock == -1) {
        return;
    }
    *hostname = *hostaddr = '\0';
    /* Get number addr */
    strcpy(hostaddr, inet_ntoa(sa.sin_addr));
    /*
       Get named site
       Hanging usually happens on BSD systems when using gethostbyaddr.  If this happens
       to you then alter the resolve_ip setting in the config file.
     */
    switch (amsys->resolve_ip) {
    default:
        /* do not resolve */
        strcpy(hostname, hostaddr);
        break;
    case 1:
        /* resolve automatically */
    {
        struct hostent *he;

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
        break;
#ifdef MANDNS
    case 2:
        /* resolve with function by tref */
        strcpy(hostname, resolve_ip(hostaddr));
        break;
#endif
#ifdef IDENTD
    case 3:
        /* resolve using identd (ArIdent) */
        strcpy(hostname, hostaddr);
        if (amsys->ident_state) {
            sprintf(buffer, "SITE: %s\n", hostaddr);
            write_sock(amsys->ident_socket, buffer);
        }
        break;
#endif
    }
    if (site_banned(hostaddr, 0) || site_banned(hostname, 0)) {
        write_sock(accept_sock,
                "\n\rLogins from your site/domain are banned.\n\n\r");
        close(accept_sock);
        write_syslog(SYSLOG, 1, "Attempted login from banned site %s (%s).\n",
                hostname, hostaddr);
        return;
    }
    if (amsys->flood_protect
            && (login_port_flood(hostaddr) || login_port_flood(hostname))) {
        write_sock(accept_sock,
                "\n\rYou have attempted to flood the talker ports. We view this as\n\r");
        write_sock(accept_sock,
                "a sign that you are trying to hack this system.  Therefore you have now\n\r");
        write_sock(accept_sock, "had logins from your site/domain banned.\n\n\r");
        close(accept_sock);
        write_syslog(SYSLOG, 1, "Attempted port flood from site %s (%s).\n",
                hostname, hostaddr);
        auto_ban_site(hostaddr);
        return;
    }
    /* get random motd1 and send  pre-login message */
    if (amsys->motd1_cnt) {
        sprintf(motdname, "%s/motd1/motd%d", MOTDFILES, (get_motd_num(1)));
        more(NULL, accept_sock, motdname);
    } else {
        sprintf(text,
                "Welcome to %s!\n\n\rSorry, but the login screen appears to be missing at this time.\n\r",
                TALKER_NAME);
        write_sock(accept_sock, text);
    }
    /* check to see if logins are stopped--does not apply to wizport */
#ifdef WIZPORT
    if (lsock != amsys->wport_socket)
#endif
    {
        if (amsys->stop_logins) {
            write_sock(accept_sock,
                    "\n\rSorry, but no connections can be made at the moment.\n\rPlease try later.\n\n\r");
            close(accept_sock);
            return;
        }
        if (amsys->num_of_users + amsys->num_of_logins >= amsys->max_users) {
            write_sock(accept_sock,
                    "\n\rSorry, but we cannot accept any more connections at the moment.\n\rPlease try later.\n\n\r");
            close(accept_sock);
            return;
        }
    }
    user = create_user();
    if (!user) {
        sprintf(text, "\n\r%s: unable to create session.\n\n\r", syserror);
        write_sock(accept_sock, text);
        close(accept_sock);
        return;
    }
    user->socket = accept_sock;
    user->login = LOGIN_NAME;
    user->last_input = time(0);
#ifdef WIZPORT
    user->wizport = lsock == amsys->wport_socket;
    if (user->wizport) {
        write_user(user, "** Wizport login **\n\n");
    }
#endif
    strcpy(user->site, hostname);
    strcpy(user->ipsite, hostaddr);
    sprintf(user->site_port, "%d", ntohs(sa.sin_port));
    echo_on(user);
    write_user(user, "Give me a name: ");
    ++amsys->num_of_logins;
#ifdef IDENTD
    if (amsys->resolve_ip == 3 && amsys->ident_state) {
#ifdef WIZPORT
        buffer = sdscatfmt(sdsempty(), "AUTH: %d %s %s %s\n", user->socket, user->site_port,
                !user->wizport ? amsys->mport_port : amsys->wport_port,
                user->site);
#else
        buffer = sdscatfmt(sdsempty(), "AUTH: %d %s %s %s\n", user->socket, user->site_port,
                amsys->mport_port, user->site);
#endif
        write_sock(amsys->ident_socket, buffer);
        sdsfree(buffer);
    }
#endif
}


#ifdef MANDNS

/*
 * Resolve an IP address if the resolve_ip set to MANUAL in config file.
 * This code was written by tref, and submitted to ewtoo.org by ruGG.
 * Claimed useful for BSD systems in which gethostbyaddr() calls caused
 * extreme hanging/blocking of the talker. Note, popen is a blocking
 * call however.
 */
char *
resolve_ip(char *host)
{
    static char str[256];
    char *txt, *t;
    FILE *hp;

    sprintf(str, "/usr/bin/host %s", host);
    hp = popen(str, "r");

    *str = 0;
    fgets(str, 255, hp);
    pclose(hp);

    txt = strchr(str, ':');
    if (txt) {
        for (++txt; *txt; ++txt) {
            if (!isspace(*txt)) {
                break;
            }
        }
        for (t = txt; *t; ++t) {
            if (isspace(*txt)) {
                break;
            }
        }
        *t = '\0';
        return txt;
    }
    return host;
}
#endif

/* Initialize listen socket on port */
int
socket_listen(const char *host, const char *serv)
{
    struct sockaddr_in sa;
    int s, rc;

    /* create socket */
    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == -1) {
        return -2;
    }
    /* allow reboots on port even with TIME_WAITS */
    rc = 1;
    rc = setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &rc, (sizeof rc));
#if 0
    if (rc == -1) {
        /* Do Something */;
    }
#endif
    /* bind socket and set up listen queue */
    sa.sin_family = AF_INET;
    sa.sin_port = !serv ? htons((uint16_t) 0) : htons((uint16_t) atoi(serv));
    /* FIXME: Do name lookup on "host" and error checking */
    sa.sin_addr.s_addr = !host ? htonl((uint32_t) INADDR_ANY) : inet_addr(host);
    rc = bind(s, (struct sockaddr *) &sa, (sizeof sa));
    if (rc == -1) {
        return -5;
    }
    rc = listen(s, SOMAXCONN);
    if (rc == -1) {
        return -8;
    }
    /* Set to non-blocking */
    rc = 0;
    rc = fcntl(s, F_GETFL, rc);
#if 0
    if (rc == -1) {
        /* Do Something */;
    }
#endif
    rc |= O_NONBLOCK;
    rc = fcntl(s, F_SETFL, rc);
#if 0
    if (rc == -1) {
        /* Do Something */;
    }
#endif
    return s;
}


/******************************************************************************
 The loading up and parsing of the configuration file
 *****************************************************************************/


#define ML_ENTRY(a) ML_EXPAND a

#define CONFIG_LIST \
  ML_ENTRY((INIT,   "INIT"  )) \
  ML_ENTRY((ROOMS,  "ROOMS" )) \
  ML_ENTRY((TOPICS, "TOPICS")) \
  ML_ENTRY((SITES,  "SITES" )) \
  ML_ENTRY((COUNT,  NULL    ))

void
load_and_parse_config(void)
{

    enum config_value {
#define ML_EXPAND(value,name) CONFIG_ ## value,
        CONFIG_LIST
#undef ML_EXPAND
    };
    static const char *const sections[] = {
#define ML_EXPAND(value,name) name,
        CONFIG_LIST
#undef ML_EXPAND
    };
    FILE *fp;
    char line[81]; /* Should be long enough */
    char filename[80], *s, *t;
    const char *const *section;
    int c, i;
    unsigned got_sections;
    RM_OBJECT rm1, rm2;
#ifdef NETLINKS
    NL_OBJECT nl;
#endif

    printf("Parsing config file \"%s\"...\n", confile);
    sprintf(filename, "%s/%s", DATAFILES, confile);
    fp = fopen(filename, "r");
    if (!fp) {
        perror("Amnuts: Cannot open config file");
        boot_exit(1);
    }
    section = NULL;
    got_sections = 0;
    /* Main reading loop */
    config_line = 0;
    for (s = fgets(line, 81, fp); s; s = fgets(line, 81, fp)) {
        ++config_line;
        /* Handle comments */
        t = strchr(s, '#');
        if (t) {
            *t = '\0';
        }
        for (i = 0; i < 8; ++i) {
            *wrd[i] = '\0';
        }
        sscanf(s, "%s %s %s %s %s %s %s %s", wrd[0], wrd[1], wrd[2], wrd[3],
                wrd[4], wrd[5], wrd[6], wrd[7]);
        if (!*wrd[0]) {
            continue;
        }
        /* See if new section */
        t = strchr(wrd[0], ':');
        if (t && !t[1]) {
            *t = '\0';
            for (section = sections; *section; ++section) {
                if (!strcmp(wrd[0], *section)) {
                    break;
                }
            }
            if (!*section) {
                fprintf(stderr, "Amnuts: Unknown section header on line %d.\n",
                        config_line);
                fclose(fp);
                boot_exit(1);
            }
            if (got_sections & BIT(section - sections)) {
                fprintf(stderr, "Amnuts: Unexpected %s section header on line %d.\n",
                        *section, config_line);
                fclose(fp);
                boot_exit(1);
            }
            got_sections |= BIT(section - sections);
            continue;
        }
        if (!section) {
            fprintf(stderr, "Amnuts: Section header expected on line %d.\n",
                    config_line);
            fclose(fp);
            boot_exit(1);
        }
        switch ((enum config_value) (section - sections)) {
        case CONFIG_INIT:
            parse_init_section();
            break;
        case CONFIG_ROOMS:
            parse_rooms_section();
            break;
        case CONFIG_TOPICS:
            parse_topics_section(remove_first(s));
            break;
        case CONFIG_SITES:
#ifdef NETLINKS
            parse_sites_section();
#endif
            break;
        default:
            break;
        }
    }
    fclose(fp);
    /*
     * See if required sections were present (SITES and TOPICS is optional)
     * and if required parameters were set.
     */
    if (!(got_sections & BIT(CONFIG_INIT))) {
        fprintf(stderr, "Amnuts: INIT section missing from config file.\n");
        boot_exit(1);
    }
    if (!(got_sections & BIT(CONFIG_ROOMS))) {
        fprintf(stderr, "Amnuts: ROOMS section missing from config file.\n");
        if (got_sections & BIT(CONFIG_TOPICS)) {
            fprintf(stderr,
                    "Amnuts: TOPICS section must come after ROOMS section in the config file.\n");
        }
        boot_exit(1);
    }
    if (!*amsys->mport_port) {
        fprintf(stderr, "Amnuts: Main port number not set in config file.\n");
        boot_exit(1);
    }
#ifdef WIZPORT
    if (!*amsys->wport_port) {
        fprintf(stderr, "Amnuts: Wiz port number not set in config file.\n");
        boot_exit(1);
    }
    if (!strcmp(amsys->mport_port, amsys->wport_port)) {
        fprintf(stderr, "Amnuts: Port numbers must be unique.\n");
        boot_exit(1);
    }
#endif
#ifdef NETLINKS
    if (!*amsys->nlink_port) {
        fprintf(stderr, "Amnuts: Link port number not set in config file.\n");
        boot_exit(1);
    }
    if (!*amsys->verification) {
        fprintf(stderr, "Amnuts: Verification not set in config file.\n");
        boot_exit(1);
    }
    if (!strcmp(amsys->mport_port, amsys->nlink_port)) {
        fprintf(stderr, "Amnuts: Port numbers must be unique.\n");
        boot_exit(1);
    }
#ifdef WIZPORT
    if (!strcmp(amsys->wport_port, amsys->nlink_port)) {
        fprintf(stderr, "Amnuts: Port numbers must be unique.\n");
        boot_exit(1);
    }
#endif
#endif
    if (!room_first) {
        fprintf(stderr, "Amnuts: No rooms configured in config file.\n");
        boot_exit(1);
    }

    /* Parsing done, now check data is valid. Check room stuff first. */
    for (rm1 = room_first; rm1; rm1 = rm1->next) {
        for (i = 0; i < MAX_LINKS; ++i) {
            if (!*rm1->link_label[i]) {
                break;
            }
            for (rm2 = room_first; rm2; rm2 = rm2->next) {
                if (!strcmp(rm1->link_label[i], rm2->label)) {
                    break;
                }
            }
            if (!rm2) {
                fprintf(stderr, "Amnuts: Room %s has undefined link label \"%s\".\n",
                        rm1->name, rm1->link_label[i]);
                boot_exit(1);
            }
            if (rm2 == rm1) {
                fprintf(stderr, "Amnuts: Room %s cannot link to itself.\n",
                        rm1->name);
                boot_exit(1);
            }
            rm1->link[i] = rm2;
        }
    }

#ifdef NETLINKS
    /* Check service names */
    for (nl = nl_first; nl; nl = nl->next) {
        for (rm1 = room_first; rm1; rm1 = rm1->next) {
            if (!strcmp(nl->service, rm1->name)) {
                break;
            }
        }
        if (rm1) {
            fprintf(stderr, "Amnuts: Service name %s is also the name of a room.\n",
                    nl->service);
            boot_exit(1);
        }
    }

    /* Check external links */
    for (rm1 = room_first; rm1; rm1 = rm1->next) {
        if (!*rm1->netlink_name) {
            continue;
        }
        for (nl = nl_first; nl; nl = nl->next) {
            if (!strcmp(rm1->netlink_name, nl->service)) {
                break;
            }
        }
        if (!nl) {
            fprintf(stderr, "Amnuts: Service name %s not defined for room %s.\n",
                    rm1->netlink_name, rm1->name);
            boot_exit(1);
        }
        rm1->netlink = nl;
    }
#endif

    /* Load room descriptions */
    for (rm1 = room_first; rm1; rm1 = rm1->next) {
        sprintf(filename, "%s/%s.R", DATAFILES, rm1->name);
        fp = fopen(filename, "r");
        if (!fp) {
            fprintf(stderr, "Amnuts: Cannot open description file for room %s.\n",
                    rm1->name);
            write_syslog(SYSLOG | ERRLOG, 0,
                    "ERROR: Cannot open description file for room %s.\n",
                    rm1->name);
            continue;
        }
        i = 0;
        for (c = getc(fp); c != EOF; c = getc(fp)) {
            if (i == ROOM_DESC_LEN) {
                break;
            }
            rm1->desc[i++] = c;
        }
        if (c != EOF) {
            fprintf(stderr, "Amnuts: Description too long for room %s.\n",
                    rm1->name);
            write_syslog(SYSLOG | ERRLOG, 0,
                    "ERROR: Description too long for room %s.\n", rm1->name);
        }
        rm1->desc[i++] = '\0';
        fclose(fp);
    }
}

#define INITOPT_LIST \
  ML_ENTRY((MAIN_PORT,          "mainport"          )) \
  ML_ENTRY((WIZ_PORT,           "wizport"           )) \
  ML_ENTRY((LINK_PORT,          "linkport"          )) \
  ML_ENTRY((SYSTEM_LOGGING,     "system_logging"    )) \
  ML_ENTRY((MINLOGIN_LEVEL,     "minlogin_level"    )) \
  ML_ENTRY((MESG_LIFE,          "mesg_life"         )) \
  ML_ENTRY((WIZPORT_LEVEL,      "wizport_level"     )) \
  ML_ENTRY((PROMPT_DEF,         "prompt_def"        )) \
  ML_ENTRY((GATECRASH_LEVEL,    "gatecrash_level"   )) \
  ML_ENTRY((MIN_PRIVATE,        "min_private"       )) \
  ML_ENTRY((IGNORE_MP_LEVEL,    "ignore_mp_level"   )) \
  ML_ENTRY((REM_USER_MAXLEVEL,  "rem_user_maxlevel" )) \
  ML_ENTRY((REM_USER_DEFLEVEL,  "rem_user_deflevel" )) \
  ML_ENTRY((VERIFICATION,       "verification"      )) \
  ML_ENTRY((MESG_CHECK_TIME,    "mesg_check_time"   )) \
  ML_ENTRY((MAX_USERS,          "max_users"         )) \
  ML_ENTRY((HEARTBEAT,          "heartbeat"         )) \
  ML_ENTRY((LOGIN_IDLE_TIME,    "login_idle_time"   )) \
  ML_ENTRY((USER_IDLE_TIME,     "user_idle_time"    )) \
  ML_ENTRY((PASSWORDECHO_DEF,   "passwordecho_def"  )) \
  ML_ENTRY((IGNORE_SIGTERM,     "ignore_sigterm"    )) \
  ML_ENTRY((AUTO_CONNECT,       "auto_connect"      )) \
  ML_ENTRY((MAX_CLONES,         "max_clones"        )) \
  ML_ENTRY((BAN_SWEARING,       "ban_swearing"      )) \
  ML_ENTRY((CRASH_ACTION,       "crash_action"      )) \
  ML_ENTRY((COLOUR_DEF,         "colour_def"        )) \
  ML_ENTRY((TIME_OUT_AFKS,      "time_out_afks"     )) \
  ML_ENTRY((CHARECHO_DEF,       "charecho_def"      )) \
  ML_ENTRY((TIME_OUT_MAXLEVEL,  "time_out_maxlevel" )) \
  ML_ENTRY((AUTO_PURGE,         "auto_purge"        )) \
  ML_ENTRY((ALLOW_RECAPS,       "allow_recaps"      )) \
  ML_ENTRY((AUTO_PROMOTE,       "auto_promote"      )) \
  ML_ENTRY((PERSONAL_ROOMS,     "personal_rooms"    )) \
  ML_ENTRY((RANDOM_MOTDS,       "random_motds"      )) \
  ML_ENTRY((STARTUP_ROOM_PARSE, "startup_room_parse")) \
  ML_ENTRY((RESOLVE_IP,         "resolve_ip"        )) \
  ML_ENTRY((FLOOD_PROTECT,      "flood_protect"     )) \
  ML_ENTRY((BOOT_OFF_MIN,       "boot_off_min"      )) \
  ML_ENTRY((DEF_WARP,           "default_warp"      )) \
  ML_ENTRY((DEF_JAIL,           "default_jail"      )) \
  ML_ENTRY((DEF_BANK,           "default_bank"      )) \
  ML_ENTRY((DEF_SHOOT,          "default_shoot"     )) \
  ML_ENTRY((COUNT,              NULL                ))

/*
 * Parse init section
 */
void
parse_init_section(void)
{

    enum initopt_value {
#define ML_EXPAND(value,name) INITOPT_ ## value,
        INITOPT_LIST
#undef ML_EXPAND
    };
    static const char *const options[] = {
#define ML_EXPAND(value,name) name,
        INITOPT_LIST
#undef ML_EXPAND
    };
    int val;
    const char *const *initopt;

    for (initopt = options; *initopt; ++initopt) {
        if (!strcmp(*initopt, wrd[0])) {
            break;
        }
    }
    if (!*initopt) {
        fprintf(stderr, "Amnuts: Unknown INIT option on line %d.\n", config_line);
        boot_exit(1);
    }
    if (!*wrd[1]) {
        fprintf(stderr, "Amnuts: Required parameter missing on line %d.\n",
                config_line);
        boot_exit(1);
    }
    if (*wrd[2]) {
        fprintf(stderr,
                "Amnuts: Unexpected word following init parameter on line %d.\n",
                config_line);
        boot_exit(1);
    }
    val = atoi(wrd[1]);
    switch ((enum initopt_value) (initopt - options)) {

    case INITOPT_MAIN_PORT:
        /* main port */
        if (strlen(wrd[1]) >= MAXSERV) { /* XXX: Use NI_MAXSERV */
            fprintf(stderr, "Amnuts: %s has Illegal port number on line %d.\n",
                    *initopt, config_line);
            boot_exit(1);
        }
        strcpy(amsys->mport_port, wrd[1]);
        break;

    case INITOPT_WIZ_PORT:
        /* wiz */
        if (strlen(wrd[1]) >= MAXSERV) { /* XXX: Use NI_MAXSERV */
            fprintf(stderr, "Amnuts: %s has Illegal port number on line %d.\n",
                    *initopt, config_line);
            boot_exit(1);
        }
#ifdef WIZPORT
        strcpy(amsys->wport_port, wrd[1]);
#endif
        break;

    case INITOPT_LINK_PORT:
        /* link */
        if (strlen(wrd[1]) >= MAXSERV) { /* XXX: Use NI_MAXSERV */
            fprintf(stderr, "Amnuts: %s has Illegal port number on line %d.\n",
                    *initopt, config_line);
            boot_exit(1);
        }
#ifdef NETLINKS
        strcpy(amsys->nlink_port, wrd[1]);
#endif
        break;

    case INITOPT_SYSTEM_LOGGING:
        val = onoff_check(wrd[1]);
        if (val == -1) {
            fprintf(stderr,
                    "Amnuts: %s must be ON or OFF on line %d.\n",
                    *initopt, config_line);
            boot_exit(1);
        }
        /* set the bits correctly */
        if (val) {
            amsys->logging = SYSLOG | REQLOG | NETLOG | ERRLOG;
        } else {
            amsys->logging = 0;
        }
        break;

    case INITOPT_MINLOGIN_LEVEL:
    {
        enum lvl_value lvl = get_level(wrd[1]);
        if (lvl == NUM_LEVELS && strcmp(wrd[1], "NONE")) {
            fprintf(stderr,
                    "Amnuts: %s has Unknown level specifier on line %d.\n",
                    *initopt, config_line);
            boot_exit(1);
        }
        amsys->minlogin_level = lvl;
    }
        break;

    case INITOPT_MESG_LIFE:
        /* message lifetime */
        if (val < 1) {
            fprintf(stderr, "Amnuts: %s has Illegal value on line %d.\n",
                    *initopt, config_line);
            boot_exit(1);
        }
        amsys->mesg_life = val;
        break;

    case INITOPT_WIZPORT_LEVEL:
        /* wizport_level */
    {
        enum lvl_value lvl = get_level(wrd[1]);
        if (lvl == NUM_LEVELS) {
            fprintf(stderr,
                    "Amnuts: %s has Unknown level specifier on line %d.\n",
                    *initopt, config_line);
            boot_exit(1);
        }
#ifdef WIZPORT
        amsys->wizport_level = lvl;
#endif
    }
        break;

    case INITOPT_PROMPT_DEF:
        /* prompt defaults */
        val = onoff_check(wrd[1]);
        if (val == -1) {
            fprintf(stderr, "Amnuts: %s must be ON or OFF on line %d.\n",
                    *initopt, config_line);
            boot_exit(1);
        }
        amsys->prompt_def = val;
        break;

    case INITOPT_GATECRASH_LEVEL:
        /* gatecrash level */
    {
        enum lvl_value lvl = get_level(wrd[1]);
        if (lvl == NUM_LEVELS) {
            fprintf(stderr,
                    "Amnuts: %s has Unknown level specifier on line %d.\n",
                    *initopt, config_line);
            boot_exit(1);
        }
        amsys->gatecrash_level = lvl;
    }
        break;

    case INITOPT_MIN_PRIVATE:
        if (val < 1) {
            fprintf(stderr,
                    "Amnuts: %s has Number too low on line %d.\n",
                    *initopt, config_line);
            boot_exit(1);
        }
        amsys->min_private_users = val;
        break;

    case INITOPT_IGNORE_MP_LEVEL:
    {
        enum lvl_value lvl = get_level(wrd[1]);
        if (lvl == NUM_LEVELS) {
            fprintf(stderr,
                    "Amnuts: %s has Unknown level specifier on line %d.\n",
                    *initopt, config_line);
            boot_exit(1);
        }
        amsys->ignore_mp_level = lvl;
    }
        break;

    case INITOPT_REM_USER_MAXLEVEL:
        /*
           Max level a remote user can remotely log in if he does not have a local
           account. ie if level set to WIZ a GOD can only be a WIZ if logging in
           from another server unless he has a local account of level GOD
         */
    {
        enum lvl_value lvl = get_level(wrd[1]);
        if (lvl == NUM_LEVELS) {
            fprintf(stderr,
                    "Amnuts: %s has Unknown level specifier on line %d.\n",
                    *initopt, config_line);
            boot_exit(1);
        }
#ifdef NETLINKS
        amsys->rem_user_maxlevel = lvl;
#endif
    }
        break;

    case INITOPT_REM_USER_DEFLEVEL:
        /*
           Default level of remote user who does not have an account on site and
           connection is from a server of version 3.3.0 or lower.
         */
    {
        enum lvl_value lvl = get_level(wrd[1]);
        if (lvl == NUM_LEVELS) {
            fprintf(stderr,
                    "Amnuts: %s has Unknown level specifier on line %d.\n",
                    *initopt, config_line);
            boot_exit(1);
        }
#ifdef NETLINKS
        amsys->rem_user_deflevel = lvl;
#endif
    }
        break;

    case INITOPT_VERIFICATION:
        if (strlen(wrd[1]) > VERIFY_LEN) {
            fprintf(stderr, "Amnuts: %s too long on line %d.\n",
                    *initopt, config_line);
            boot_exit(1);
        }
#ifdef NETLINKS
        strcpy(amsys->verification, wrd[1]);
#endif
        break;

    case INITOPT_MESG_CHECK_TIME:
        /* mesg_check_time */
        if (wrd[1][2] != ':' || strlen(wrd[1]) > 5 || !isdigit((int) wrd[1][0])
                || !isdigit((int) wrd[1][1])
                || !isdigit((int) wrd[1][3])
                || !isdigit((int) wrd[1][4])) {
            fprintf(stderr, "Amnuts: %s has Invalid value on line %d.\n",
                    *initopt, config_line);
            boot_exit(1);
        }
        sscanf(wrd[1], "%d:%d", &amsys->mesg_check_hour, &amsys->mesg_check_min);
        if (amsys->mesg_check_hour > 23 || amsys->mesg_check_min > 59) {
            fprintf(stderr, "Amnuts: %s has Invalid value on line %d.\n",
                    *initopt, config_line);
            boot_exit(1);
        }
        amsys->mesg_check_done = time(0) + 86400 - 1;
        amsys->mesg_check_done -= amsys->mesg_check_done % 86400;
        amsys->mesg_check_done +=
                3600 * amsys->mesg_check_hour + 60 * amsys->mesg_check_min;
        break;

    case INITOPT_MAX_USERS:
        if (val < 1) {
            fprintf(stderr, "Amnuts: %s has Invalid value on line %d.\n",
                    *initopt, config_line);
            boot_exit(1);
        }
        amsys->max_users = val;
        break;

    case INITOPT_HEARTBEAT:
        if (val < 1) {
            fprintf(stderr, "Amnuts: %s has Invalid value on line %d.\n",
                    *initopt, config_line);
            boot_exit(1);
        }
        amsys->heartbeat = val;
        break;

    case INITOPT_LOGIN_IDLE_TIME:
        if (val < 10) {
            fprintf(stderr,
                    "Amnuts: %s has Invalid value on line %d.\n",
                    *initopt, config_line);
            boot_exit(1);
        }
        amsys->login_idle_time = val;
        break;

    case INITOPT_USER_IDLE_TIME:
        if (val < 10) {
            fprintf(stderr,
                    "Amnuts: %s has Invalid value on line %d.\n",
                    *initopt, config_line);
            boot_exit(1);
        }
        amsys->user_idle_time = val;
        break;

    case INITOPT_PASSWORDECHO_DEF:
        val = onoff_check(wrd[1]);
        if (val == -1) {
            fprintf(stderr,
                    "Amnuts: %s must be ON or OFF on line %d.\n",
                    *initopt, config_line);
            boot_exit(1);
        }
        amsys->passwordecho_def = val;
        break;

    case INITOPT_IGNORE_SIGTERM:
        val = yn_check(wrd[1]);
        if (val == -1) {
            fprintf(stderr,
                    "Amnuts: %s must be YES or NO on line %d.\n",
                    *initopt, config_line);
            boot_exit(1);
        }
        amsys->ignore_sigterm = val;
        break;

    case INITOPT_AUTO_CONNECT:
        val = yn_check(wrd[1]);
        if (val == -1) {
            fprintf(stderr, "Amnuts: %s must be YES or NO on line %d.\n",
                    *initopt, config_line);
            boot_exit(1);
        }
#ifdef NETLINKS
        amsys->auto_connect = val;
#endif
        break;

    case INITOPT_MAX_CLONES:
        if (val < 0) {
            fprintf(stderr, "Amnuts: %s has Invalid value on line %d.\n",
                    *initopt, config_line);
            boot_exit(1);
        }
        amsys->max_clones = val;
        break;

    case INITOPT_BAN_SWEARING:
        val = minmax_check(wrd[1]);
        if (val == -1) {
            fprintf(stderr,
                    "Amnuts: %s must be OFF, MIN or MAX on line %d.\n",
                    *initopt, config_line);
            boot_exit(1);
        }
        amsys->ban_swearing = val;
        break;

    case INITOPT_CRASH_ACTION:
        if (!strcmp(wrd[1], "NONE")) {
            amsys->crash_action = 0;
        } else if (!strcmp(wrd[1], "SHUTDOWN")) {
            amsys->crash_action = 1;
        } else if (!strcmp(wrd[1], "REBOOT")) {
            amsys->crash_action = 2;
        } else if (!strcmp(wrd[1], "SEAMLESS")) {
            amsys->crash_action = 3;
        } else {
            fprintf(stderr,
                    "Amnuts: %s must be NONE, SHUTDOWN, REBOOT or SEAMLESS on line %d.\n",
                    *initopt, config_line);
            boot_exit(1);
        }
        break;

    case INITOPT_COLOUR_DEF:
        val = onoff_check(wrd[1]);
        if (val == -1) {
            fprintf(stderr, "Amnuts: %s must be ON or OFF on line %d.\n",
                    *initopt, config_line);
            boot_exit(1);
        }
        amsys->colour_def = val;
        break;

    case INITOPT_TIME_OUT_AFKS:
        val = yn_check(wrd[1]);
        if (val == -1) {
            fprintf(stderr, "Amnuts: %s must be YES or NO on line %d.\n",
                    *initopt, config_line);
            boot_exit(1);
        }
        amsys->time_out_afks = val;
        break;

    case INITOPT_CHARECHO_DEF:
        val = onoff_check(wrd[1]);
        if (val == -1) {
            fprintf(stderr, "Amnuts: %s must be ON or OFF on line %d.\n",
                    *initopt, config_line);
            boot_exit(1);
        }
        amsys->charecho_def = val;
        break;

    case INITOPT_TIME_OUT_MAXLEVEL:
    {
        enum lvl_value lvl = get_level(wrd[1]);
        if (lvl == NUM_LEVELS) {
            fprintf(stderr,
                    "Amnuts: %s has Unknown level specifier on line %d.\n",
                    *initopt, config_line);
            boot_exit(1);
        }
        amsys->time_out_maxlevel = lvl;
    }
        break;

    case INITOPT_AUTO_PURGE:
        /* auto purge on boot up */
        val = yn_check(wrd[1]);
        if (val == -1) {
            fprintf(stderr, "Amnuts: %s must be YES or NO on line %d.\n",
                    *initopt, config_line);
            boot_exit(1);
        }
        if (!val) {
            amsys->auto_purge_date = -1;
        } else {
            /* XXX: Fixed to the next midnight */
            amsys->auto_purge_date = time(0) + 86400 - 1;
            amsys->auto_purge_date -= amsys->auto_purge_date % 86400;
        }
        break;

    case INITOPT_ALLOW_RECAPS:
        /* allow recapping of names */
        val = yn_check(wrd[1]);
        if (val == -1) {
            fprintf(stderr, "Amnuts: %s must be YES or NO on line %d.\n",
                    *initopt, config_line);
            boot_exit(1);
        }
        amsys->allow_recaps = val;
        break;

    case INITOPT_AUTO_PROMOTE:
        /* define whether auto promotes are on or off */
        val = yn_check(wrd[1]);
        if (val == -1) {
            fprintf(stderr, "Amnuts: %s must be YES or NO on line %d.\n",
                    *initopt, config_line);
            boot_exit(1);
        }
        amsys->auto_promote = val;
        break;

    case INITOPT_PERSONAL_ROOMS:
        val = yn_check(wrd[1]);
        if (val == -1) {
            fprintf(stderr,
                    "Amnuts: %s must be YES or NO on line %d.\n",
                    *initopt, config_line);
            boot_exit(1);
        }
        amsys->personal_rooms = val;
        break;

    case INITOPT_RANDOM_MOTDS:
        val = yn_check(wrd[1]);
        if (val == -1) {
            fprintf(stderr, "Amnuts: %s must be YES or NO on line %d.\n",
                    *initopt, config_line);
            boot_exit(1);
        }
        amsys->random_motds = val;
        break;

    case INITOPT_STARTUP_ROOM_PARSE:
        val = yn_check(wrd[1]);
        if (val == -1) {
            fprintf(stderr,
                    "Amnuts: %s must be YES or NO on line %d.\n",
                    *initopt, config_line);
            boot_exit(1);
        }
        amsys->startup_room_parse = val;
        break;

    case INITOPT_RESOLVE_IP:
        val = resolve_check(wrd[1]);
        if (val == -1) {
            fprintf(stderr,
                    "Amnuts: %s must be OFF, AUTO, MANUAL, or IDENTD on line %d.\n",
                    *initopt, config_line);
            boot_exit(1);
        }
        amsys->resolve_ip = val;
        break;

    case INITOPT_FLOOD_PROTECT:
        /* turns flood protection and auto-baning on and off */
        val = onoff_check(wrd[1]);
        if (val == -1) {
            fprintf(stderr, "Amnuts: %s must be ON or OFF on line %d.\n",
                    *initopt, config_line);
            boot_exit(1);
        }
        amsys->flood_protect = val;
        break;

    case INITOPT_BOOT_OFF_MIN:
        val = yn_check(wrd[1]);
        if (val == -1) {
            fprintf(stderr, "Amnuts: %s must be YES or NO on line %d.\n",
                    *initopt, config_line);
            boot_exit(1);
        }
        amsys->boot_off_min = val;
        break;

    case INITOPT_DEF_WARP:
        if (strlen(wrd[1]) >= ROOM_NAME_LEN + 1) {
            fprintf(stderr, "Amnuts: %s has Illegal room name on line %d.\n",
                    *initopt, config_line);
            boot_exit(1);
        }
        strcpy(amsys->default_warp, wrd[1]);
        break;

    case INITOPT_DEF_JAIL:
        if (strlen(wrd[1]) >= ROOM_NAME_LEN + 1) {
            fprintf(stderr, "Amnuts: %s has Illegal room name on line %d.\n",
                    *initopt, config_line);
            boot_exit(1);
        }
        strcpy(amsys->default_jail, wrd[1]);
        break;

    case INITOPT_DEF_BANK:
        if (strlen(wrd[1]) >= ROOM_NAME_LEN + 1) {
            fprintf(stderr, "Amnuts: %s has Illegal room name on line %d.\n",
                    *initopt, config_line);
            boot_exit(1);
        }
#ifdef GAMES
        strcpy(amsys->default_bank, wrd[1]);
#endif
        break;

    case INITOPT_DEF_SHOOT:
        if (strlen(wrd[1]) >= ROOM_NAME_LEN + 1) {
            fprintf(stderr, "Amnuts: %s has Illegal room name on line %d.\n",
                    *initopt, config_line);
            boot_exit(1);
        }
#ifdef GAMES
        strcpy(amsys->default_shoot, wrd[1]);
#endif
        break;

    default:
        fprintf(stderr, "Amnuts: %s is Unknown INIT option on line %d.\n",
                *initopt, config_line);
        boot_exit(1);
        break;

    }
}

/*
 * Parse rooms section
 */
void
parse_rooms_section(void)
{
    RM_OBJECT room;
    char *ptr1, *ptr2, c;
    int i;

    if (!*wrd[3]) {
        fprintf(stderr, "Amnuts: Required parameter(s) missing on line %d.\n",
                config_line);
        boot_exit(1);
    }
    if (strlen(wrd[0]) > ROOM_NAME_LEN) {
        fprintf(stderr, "Amnuts: Room map name too long on line %d.\n",
                config_line);
        boot_exit(1);
    }
    if (strlen(wrd[1]) > ROOM_LABEL_LEN) {
        fprintf(stderr, "Amnuts: Room label too long on line %d.\n", config_line);
        boot_exit(1);
    }
    if (strlen(wrd[2]) > ROOM_NAME_LEN) {
        fprintf(stderr, "Amnuts: Room name too long on line %d.\n", config_line);
        boot_exit(1);
    }
    /* Check for duplicate label or name */
    for (room = room_first; room; room = room->next) {
        if (!strcmp(room->label, wrd[1])) {
            fprintf(stderr, "Amnuts: Duplicate room label on line %d.\n",
                    config_line);
            boot_exit(1);
        }
        if (!strcmp(room->name, wrd[2])) {
            fprintf(stderr, "Amnuts: Duplicate room name on line %d.\n",
                    config_line);
            boot_exit(1);
        }
    }
    room = create_room();
    strcpy(room->map, wrd[0]);
    strcpy(room->label, wrd[1]);
    strcpy(room->name, wrd[2]);
    strcpy(room->show_name, room->name);
    /* Parse internal links bit ie hl,gd,of etc. MUST NOT be any spaces between the commas */
    i = 0;
    ptr1 = wrd[3];
    ptr2 = wrd[3];
    for (;;) {
        while (*ptr2 != ',' && *ptr2) {
            ++ptr2;
        }
        if (*ptr2 == ',' && !ptr2[1]) {
            fprintf(stderr, "Amnuts: Missing link label on line %d.\n",
                    config_line);
            boot_exit(1);
        }
        c = *ptr2;
        *ptr2 = '\0';
        if (!strcmp(ptr1, room->label)) {
            fprintf(stderr, "Amnuts: Room has a link to itself on line %d.\n",
                    config_line);
            boot_exit(1);
        }
        strcpy(room->link_label[i], ptr1);
        if (!c) {
            break;
        }
        if (++i >= MAX_LINKS) {
            fprintf(stderr, "Amnuts: Too many links on line %d.\n", config_line);
            boot_exit(1);
        }
        *ptr2 = c;
        ptr1 = ++ptr2;
    }
    /* Parse access privs */
    if (!*wrd[4]) {
        room->access = 0;
        return;
    }
    if (!strcmp(wrd[4], "BOTH")) {
        room->access = 0;
    } else if (!strcmp(wrd[4], "PUB")) {
        room->access = FIXED;
    } else if (!strcmp(wrd[4], "PRIV")) {
        room->access = FIXED | PRIVATE;
    } else {
        fprintf(stderr, "Amnuts: Unknown room access type on line %d.\n",
                config_line);
        boot_exit(1);
    }
    /* Parse external link stuff */
#ifdef NETLINKS
    if (!*wrd[5]) {
        return;
    }
    if (!strcmp(wrd[5], "ACCEPT")) {
        if (*wrd[6]) {
            fprintf(stderr,
                    "Amnuts: Unexpected word following ACCEPT keyword on line %d.\n",
                    config_line);
            boot_exit(1);
        }
        room->inlink = 1;
        return;
    }
    if (!strcmp(wrd[5], "CONNECT")) {
        if (!*wrd[6]) {
            fprintf(stderr, "Amnuts: External link name missing on line %d.\n",
                    config_line);
            boot_exit(1);
        }
        strcpy(room->netlink_name, wrd[6]);
        if (*wrd[7]) {
            fprintf(stderr,
                    "Amnuts: Unexpected word following external link name on line %d.\n",
                    config_line);
            boot_exit(1);
        }
        return;
    }
    fprintf(stderr, "Amnuts: Unknown connection option on line %d.\n",
            config_line);
    boot_exit(1);
#endif
}

/*
 * Parse rooms desc (topic) section
 */
void
parse_topics_section(char *topic)
{
    RM_OBJECT room;

    if (!*wrd[1]) {
        fprintf(stderr, "Amnuts: Required parameter(s) missing on line %d.\n",
                config_line);
        boot_exit(1);
    }
    /* Check to see if room exists */
    for (room = room_first; room; room = room->next)
        if (!strcmp(room->name, wrd[0])) {
            break;
        }
    if (!room) {
        fprintf(stderr, "Amnuts: Room does not exist on line %d.\n", config_line);
        boot_exit(1);
        return;
    }
    if (topic[strlen(topic) - 1] == '\n') {
        topic[strlen(topic) - 1] = '\0';
    }
    *room->topic = '\0';
    strncat(room->topic, topic, TOPIC_LEN);
}


#ifdef NETLINKS

/*
 * Parse sites section
 */
void
parse_sites_section(void)
{
    NL_OBJECT nl;

    if (!*wrd[3]) {
        fprintf(stderr, "Amnuts: Required parameter(s) missing on line %d.\n",
                config_line);
        boot_exit(1);
    }
    nl = create_netlink();
    if (!nl) {
        fprintf(stderr,
                "Amnuts: Memory allocation failure creating netlink on line %d.\n",
                config_line);
        boot_exit(1);
        return;
    }
    if (strlen(wrd[0]) > SERV_NAME_LEN) {
        fprintf(stderr, "Amnuts: Link name length too long on line %d.\n",
                config_line);
        boot_exit(1);
    }
    strcpy(nl->service, wrd[0]);
    if (strlen(wrd[1]) >= MAXHOST) { /* XXX: Use NI_MAXHOST */
        fprintf(stderr, "Amnuts: Site name length too long on line %d.\n",
                config_line);
        boot_exit(1);
    }
    strcpy(nl->site, wrd[1]);
    strtolower(nl->site);
    if (strlen(wrd[2]) >= MAXSERV) { /* XXX: USE NI_MAXSERV */
        fprintf(stderr, "Amnuts: Illegal port number on line %d.\n", config_line);
        boot_exit(1);
    }
    strcpy(nl->port, wrd[2]);
    if (strlen(wrd[3]) > VERIFY_LEN) {
        fprintf(stderr, "Amnuts: Verification too long on line %d.\n",
                config_line);
        boot_exit(1);
    }
    strcpy(nl->verification, wrd[3]);
    if (!*wrd[4] || !strcmp(wrd[4], "ALL")) {
        nl->allow = ALL;
    } else if (!strcmp(wrd[4], "IN")) {
        nl->allow = IN;
    } else if (!strcmp(wrd[4], "OUT")) {
        nl->allow = OUT;
    } else {
        fprintf(stderr, "Amnuts: Unknown netlink access type on line %d.\n",
                config_line);
        boot_exit(1);
    }
}
#endif



/******************************************************************************
 Signal handlers and exit functions
 *****************************************************************************/

/*
 * Initialise the signal traps, etc.
 */
void
init_signals(void)
{
    /*
     * NOTE: Neither of these should be needed but...
     * sysv_signal() => SA_RESETHAND (which in turn => SA_NODEFER)
     * bsd_signal() => SA_RESTART
     */
    struct sigaction sa;

    memset(&sa, 0, (sizeof sa));
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = SIG_DFL;
    sigaction(SIGABRT, &sa, NULL);
    sa.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &sa, NULL);
    sigaction(SIGHUP, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);
    sa.sa_handler = sig_handler;
    sigaction(SIGTERM, &sa, NULL);
#ifndef DUMPCORE
    sigaction(SIGILL, &sa, NULL);
    sigaction(SIGFPE, &sa, NULL);
    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGBUS, &sa, NULL);
#endif
}

/*
 * Talker signal handler function. Can either ignore, shutdown, reboot or
 * seamless reboot.
 *
 * If a unix error occurs though and we ignore it we are living on borrowed
 * time as usually it will crash completely after a while anyway.
 */
void
sig_handler(int sig)
{
    int saveerrno = errno;
    time_t tm;

    force_listen = 1;
    dump_commands(sig);
    switch (sig) {
    case SIGTERM:
        if (amsys->ignore_sigterm) {
            write_syslog(SYSLOG, 1, "SIGTERM signal received - ignoring.\n");
            errno = saveerrno;
            return;
        }
        write_room(NULL,
                "\n\n~OLSYSTEM:~FR~LI ALERT - termination signal received, initiating shutdown!\n\n");
        talker_shutdown(NULL, "a termination signal (SIGTERM)", 0);
        break; /* do not really need this here, but better safe... */
    case SIGILL:
        /*
           crash less than heartbeat seconds after boot?  must be the boot
           process causing the crash...cannot recover so just abort
         */
        time(&tm);
        if ((int) tm - (int) amsys->boot_time <= amsys->heartbeat) {
            write_room(NULL,
                    "\n\n\07~OLSYSTEM:~FR~LI PANIC - Illegal instruction recurring, initiating abort!\n\n");
            write_syslog(SYSLOG, 1,
                    "[ALERT] %d second illegal instruction...aborted!\n",
                    (int) tm - (int) amsys->boot_time);
            abort();
        }
        switch (amsys->crash_action) {
        case 0:
            write_room(NULL,
                    "\n\n\07~OLSYSTEM:~FR~LI WARNING - An illegal instruction has just occured!\n\n");
            write_syslog(SYSLOG, 1, "WARNING: An illegal instruction occured!\n");
            abort();
            break;
        case 1:
            write_room(NULL,
                    "\n\n\07~OLSYSTEM:~FR~LI PANIC - Illegal instruction, initiating shutdown!\n\n");
            talker_shutdown(NULL, "an illegal instruction (SIGILL)", 0);
            break;
        case 2:
            write_room(NULL,
                    "\n\n\07~OLSYSTEM:~FR~LI PANIC - Illegal instruction, initiating reboot!\n\n");
            talker_shutdown(NULL, "an illegal instruction (SIGILL)", 1);
            break;
        case 3:
            write_level(WIZ, 1, NORECORD,
                    "\n\n\07~OLSYSTEM:~FR~LI PANIC - Illegal instruction, initiating seamless reboot!",
                    NULL);
            talker_shutdown(NULL, "an illegal instruction (SIGILL)", 2);
            break;
        }
        break; /* do not really need this here, but better safe... */
    case SIGFPE:
        /*
           crash less than heartbeat seconds after boot?  must be the boot
           process causing the crash...cannot recover so just abort
         */
        time(&tm);
        if ((int) tm - (int) amsys->boot_time <= amsys->heartbeat) {
            write_room(NULL,
                    "\n\n\07~OLSYSTEM:~FR~LI PANIC - Arithmetic exception recurring, initiating abort!\n\n");
            write_syslog(SYSLOG, 1,
                    "[ALERT] %d second recurring arithmetic exception...aborted!\n",
                    (int) tm - (int) amsys->boot_time);
            abort();
        }
        switch (amsys->crash_action) {
        case 0:
            write_room(NULL,
                    "\n\n\07~OLSYSTEM:~FR~LI WARNING - An arithmetic exception has just occured!\n\n");
            write_syslog(SYSLOG, 1, "WARNING: An arithmetic exception occured!\n");
            abort();
            break;
        case 1:
            write_room(NULL,
                    "\n\n\07~OLSYSTEM:~FR~LI PANIC - Arithmetic exception, initiating shutdown!\n\n");
            talker_shutdown(NULL, "an arithmetic exception (SIGFPE)", 0);
            break;
        case 2:
            write_room(NULL,
                    "\n\n\07~OLSYSTEM:~FR~LI PANIC - Arithmetic exception, initiating reboot!\n\n");
            talker_shutdown(NULL, "an arithmetic exception (SIGFPE)", 1);
            break;
        case 3:
            write_level(WIZ, 1, NORECORD,
                    "\n\n\07~OLSYSTEM:~FR~LI PANIC - Arithmetic exception, initiating seamless reboot!",
                    NULL);
            talker_shutdown(NULL, "an arithmetic exception (SIGFPE)", 2);
            break;
        }
        break; /* do not really need this here, but better safe... */
    case SIGSEGV:
        /*
           crash less than heartbeat seconds after boot?  must be the boot
           process causing the crash...cannot recover so just abort
         */
        time(&tm);
        if ((int) tm - (int) amsys->boot_time <= amsys->heartbeat) {
            write_room(NULL,
                    "\n\n\07~OLSYSTEM:~FR~LI PANIC - Segmentation fault recurring, initiating abort!\n\n");
            write_syslog(SYSLOG, 1,
                    "[ALERT] %d second recurring segmentation fault...aborted!\n",
                    (int) tm - (int) amsys->boot_time);
            abort();
        }
        switch (amsys->crash_action) {
        case 0:
            write_room(NULL,
                    "\n\n\07~OLSYSTEM:~FR~LI WARNING - A segmentation fault has just occured!\n\n");
            write_syslog(SYSLOG, 1, "WARNING: A segmentation fault occured!\n");
            abort();
            break;
        case 1:
            write_room(NULL,
                    "\n\n\07~OLSYSTEM:~FR~LI PANIC - Segmentation fault, initiating shutdown!\n\n");
            talker_shutdown(NULL, "a segmentation fault (SIGSEGV)", 0);
            break;
        case 2:
            write_room(NULL,
                    "\n\n\07~OLSYSTEM:~FR~LI PANIC - Segmentation fault, initiating reboot!\n\n");
            talker_shutdown(NULL, "a segmentation fault (SIGSEGV)", 1);
            break;
        case 3:
            write_level(WIZ, 1, NORECORD,
                    "\n\n\07~OLSYSTEM:~FR~LI PANIC - Segmentation fault, initiating seamless reboot!",
                    NULL);
            talker_shutdown(NULL, "a segmentation fault (SIGSEGV)", 2);
            break;
        }
        break; /* do not really need this here, but better safe... */
    case SIGBUS:
        /*
           crash less than heartbeat seconds after boot?  must be the boot
           process causing the crash...cannot recover so just abort
         */
        time(&tm);
        if ((int) tm - (int) amsys->boot_time <= amsys->heartbeat) {
            write_room(NULL,
                    "\n\n\07~OLSYSTEM:~FR~LI PANIC - Bus error recurring, initiating abort!\n\n");
            write_syslog(SYSLOG, 1,
                    "[ALERT] %d second recurring bus error...aborted!\n",
                    (int) tm - (int) amsys->boot_time);
            abort();
        }
        switch (amsys->crash_action) {
        case 0:
            write_room(NULL,
                    "\n\n\07~OLSYSTEM:~FR~LI WARNING - A bus error has just occured!\n\n");
            write_syslog(SYSLOG, 1, "WARNING: A bus error occured!\n");
            abort();
            break;
        case 1:
            write_room(NULL,
                    "\n\n\07~OLSYSTEM:~FR~LI PANIC - Bus error, initiating shutdown!\n\n");
            talker_shutdown(NULL, "a bus error (SIGBUS)", 0);
            break;
        case 2:
            write_room(NULL,
                    "\n\n\07~OLSYSTEM:~FR~LI PANIC - Bus error, initiating reboot!\n\n");
            talker_shutdown(NULL, "a bus error (SIGBUS)", 1);
            break;
        case 3:
            write_level(WIZ, 1, NORECORD,
                    "\n\n\07~OLSYSTEM:~FR~LI PANIC - Bus error, initiating seamless reboot!",
                    NULL);
            talker_shutdown(NULL, "a bus error (SIGBUS)", 2);
            break;
        }
        break; /* do not really need this here, but better safe... */
    }
    errno = saveerrno;
}

/*
 * Exit because of error during bootup
 */
void
boot_exit(int code)
{
    switch (code) {
    case 1:
        write_syslog(SYSLOG, 1,
                "BOOT FAILURE: Error while parsing configuration file.\n");
        break;
    case 2:
        perror("Amnuts: Cannot open main port listen socket");
        write_syslog(SYSLOG, 1,
                "BOOT FAILURE: Cannot open main port listen socket.\n");
        break;
    case 3:
        perror("Amnuts: Cannot open wiz port listen socket");
        write_syslog(SYSLOG, 1,
                "BOOT FAILURE: Cannot open wiz port listen socket.\n");
        break;
    case 4:
        perror("Amnuts: Cannot open link port listen socket");
        write_syslog(SYSLOG, 1,
                "BOOT FAILURE: Cannot open link port listen socket.\n");
        break;
    case 5:
        perror("Amnuts: Cannot bind to main port");
        write_syslog(SYSLOG, 1, "BOOT FAILURE: Cannot bind to main port.\n");
        break;
    case 6:
        perror("Amnuts: Cannot bind to wiz port");
        write_syslog(SYSLOG, 1, "BOOT FAILURE: Cannot bind to wiz port.\n");
        break;
    case 7:
        perror("Amnuts: Cannot bind to link port");
        write_syslog(SYSLOG, 1, "BOOT FAILURE: Cannot bind to link port.\n");
        break;
    case 8:
        perror("Amnuts: Listen error on main port");
        write_syslog(SYSLOG, 1, "BOOT FAILURE: Listen error on main port.\n");
        break;
    case 9:
        perror("Amnuts: Listen error on wiz port");
        write_syslog(SYSLOG, 1, "BOOT FAILURE: Listen error on wiz port.\n");
        break;
    case 10:
        perror("Amnuts: Listen error on link port");
        write_syslog(SYSLOG, 1, "BOOT FAILURE: Listen error on link port.\n");
        break;
    case 11:
        perror("Amnuts: Failed to fork");
        write_syslog(SYSLOG, 1, "BOOT FAILURE: Failed to fork.\n");
        break;
    case 12:
        perror("Amnuts: Failed to parse user structure");
        write_syslog(SYSLOG, 1,
                "BOOT FAILURE: Failed to parse user structure.\n");
        break;
    case 13:
        perror("Amnuts: Failed to parse user commands");
        write_syslog(SYSLOG, 1, "BOOT FAILURE: Failed to parse user commands.\n");
        break;
    case 14:
        write_syslog(SYSLOG, 1, "BOOT FAILURE: Level names are not unique.\n");
        break;
    case 15:
        write_syslog(SYSLOG, 1,
                "BOOT FAILURE: Failed to read directory structure in USERFILES.\n");
        break;
    case 16:
        perror("Amnuts: Directory structure in USERFILES is incorrect");
        write_syslog(SYSLOG, 1,
                "BOOT FAILURE: Directory structure in USERFILES is incorrect.\n");
        break;
    case 17:
        perror("Amnuts: Failed to create temp user structure");
        write_syslog(SYSLOG, 1,
                "BOOT FAILURE: Failed to create temp user structure.\n");
        break;
    case 18:
        perror("Amnuts: Failed to parse a user structure");
        write_syslog(SYSLOG, 1,
                "BOOT FAILURE: Failed to parse a user structure.\n");
        break;
    case 19:
        perror("Amnuts: Failed to open directory USERROOMS for reading");
        write_syslog(SYSLOG, 1,
                "BOOT FAILURE: Failed to open directory USERROOMS.\n");
        break;
    case 20:
        perror("Amnuts: Failed to open directory in MOTDFILES for reading");
        write_syslog(SYSLOG, 1,
                "BOOT FAILURE: Failed to open a directory MOTDFILES.\n");
        break;
    case 21:
        perror("Amnuts: Failed to create system object");
        write_syslog(SYSLOG, 1,
                "BOOT FAILURE: Failed to create system object.\n");
        break;
    }
    exit(code);
}



/******************************************************************************
 Event functions
 *****************************************************************************/

/*
 * See if timed reboot or shutdown is underway
 */
void
check_reboot_shutdown(void)
{
    static const char *const w[] = {"~FRShutdown", "~FYRebooting", "~FGSeamless Rebooting"};
    int secs;

    if (!amsys->rs_user) {
        return;
    }
    amsys->rs_countdown -= amsys->heartbeat;
    if (amsys->rs_countdown <= 0) {
        talker_shutdown(amsys->rs_user, NULL, amsys->rs_which);
    }

    /*
       Print countdown message every minute unless we have less than 1 minute
       to go when we print every 10 secs
     */
    secs = time(0) - amsys->rs_announce;

    if (amsys->rs_which < 2) {
        if (amsys->rs_countdown >= 60 && secs >= 60) {
            vwrite_room(NULL, "~OLSYSTEM: %s in %d minute%s, %d second%s.\n",
                    w[amsys->rs_which], amsys->rs_countdown / 60,
                    PLTEXT_S(amsys->rs_countdown / 60),
                    amsys->rs_countdown % 60,
                    PLTEXT_S(amsys->rs_countdown % 60));
            amsys->rs_announce = time(0);
        }
        if (amsys->rs_countdown < 60 && secs >= 10) {
            vwrite_room(NULL, "~OLSYSTEM: %s in %d second%s.\n", w[amsys->rs_which],
                    amsys->rs_countdown, PLTEXT_S(amsys->rs_countdown));
            amsys->rs_announce = time(0);
        }
    } else {
        if (amsys->rs_countdown >= 60 && secs >= 60) {
            vwrite_level(WIZ, 1, NORECORD, NULL,
                    "~OLSYSTEM: %s in %d minute%s, %d second%s.\n",
                    w[amsys->rs_which], amsys->rs_countdown / 60,
                    PLTEXT_S(amsys->rs_countdown / 60),
                    amsys->rs_countdown % 60,
                    PLTEXT_S(amsys->rs_countdown % 60));
            amsys->rs_announce = time(0);
        }
        if (amsys->rs_countdown < 60 && secs >= 10) {
            vwrite_level(WIZ, 1, NORECORD, NULL, "~OLSYSTEM: %s in %d second%s.\n",
                    w[amsys->rs_which], amsys->rs_countdown,
                    PLTEXT_S(amsys->rs_countdown));
            amsys->rs_announce = time(0);
        }
    }
}

/*
 * login_time_out is the length of time someone can idle at login,
 * user_idle_time is the length of time they can idle once logged in.
 * Also ups users total login time.
 */
void
check_idle_and_timeout(void)
{
    UR_OBJECT user, next;
    int tm;

    /*
       Save next pointer here for when user structure gets
       destructed, we may lose ->next link and crash the program
     */
    for (user = user_first; user; user = next) {
        next = user->next;
        if (user->type == CLONE_TYPE) {
            continue;
        }
        user->total_login += amsys->heartbeat;
        if (user->level > amsys->time_out_maxlevel) {
            continue;
        }
        tm = (int) (time(0) - user->last_input);
        if (user->login && tm >= amsys->login_idle_time) {
            write_user(user, "\n\n*** Time out ***\n\n");
            disconnect_user(user);
            continue;
        }
        if (user->warned) {
            if (tm < amsys->user_idle_time - 60) {
                user->warned = 0;
                continue;
            }
            if (tm >= amsys->user_idle_time) {
                write_user(user,
                        "\n\n\07~FR~OL~LI*** You have been timed out. ***\n\n");
                disconnect_user(user);
                continue;
            }
        }
        if ((!user->afk || (user->afk && amsys->time_out_afks))
                && !user->login && !user->warned && tm >= amsys->user_idle_time - 60) {
            write_user(user,
                    "\n\07~FY~OL~LI*** WARNING - Input within 1 minute or you will be disconnected. ***\n\n");
            user->warned = 1;
        }
    }
}

/*
 * Records when the user last logged on for use with the .last command
 */
void
record_last_login(const char *name)
{
    int i;

    for (i = LASTLOGON_NUM; i > 0; --i) {
        strcpy(last_login_info[i].name, last_login_info[i - 1].name);
        strcpy(last_login_info[i].time, last_login_info[i - 1].time);
        last_login_info[i].on = last_login_info[i - 1].on;
    }
    strcpy(last_login_info[0].name, name);
    strcpy(last_login_info[0].time, long_date(1));
    last_login_info[0].on = 1;
}

/*
 * Records when the user last logged out for use with the .last command
 */
void
record_last_logout(const char *name)
{
    int i;

    for (i = 0; i < LASTLOGON_NUM; ++i) {
        if (!strcmp(last_login_info[i].name, name)) {
            break;
        }
    }
    if (i < LASTLOGON_NUM) {
        last_login_info[i].on = 0;
    }
}



/******************************************************************************
 Initializing of globals and other stuff
 *****************************************************************************/

#define USERDB_LIST \
  ML_ENTRY((VERSION,       "version"     )) \
  ML_ENTRY((PASSWORD,      "password"    )) \
  ML_ENTRY((PROMOTE_DATE,  "promote_date")) \
  ML_ENTRY((TIMES,         "times"       )) \
  ML_ENTRY((LEVELS,        "levels"      )) \
  ML_ENTRY((GENERAL,       "general"     )) \
  ML_ENTRY((USER_SET,      "user_set"    )) \
  ML_ENTRY((USER_IGNORES,  "user_ignores")) \
  ML_ENTRY((FIGHTING,      "fighting"    )) \
  ML_ENTRY((PURGING,       "purging"     )) \
  ML_ENTRY((LAST_SITE,     "last_site"   )) \
  ML_ENTRY((MAIL_VERIFY,   "mail_verify" )) \
  ML_ENTRY((DESCRIPTION,   "description" )) \
  ML_ENTRY((IN_PHRASE,     "in_phrase"   )) \
  ML_ENTRY((OUT_PHRASE,    "out_phrase"  )) \
  ML_ENTRY((EMAIL,         "email"       )) \
  ML_ENTRY((HOMEPAGE,      "homepage"    )) \
  ML_ENTRY((RECAP_NAME,    "recap_name"  )) \
  ML_ENTRY((GAME_SETTINGS, "games"       )) \
  ML_ENTRY((COUNT,         NULL          ))

enum userdb_value {
#define ML_EXPAND(value,name) USERDB_ ## value,
    USERDB_LIST
#undef ML_EXPAND
};
static const char *const userfile_options[] = {
#define ML_EXPAND(value,name) name,
    USERDB_LIST
#undef ML_EXPAND
};

/*
 * Load the users details
 */
int
load_user_details(UR_OBJECT user)
{
    const char *const *option;
    char filename[80];
    char line[ARR_SIZE], *s;
    char user_words[UFILE_WORDS][UFILE_WORD_LEN];
    const char *str;
    FILE *fp;
    int wcnt, wpos, damaged, version_found;

    sprintf(filename, "%s/%s.D", USERFILES, user->name);
    fp = fopen(filename, "r");
    if (!fp) {
        return 0;
    }
    damaged = version_found = 0;
    for (s = fgets(line, ARR_SIZE - 1, fp); s;
            s = fgets(line, ARR_SIZE - 1, fp)) {
        s[strlen(s) - 1] = '\0';
        /*
           make this into the functions own word array.  This allows this array to
           have a different length from the general words array.
         */
        str = s;
        for (wcnt = 0; wcnt < UFILE_WORDS; ++wcnt) {
            for (; *str; ++str) {
                if (!isspace((int) *str)) {
                    break;
                }
            }
            if (!*str) {
                break;
            }
            for (wpos = 0; wpos < UFILE_WORD_LEN; ++wpos) {
                if (!*str || isspace(*str)) {
                    break;
                }
                user_words[wcnt][wpos] = *str++;
            }
            user_words[wcnt][wpos] = '\0';
            for (; *str; ++str) {
                if (isspace(*str)) {
                    break;
                }
            }
        }
        /* now get the option we are on */
        for (option = userfile_options; *option; ++option) {
            if (!strcmp(*option, user_words[0])) {
                break;
            }
        }
        if (!*option) {
            ++damaged;
            continue;
        }
        switch ((enum userdb_value) (option - userfile_options)) {
        case USERDB_VERSION:
            if (wcnt >= 2) {
                /* make sure more than just option string was there */
                strcpy(user->version, remove_first(s));
                version_found = 1; /* gotta compensate still for old versions */
            }
            break;
        case USERDB_PASSWORD:
            if (wcnt >= 2) {
                strcpy(user->pass, remove_first(s));
            }
            break;
        case USERDB_PROMOTE_DATE:
            if (wcnt >= 2) {
                strcpy(user->date, remove_first(s));
            }
            break;
        case USERDB_TIMES:
            switch (wcnt) {
            default:
            case 4:
                user->read_mail = (time_t) atoi(user_words[4]);
                __attribute__ ((fallthrough));
            case 3:
                user->last_login_len = atoi(user_words[3]);
                __attribute__ ((fallthrough));
            case 2:
                user->total_login = (time_t) atoi(user_words[2]);
                __attribute__ ((fallthrough));
            case 1:
                user->last_login = (time_t) atoi(user_words[1]);
                __attribute__ ((fallthrough));
            case 0:
                break;
            }
            break;
        case USERDB_LEVELS:
            switch (wcnt) {
            default:
            case 5:
                user->retired = atoi(user_words[5]);
                __attribute__ ((fallthrough));
            case 4:
                user->muzzled = (enum lvl_value) atoi(user_words[4]);
                __attribute__ ((fallthrough));
            case 3:
                user->arrestby = (enum lvl_value) atoi(user_words[3]);
                __attribute__ ((fallthrough));
            case 2:
                user->unarrest = (enum lvl_value) atoi(user_words[2]);
                __attribute__ ((fallthrough));
            case 1:
                user->level = (enum lvl_value) atoi(user_words[1]);
                __attribute__ ((fallthrough));
            case 0:
                break;
            }
            break;
        case USERDB_GENERAL:
            switch (wcnt) {
            default:
            case 8:
                user->logons = atoi(user_words[8]);
                __attribute__ ((fallthrough));
            case 7:
                user->mail_verified = atoi(user_words[7]);
                __attribute__ ((fallthrough));
            case 6:
                user->monitor = atoi(user_words[6]);
                __attribute__ ((fallthrough));
            case 5:
                user->vis = atoi(user_words[5]);
                __attribute__ ((fallthrough));
            case 4:
                user->prompt = atoi(user_words[4]);
                __attribute__ ((fallthrough));
            case 3:
                user->command_mode = atoi(user_words[3]);
                __attribute__ ((fallthrough));
            case 2:
                user->charmode_echo = atoi(user_words[2]);
                __attribute__ ((fallthrough));
            case 1:
                user->accreq = atoi(user_words[1]);
                __attribute__ ((fallthrough));
            case 0:
                break;
            }
            break;
        case USERDB_USER_SET:
            switch (wcnt) {
            default:
            case 14:
                user->reverse_buffer = atoi(user_words[14]);
                __attribute__ ((fallthrough));
            case 13:
                *user->icq = '\0';
                if (strcmp(user_words[13], "#UNSET")) {
                    strcpy(user->icq, user_words[13]);
                }
                __attribute__ ((fallthrough));
            case 12:
                user->cmd_type = atoi(user_words[12]);
                __attribute__ ((fallthrough));
            case 11:
                user->show_rdesc = atoi(user_words[11]);
                __attribute__ ((fallthrough));
            case 10:
                user->show_pass = atoi(user_words[10]);
                __attribute__ ((fallthrough));
            case 9:
                user->autofwd = atoi(user_words[9]);
                __attribute__ ((fallthrough));
            case 8:
                user->alert = atoi(user_words[8]);
                __attribute__ ((fallthrough));
            case 7:
                user->lroom = atoi(user_words[7]);
                __attribute__ ((fallthrough));
            case 6:
                user->colour = atoi(user_words[6]);
                __attribute__ ((fallthrough));
            case 5:
                user->hideemail = atoi(user_words[5]);
                __attribute__ ((fallthrough));
            case 4:
                user->pager = atoi(user_words[4]);
                __attribute__ ((fallthrough));
            case 3:
                user->wrap = atoi(user_words[3]);
                __attribute__ ((fallthrough));
            case 2:
                user->age = atoi(user_words[2]);
                __attribute__ ((fallthrough));
            case 1:
                user->gender = atoi(user_words[1]);
                __attribute__ ((fallthrough));
            case 0:
                break;
            }
            break;
        case USERDB_USER_IGNORES:
            switch (wcnt) {
            default:
            case 8:
                user->ignbeeps = atoi(user_words[8]);
                __attribute__ ((fallthrough));
            case 7:
                user->igngreets = atoi(user_words[7]);
                __attribute__ ((fallthrough));
            case 6:
                user->ignwiz = atoi(user_words[6]);
                __attribute__ ((fallthrough));
            case 5:
                user->ignlogons = atoi(user_words[5]);
                __attribute__ ((fallthrough));
            case 4:
                user->ignpics = atoi(user_words[4]);
                __attribute__ ((fallthrough));
            case 3:
                user->ignshouts = atoi(user_words[3]);
                __attribute__ ((fallthrough));
            case 2:
                user->igntells = atoi(user_words[2]);
                __attribute__ ((fallthrough));
            case 1:
                user->ignall = atoi(user_words[1]);
                __attribute__ ((fallthrough));
            case 0:
                break;
            }
            break;
            /* XXX: For backwards compatibility only */
        case USERDB_FIGHTING:
            switch (wcnt) {
            default:
            case 6:
                user->hps = atoi(user_words[6]);
                __attribute__ ((fallthrough));
            case 5:
                user->bullets = atoi(user_words[5]);
                __attribute__ ((fallthrough));
            case 4:
                user->kills = atoi(user_words[4]);
                __attribute__ ((fallthrough));
            case 3:
                user->deaths = atoi(user_words[3]);
                __attribute__ ((fallthrough));
            case 2:
                user->misses = atoi(user_words[2]);
                __attribute__ ((fallthrough));
            case 1:
                user->hits = atoi(user_words[1]);
                __attribute__ ((fallthrough));
            case 0:
                break;
            }
            break;
        case USERDB_PURGING:
            switch (wcnt) {
            default:
            case 2:
                /* XXX: Was t_expire; now using last_login */
            case 1:
                user->expire = atoi(user_words[1]);
                __attribute__ ((fallthrough));
            case 0:
                break;
            }
            break;
        case USERDB_LAST_SITE:
            switch (wcnt) {
            default:
            case 2:
                strcpy(user->logout_room, user_words[2]);
                __attribute__ ((fallthrough));
            case 1:
                strcpy(user->last_site, user_words[1]);
                __attribute__ ((fallthrough));
            case 0:
                break;
            }
            break;
        case USERDB_MAIL_VERIFY:
            if (wcnt >= 2) {
                str = remove_first(s);
                *user->verify_code = '\0';
                /* Leave the three of them (backwards compatibility) */
                /* And keep there the conditions to 0: who said that anything != 0 is true? Maybe in your OS, but not in all of them... */
                if ((strcmp(str, "#UNSET") != 0) && (strcmp(str, "#NONE") != 0) && (strcmp(str, "#EMAILSET") != 0)) {
                    strcpy(user->verify_code, str);
                }
            }
            break;
        case USERDB_DESCRIPTION:
            if (wcnt >= 2) {
                strcpy(user->desc, remove_first(s));
            }
            break;
        case USERDB_IN_PHRASE:
            if (wcnt >= 2) {
                strcpy(user->in_phrase, remove_first(s));
            }
            break;
        case USERDB_OUT_PHRASE:
            if (wcnt >= 2) {
                strcpy(user->out_phrase, remove_first(s));
            }
            break;
        case USERDB_EMAIL:
            if (wcnt >= 2) {
                str = remove_first(s);
                *user->email = '\0';
                if (strcmp(str, "#UNSET")) {
                    strcpy(user->email, str);
                }
            }
            break;
        case USERDB_HOMEPAGE:
            if (wcnt >= 2) {
                str = remove_first(s);
                *user->homepage = '\0';
                if (strcmp(str, "#UNSET")) {
                    strcpy(user->homepage, str);
                }
            }
            break;
        case USERDB_RECAP_NAME:
            if (wcnt >= 2) {
                strcpy(user->recap, remove_first(s));
            }
            break;
        case USERDB_GAME_SETTINGS:
            switch (wcnt) {
            default:
            case 8:
                user->bank = atoi(user_words[8]);
                __attribute__ ((fallthrough));
            case 7:
                user->money = atoi(user_words[7]);
                __attribute__ ((fallthrough));
            case 6:
                user->hps = atoi(user_words[6]);
                __attribute__ ((fallthrough));
            case 5:
                user->bullets = atoi(user_words[5]);
                __attribute__ ((fallthrough));
            case 4:
                user->kills = atoi(user_words[4]);
                __attribute__ ((fallthrough));
            case 3:
                user->deaths = atoi(user_words[3]);
                __attribute__ ((fallthrough));
            case 2:
                user->misses = atoi(user_words[2]);
                __attribute__ ((fallthrough));
            case 1:
                user->hits = atoi(user_words[1]);
                __attribute__ ((fallthrough));
            case 0:
                break;
            }
            break;
        default:
            ++damaged;
            break;
        }

    }
    fclose(fp);
    if (!version_found) {
        load_user_details_old(user);
    } else if (damaged) {
        write_syslog(SYSLOG, 1, "DAMAGED userfile \"%s.D\"\n", user->name);
    }
    if (!amsys->allow_recaps || !*user->recap) {
        strcpy(user->recap, user->name);
    }
    strcpy(user->bw_recap, colour_com_strip(user->recap));
    user->real_level = user->level;
    /* compensate for new accreq change */
    if (user->level > NEW) {
        user->accreq = -1;
    }
    get_macros(user);
    get_xgcoms(user);
    read_user_reminders(user);
    load_flagged_users(user);
    return 1;
}

/*
 * Save the details for the user
 */
int
save_user_details(UR_OBJECT user, int save_current)
{
    char filename[80];
    FILE *fp;

    if (user->type == REMOTE_TYPE || user->type == CLONE_TYPE) {
        return 0;
    }
    sprintf(filename, "%s/%s.D", USERFILES, user->name);
    fp = fopen(filename, "w");
    if (!fp) {
        vwrite_user(user, "%s: failed to save your details.\n", syserror);
        write_syslog(SYSLOG, 1, "SAVE_USER_STATS: Failed to save %s.\n",
                user->name);
        return 0;
    }
    /* reset normal level */
    if (user->real_level < user->level) {
        user->level = user->real_level;
    }
    /* print out the file */
    fprintf(fp, "%-13.13s %s\n", userfile_options[USERDB_VERSION], USERVER); /* of user database */
    fprintf(fp, "%-13.13s %s\n", userfile_options[USERDB_PASSWORD], user->pass);
    fprintf(fp, "%-13.13s %s\n", userfile_options[USERDB_PROMOTE_DATE],
            user->date);
    if (save_current) {
        fprintf(fp, "%-13.13s %d %d %d %d\n", userfile_options[USERDB_TIMES],
                (int) time(0), (int) user->total_login,
                (int) (time(0) - user->last_login), (int) user->read_mail);
    } else {
        fprintf(fp, "%-13.13s %d %d %d %d\n", userfile_options[USERDB_TIMES],
                (int) user->last_login, (int) user->total_login,
                user->last_login_len, (int) user->read_mail);
    }
    fprintf(fp, "%-13.13s %d %d %d %d %d\n", userfile_options[USERDB_LEVELS],
            user->level, user->unarrest, user->arrestby, user->muzzled,
            user->retired);
    fprintf(fp, "%-13.13s %d %d %d %d %d %d %d %d\n",
            userfile_options[USERDB_GENERAL], user->accreq, user->charmode_echo,
            user->command_mode, user->prompt, user->vis, user->monitor,
            user->mail_verified, user->logons);
    fprintf(fp, "%-13.13s %d %d %d %d %d %d %d %d %d %d %d %d %s %d\n",
            userfile_options[USERDB_USER_SET], user->gender, user->age,
            user->wrap, user->pager, user->hideemail, user->colour, user->lroom,
            user->alert, user->autofwd, user->show_pass, user->show_rdesc,
            user->cmd_type, *user->icq ? user->icq : "#UNSET",
            user->reverse_buffer);
    fprintf(fp, "%-13.13s %d %d %d %d %d %d %d %d\n",
            userfile_options[USERDB_USER_IGNORES], user->ignall, user->igntells,
            user->ignshouts, user->ignpics, user->ignlogons, user->ignwiz,
            user->igngreets, user->ignbeeps);
    if (!save_current) {
        fprintf(fp, "%-13.13s %d %d\n", userfile_options[USERDB_PURGING],
                user->expire, -1);
    } else {
        if (user->level == NEW) {
            fprintf(fp, "%-13.13s %d %d\n", userfile_options[USERDB_PURGING],
                    user->expire, (int) (time(0) + (NEWBIE_EXPIRES * 86400)));
        } else {
            fprintf(fp, "%-13.13s %d %d\n", userfile_options[USERDB_PURGING],
                    user->expire, (int) (time(0) + (USER_EXPIRES * 86400)));
        }
    }
    if (save_current) {
        fprintf(fp, "%-13.13s %s %s\n", userfile_options[USERDB_LAST_SITE],
                user->site, user->room->name);
    } else {
        fprintf(fp, "%-13.13s %s %s\n", userfile_options[USERDB_LAST_SITE],
                user->last_site, user->logout_room);
    }
    fprintf(fp, "%-13.13s %s\n", userfile_options[USERDB_MAIL_VERIFY],
            *user->verify_code ? user->verify_code : "#UNSET");
    fprintf(fp, "%-13.13s %s\n", userfile_options[USERDB_DESCRIPTION],
            user->desc);
    fprintf(fp, "%-13.13s %s\n", userfile_options[USERDB_IN_PHRASE],
            user->in_phrase);
    fprintf(fp, "%-13.13s %s\n", userfile_options[USERDB_OUT_PHRASE],
            user->out_phrase);
    fprintf(fp, "%-13.13s %s\n", userfile_options[USERDB_EMAIL],
            *user->email ? user->email : "#UNSET");
    fprintf(fp, "%-13.13s %s\n", userfile_options[USERDB_HOMEPAGE],
            *user->homepage ? user->homepage : "#UNSET");
    fprintf(fp, "%-13.13s %s\n", userfile_options[USERDB_RECAP_NAME],
            user->recap);
    fprintf(fp, "%-13.13s %d %d %d %d %d %d %d %d\n",
            userfile_options[USERDB_GAME_SETTINGS], user->hits, user->misses,
            user->deaths, user->kills, user->bullets, user->hps, user->money,
            user->bank);
    fclose(fp);
    return 1;
}

/*
 * Load the users details
 */
int
load_user_details_old(UR_OBJECT user)
{
    char line[82], filename[80];
    FILE *fp;
    int temp1, temp2, temp3, temp4;

    sprintf(filename, "%s/%s.D", USERFILES, user->name);
    fp = fopen(filename, "r");
    if (!fp) {
        return 0;
    }

    /* Password */
    fscanf(fp, "%s\n", user->pass);
    /* version control of user structure */
    fgets(line, 82, fp);
    line[strlen(line) - 1] = '\0';
    strcpy(user->version, line);
    /* date when user last promoted/demoted */
    fgets(line, 82, fp);
    line[strlen(line) - 1] = '\0';
    strcpy(user->date, line);
    /* FIXME: use scanf "*" flag for discarding instead of redundant temps */
    /* times, levels, and important stats */
    fscanf(fp, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d", &temp1,
            &temp2, &user->last_login_len, &temp3, &temp1, &user->prompt, &temp2,
            &user->charmode_echo, &user->command_mode, &user->vis,
            &user->monitor, &temp4, &user->logons, &user->accreq,
            &user->mail_verified, &temp3);
    user->level = (enum lvl_value) temp1;
    user->muzzled = (enum lvl_value) temp2;
    user->arrestby = (enum lvl_value) temp3;
    user->unarrest = (enum lvl_value) temp4;
    /* stats set using the "set" function */
    fscanf(fp, "%d %d %d %d %d %d %d %d %d %d %d %d %s", &user->gender,
            &user->age, &user->wrap, &user->pager, &user->hideemail,
            &user->colour, &user->lroom, &user->alert, &user->autofwd,
            &user->show_pass, &user->show_rdesc, &user->cmd_type, user->icq);
    if (!strcmp(user->icq, "#UNSET")) {
        *user->icq = '\0';
    }
    /* ignore status */
    fscanf(fp, "%d %d %d %d %d %d %d %d", &user->ignall, &user->igntells,
            &user->ignshouts, &user->ignpics, &user->ignlogons, &user->ignwiz,
            &user->igngreets, &user->ignbeeps);
    /* Gun fight information */
    fscanf(fp, "%d %d %d %d %d %d", &user->hits, &user->misses, &user->deaths,
            &user->kills, &user->bullets, &user->hps);
    /* user expires and date */
    fscanf(fp, "%d %*d", &user->expire);
    /* site address and last room they were in */
    fscanf(fp, "%s %s %s\n", user->last_site, user->logout_room,
            user->verify_code);
    if (!strcmp(user->verify_code, "#UNSET")
            /* These two are for backwards compatibility */
            || !strcmp(user->verify_code, "#NONE")
            || !strcmp(user->verify_code, "#EMAILSET")) {
        *user->verify_code = '\0';
    }
    /* general text stuff
       Need to do the rest like this because they may be more than 1 word each
       possible for one colour for each letter, *3 for the code length */
    fgets(line, USER_DESC_LEN + USER_DESC_LEN * 3, fp);
    line[strlen(line) - 1] = '\0';
    strcpy(user->desc, line);
    fgets(line, PHRASE_LEN + PHRASE_LEN * 3, fp);
    line[strlen(line) - 1] = '\0';
    strcpy(user->in_phrase, line);
    fgets(line, PHRASE_LEN + PHRASE_LEN * 3, fp);
    line[strlen(line) - 1] = '\0';
    strcpy(user->out_phrase, line);
    fgets(line, 81, fp);
    line[strlen(line) - 1] = '\0';
    *user->email = '\0';
    if (strcmp(line, "#UNSET")) {
        strcpy(user->email, line);
    }
    fgets(line, 81, fp);
    line[strlen(line) - 1] = '\0';
    *user->homepage = '\0';
    if (strcmp(line, "#UNSET")) {
        strcpy(user->homepage, line);
    }
    fscanf(fp, "%s\n", user->recap);
    fclose(fp);

    if (strcmp(user->version, USERVER)) {
        /* As default, if the version cannot be read then it will load up version 1.4.2 files */
        if (!strcmp(user->version, "2.0.0")) {
            load_oldversion_user(user, 200);
            write_syslog(SYSLOG, 1,
                    "Reading old user file (version 2.0.0) in load_user_details()\n");
        } else if (!strcmp(user->version, "2.0.1")) {
            load_oldversion_user(user, 201);
            write_syslog(SYSLOG, 1,
                    "Reading old user file (version 2.0.1) in load_user_details()\n");
        } else if (!strcmp(user->version, "2.1.0")) {
            load_oldversion_user(user, 210);
            write_syslog(SYSLOG, 1,
                    "Reading old user file (version 2.1.0) in load_user_details()\n");
        } else if (!strcmp(user->version, "2.1.1")) {
            write_syslog(SYSLOG, 1,
                    "Reading old user file (version 2.1.1) in load_user_details()\n");
            /* do nothing.. userfile type not changed */
        } else {
            load_oldversion_user(user, 142);
            write_syslog(SYSLOG, 1,
                    "Reading old user file (version 1.4.2) in load_user_details()\n");
        }
        strcpy(user->version, USERVER);
        if (user->level > NEW) {
            user->accreq = -1;
        }
    } else {
        user->last_login = (time_t) temp1;
        user->total_login = (time_t) temp2;
        user->read_mail = (time_t) temp3;
    }
    if (!amsys->allow_recaps) {
        strcpy(user->recap, user->name);
    }
    strcpy(user->bw_recap, colour_com_strip(user->recap));
    user->real_level = user->level;
    /* compensate for new accreq change */
    if (user->level > NEW) {
        user->accreq = -1;
    }
    get_macros(user);
    get_xgcoms(user);
    read_user_reminders(user);
    load_flagged_users(user);
    return 1;
}

/*
 * Below is the loading for Amnuts version 1.4.2 and 2.x.x user structures
 */
int
load_oldversion_user(UR_OBJECT user, int version)
{
    char line[81], filename[80];
    FILE *fp;
    int temp1, temp2, temp3, temp4, oldvote;

    reset_user(user); /* make sure reads in fresh */
    sprintf(filename, "%s/%s.D", USERFILES, user->name);
    fp = fopen(filename, "r");
    if (!fp) {
        return 0;
    }

    /* Password */
    fscanf(fp, "%s\n", user->pass);
    /* read version control and date last promoted if version 2.0.0, 2.0.1, 2.1.0 */
    if (version >= 200) {
        fgets(line, 82, fp);
        line[strlen(line) - 1] = '\0';
        strcpy(user->version, line);
        fgets(line, 82, fp);
        line[strlen(line) - 1] = '\0';
        strcpy(user->date, line);
    } else {
        strcpy(user->version, "1.4.2");
        strcpy(user->date, long_date(1));
    }
    if (version >= 200) {
        /* FIXME: use scanf "*" flag for discarding instead of redundant temps */
        /* times, levels, and important stats */
        fscanf(fp, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d", &temp1, &temp2,
                &user->last_login_len, &temp3, &temp1, &user->prompt, &temp2,
                &user->charmode_echo, &user->command_mode, &user->vis,
                &user->monitor, &temp4, &user->logons, &user->accreq,
                &user->mail_verified);
    } else {
        /* FIXME: use scanf "*" flag for discarding instead of redundant temps */
        /* times, levels, and important stats */
        fscanf(fp, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d", &temp1,
                &temp2, &user->last_login_len, &temp3, &temp1, &user->prompt,
                &temp2, &user->charmode_echo, &user->command_mode, &user->vis,
                &user->monitor, &oldvote, &temp4, &user->logons,
                &user->accreq, &user->mail_verified);
    }
    user->level = (enum lvl_value) temp1;
    user->muzzled = (enum lvl_value) temp2;
    user->unarrest = (enum lvl_value) temp4;
    if (version >= 210) {
        /* stats set using the "set" function */
        fscanf(fp, "%d %d %d %d %d %d %d %d %d %d %d %d %s", &user->gender,
                &user->age, &user->wrap, &user->pager, &user->hideemail,
                &user->colour, &user->lroom, &user->alert, &user->autofwd,
                &user->show_pass, &user->show_rdesc, &user->cmd_type, user->icq);
        if (!strcmp(user->icq, "#UNSET")) {
            *user->icq = '\0';
        }
    } else if (version >= 200) {
        /* stats set using the "set" function */
        fscanf(fp, "%d %d %d %d %d %d %d %d %d %d %d %d", &user->gender,
                &user->age, &user->wrap, &user->pager, &user->hideemail,
                &user->colour, &user->lroom, &user->alert, &user->autofwd,
                &user->show_pass, &user->show_rdesc, &user->cmd_type);
    } else {
        /* stats set using the "set" function */
        fscanf(fp, "%d %d %d %d %d %d %d %d %d %d", &user->gender, &user->age,
                &user->wrap, &user->pager, &user->hideemail, &user->colour,
                &user->lroom, &user->alert, &user->autofwd, &user->show_pass);
        user->show_rdesc = 1;
        user->cmd_type = 0;
    }
    /* ignore status */
    fscanf(fp, "%d %d %d %d %d %d %d %d", &user->ignall, &user->igntells,
            &user->ignshouts, &user->ignpics, &user->ignlogons, &user->ignwiz,
            &user->igngreets, &user->ignbeeps);
    /* Gun fight information */
    fscanf(fp, "%d %d %d %d %d %d", &user->hits, &user->misses, &user->deaths,
            &user->kills, &user->bullets, &user->hps);
    /* user expires and date */
    fscanf(fp, "%d %*d", &user->expire);
    /* site address and last room they were in */
    fscanf(fp, "%s %s %s\n", user->last_site, user->logout_room,
            user->verify_code);
    if (!strcmp(user->verify_code, "#UNSET")
            /* These two are for backwards compatibility */
            || !strcmp(user->verify_code, "#NONE")
            || !strcmp(user->verify_code, "#EMAILSET")) {
        *user->verify_code = '\0';
    }
    /*
       general text stuff
       Need to do the rest like this because they may be more than 1 word each
       possible for one colour for each letter, *3 for the code length
     */
    fgets(line, USER_DESC_LEN + USER_DESC_LEN * 3, fp);
    line[strlen(line) - 1] = '\0';
    strcpy(user->desc, line);
    fgets(line, PHRASE_LEN + PHRASE_LEN * 3, fp);
    line[strlen(line) - 1] = '\0';
    strcpy(user->in_phrase, line);
    fgets(line, PHRASE_LEN + PHRASE_LEN * 3, fp);
    line[strlen(line) - 1] = '\0';
    strcpy(user->out_phrase, line);
    fgets(line, 82, fp);
    line[strlen(line) - 1] = '\0';
    *user->email = '\0';
    if (strcmp(line, "#UNSET")) {
        strcpy(user->email, line);
    }
    fgets(line, 82, fp);
    line[strlen(line) - 1] = '\0';
    *user->homepage = '\0';
    if (strcmp(line, "#UNSET")) {
        strcpy(user->homepage, line);
    }
    if (version >= 200) {
        fscanf(fp, "%s\n", user->recap);
    } else {
        strcpy(user->recap, user->name);
    }
    fclose(fp);
    user->last_login = (time_t) temp1;
    user->total_login = (time_t) temp2;
    user->read_mail = (time_t) temp3;
    return 1;
}

/*
 * Get all users from the user directories and add them to the user lists.
 * If verbose mode is on, then attempt to get date string as well
 */
void
process_users(void)
{
    char *s;
    DIR *dirp;
    struct dirent *dp;
    UR_OBJECT u;

    u = create_user();
    if (!u) {
        fprintf(stderr, "Amnuts: Create user failure in process_users().\n");
        boot_exit(17);
        return;
    }
    /* open the directory file up */
    dirp = opendir(USERFILES);
    if (!dirp) {
        fprintf(stderr, "Amnuts: Directory open failure in process_users().\n");
        destruct_user(u);
        boot_exit(12);
    }
    /* count up how many files in the directory - this include . and .. */
    for (dp = readdir(dirp); dp; dp = readdir(dirp)) {
        s = strchr(dp->d_name, '.');
        if (!s || strcmp(s, ".D")) {
            continue;
        }
        *u->name = '\0';
        strncat(u->name, dp->d_name, (size_t) (s - dp->d_name));
        if (!load_user_details(u)) {
            break;
        }
        add_user_node(u->name, u->level);
        if (u->retired) {
            add_retire_list(u->name);
        }
        reset_user(u);
    }
    closedir(dirp);
    if (dp) {
        fprintf(stderr,
                "Amnuts: Cannot load userfile for \"%s\" in process_users().\n",
                u->name);
        boot_exit(18);
    }
    destruct_user(u);
}

/*
 * Put commands in an ordered linked list for viewing with .help
 */
void
parse_commands(void)
{
    const struct cmd_entry *com_tab;

    for (com_tab = command_table; com_tab->name; ++com_tab) {
        if (!add_command((enum cmd_value) (com_tab - command_table))) {
            fprintf(stderr,
                    "Amnuts: Memory allocation failure in parse_commands().\n");
            boot_exit(13);
        }
    }
    return;
}



/******************************************************************************
 File functions - reading, writing, counting of lines, etc
 *****************************************************************************/

/*
 * wipes ALL the files belonging to the user with name given
 */
void
clean_files(char *name)
{
    char filename[80];

    *name = toupper(*name);
    sprintf(filename, "%s/%s.D", USERFILES, name);
    remove(filename);
    sprintf(filename, "%s/%s/%s.M", USERFILES, USERMAILS, name);
    remove(filename);
    sprintf(filename, "%s/%s/%s.P", USERFILES, USERPROFILES, name);
    remove(filename);
    sprintf(filename, "%s/%s/%s.H", USERFILES, USERHISTORYS, name);
    remove(filename);
    sprintf(filename, "%s/%s/%s.C", USERFILES, USERCOMMANDS, name);
    remove(filename);
    sprintf(filename, "%s/%s/%s.MAC", USERFILES, USERMACROS, name);
    remove(filename);
    sprintf(filename, "%s/%s/%s.R", USERFILES, USERROOMS, name);
    remove(filename);
    sprintf(filename, "%s/%s/%s.B", USERFILES, USERROOMS, name);
    remove(filename);
    sprintf(filename, "%s/%s/%s.REM", USERFILES, USERREMINDERS, name);
    remove(filename);
    sprintf(filename, "%s/%s/%s.U", USERFILES, USERFLAGGED, name);
    remove(filename);
}

/*
 * remove a line from the top or bottom of a file.
 * where=0 then remove from the top
 * where=1 then remove from the bottom
 */
int
remove_top_bottom(char *filename, int where)
{
    char line[ARR_SIZE], *s;
    FILE *fpi, *fpo;
    int i, cnt;

    fpi = fopen(filename, "r");
    if (!fpi) {
        return 0;
    }
    fpo = fopen("temp_file", "w");
    if (!fpo) {
        fclose(fpi);
        return 0;
    }
    if (!where) {
        /* remove from top */
        s = fgets(line, ARR_SIZE, fpi);
        /* get rest of file */
        for (s = fgets(line, ARR_SIZE, fpi); s;
                s = fgets(line, ARR_SIZE - 1, fpi)) {
            s[strlen(s) - 1] = '\0';
            fprintf(fpo, "%s\n", s);
        }
    } else {
        /* remove from bottom of file */
        cnt = count_lines(filename);
        --cnt;
        i = 0;
        for (s = fgets(line, ARR_SIZE - 1, fpi); s;
                s = fgets(line, ARR_SIZE - 1, fpi)) {
            if (i++ < cnt) {
                s[strlen(s) - 1] = '\0';
                fprintf(fpo, "%s\n", s);
            }
        }
    }
    fclose(fpi);
    fclose(fpo);
    rename("temp_file", filename);
    cnt = count_lines(filename);
    if (!cnt) {
        remove(filename);
    }
    return 1;
}

/*
 * counts how many lines are in a file
 */
int
count_lines(char *filename)
{
    FILE *fp;
    int i, c;

    i = 0;
    fp = fopen(filename, "r");
    if (!fp) {
        return i;
    }
    for (c = getc(fp); c != EOF; c = getc(fp)) {
        if (c == '\n') {
            ++i;
        }
    }
    fclose(fp);
    return i;
}



/******************************************************************************
 Write functions - users, rooms, system logs
 *****************************************************************************/

/*
 * Write a NULL terminated string to a socket
 */
void
write_sock(int s, const char *str)
{
    send(s, str, strlen(str), 0);
}

/*
 * a vargs wrapper for the write_user function.
 * This will enable you to send arguments directly to this function.
 * before ya mention it, Squirt, I already had this in a file before your snippet...
 * but I gotta admit that your snippet kicked my butt into actually implementing this ;)
 */
void
vwrite_user(UR_OBJECT user, const char *str, ...)
{
    va_list args;

    *vtext = '\0';
    va_start(args, str);
    vsprintf(vtext, str, args);
    va_end(args);
    write_user(user, vtext);
}

/*
 * Send message to user
 */
void
write_user(UR_OBJECT user, const char *str)
{
    char buff[OUT_BUFF_SIZE];
    const char *s;
    size_t cnt;
    size_t buffpos;
    size_t i;

    if (!user) {
        return;
    }
#ifdef NETLINKS
    if (message_nl(user, str)) {
        return;
    }
#endif
    /* the universal pager processing */
    if (user->universal_pager && !amsys->is_pager) {
        int pager;

        pager = user->pager < MAX_LINES || user->pager > 999 ? 23 : user->pager;
        add_pm(user, str);
        if (user->pm_count == pager) {
            user->pm_current = user->pm_last;
            user->misc_op = 25;
            return;
        }
        if (user->pm_count >= pager) {
            return;
        }
    }
    /* Process string and write to buffer */
    cnt = 0;
    buffpos = 0;
    for (s = str; *s; ++s) {
        /* Flush buffer if above high water mark;
         * 6 chars is max a single char can expand into */
        if (buffpos > OUT_BUFF_SIZE - 6) {
            send(user->socket, buff, buffpos, 0);
            buffpos = 0;
        }
        if (*s == '\n') {
            /* Reset terminal before every newline */
            if (user->colour) {
                memcpy(buff + buffpos, colour_codes[0].esc_code,
                        strlen(colour_codes[0].esc_code));
                buffpos += strlen(colour_codes[0].esc_code);
            }
            buff[buffpos++] = '\r';
            buff[buffpos++] = '\n';
            cnt = 0;
            continue;
        } else if (*s == '~') {
            /*
               Process colour commands eg ~FR. We have to strip out the commands
               from the string even if user doesnt have colour switched on hence
               the user->colour check isnt done just yet
             */
            for (i = 0; colour_codes[i].txt_code; ++i) {
                if (!strncmp(s + 1, colour_codes[i].txt_code,
                        strlen(colour_codes[i].txt_code))) {
                    break;
                }
            }
            if (colour_codes[i].txt_code) {
                if (user->colour) {
                    memcpy(buff + buffpos, colour_codes[i].esc_code,
                            strlen(colour_codes[i].esc_code));
                    buffpos += strlen(colour_codes[i].esc_code);
                }
                s += strlen(colour_codes[i].txt_code);
                continue;
            }
        } else if (*s == '^') {
            /* See if its a ^ before a ~ , if so then we print colour command as text */
            if (s[1] == '~') {
                ++s;
            }
        }
        buff[buffpos++] = *s;
        if (user->wrap && ++cnt >= SCREEN_WRAP) {
            buff[buffpos++] = '\r';
            buff[buffpos++] = '\n';
            cnt = 0;
        }
    }
    if (buffpos) {
        send(user->socket, buff, buffpos, 0);
    }
    /* Reset terminal at end of string */
    if (user->colour) {
        write_sock(user->socket, colour_codes[0].esc_code);
    }
}

void
vwrite_level(enum lvl_value lvl, int above, int dorecord, UR_OBJECT user,
        const char *str, ...)
{
    va_list args;

    *vtext = '\0';
    va_start(args, str);
    vsprintf(vtext, str, args);
    va_end(args);
    write_level(lvl, above, dorecord, vtext, user);
}

/*
 * Write to users of level <lvl> and above or below depending on above
 * variable; if 1 then above else below.  If user given then ignore that user
 * when writing to their level.
 */
void
write_level(enum lvl_value lvl, int above, int dorecord, const char *str,
        UR_OBJECT user)
{
    UR_OBJECT u;

    for (u = user_first; u; u = u->next) {
        if (check_igusers(u, user) && user->level < GOD) {
            continue;
        }
        if ((u->ignwiz && (com_num == WIZSHOUT || com_num == WIZEMOTE))
                || (u->ignlogons && logon_flag)) {
            continue;
        }
        if (u != user && !u->login && u->type != CLONE_TYPE) {
            if ((above && u->level >= lvl) || (!above && u->level <= lvl)) {
                if (u->afk) {
                    if (dorecord) {
                        record_afk(user, u, str);
                    }
                    continue;
                }
                if (u->malloc_start) {
                    if (dorecord) {
                        record_edit(user, u, str);
                    }
                    continue;
                }
                if (!u->ignall) {
                    write_user(u, str);
                }
                if (dorecord) {
                    record_tell(user, u, str);
                }
            }
        }
    }
}

/*
 * a vargs wrapper for the write_room function.  This will enable you
 * to send arguments directly to this function
 * This is more a subsid than a wrapper as it does not directly call write_room
 */
void
vwrite_room(RM_OBJECT rm, const char *str, ...)
{
    va_list args;

    *vtext = '\0';
    va_start(args, str);
    vsprintf(vtext, str, args);
    va_end(args);
    write_room_except(rm, vtext, NULL);
}

/*
 * Subsid function to below but this one is used the most
 */
void
write_room(RM_OBJECT rm, const char *str)
{
    write_room_except(rm, str, NULL);
}

/*
 * a vargs wrapper for the write_room_except function.  This will enable you
 * to send arguments directly to this function
 */
void
vwrite_room_except(RM_OBJECT rm, UR_OBJECT user, const char *str, ...)
{
    va_list args;

    *vtext = '\0';
    va_start(args, str);
    vsprintf(vtext, str, args);
    va_end(args);
    write_room_except(rm, vtext, user);
}

/*
 * Write to everyone in room rm except for "user". If rm is NULL write
 * to all rooms
 */
void
write_room_except(RM_OBJECT rm, const char *str, UR_OBJECT user)
{
    char buff[ARR_SIZE * 2];
    UR_OBJECT u;

    for (u = user_first; u; u = u->next) {
        if (u->login || !u->room || (u->room != rm && rm)
                || (u->ignall && !force_listen)
                || (u->ignshouts && (com_num == SHOUT || com_num == SEMOTE))
                || (u->ignlogons && logon_flag)
                || (u->igngreets && com_num == GREET)
                || u == user) {
            continue;
        }
        if (check_igusers(u, user) && user->level < ARCH) {
            continue;
        }
        if (u->type == CLONE_TYPE) {
            if (u->clone_hear == CLONE_HEAR_NOTHING || u->owner->ignall) {
                continue;
            }
            /*
               Ignore anything not in clones room, eg shouts, system messages
               and semotes since the clones owner will hear them anyway.
             */
            if (rm != u->room) {
                continue;
            }
            if (u->clone_hear == CLONE_HEAR_SWEARS) {
                if (!contains_swearing(str)) {
                    continue;
                }
            }
            *buff = '\0';
            sprintf(buff, "~FC[ %s ]:~RS %s", u->room->name, str);
            write_user(u->owner, buff);
        } else {
            write_user(u, str);
        }
    }
}

/*
 * a vargs wrapper for the write_room_except_both function.  This will enable you
 * to send arguments directly to this function
 */
void
vwrite_room_except_both(RM_OBJECT rm, UR_OBJECT u1, UR_OBJECT u2,
        const char *str, ...)
{
    va_list args;

    *vtext = '\0';
    va_start(args, str);
    vsprintf(vtext, str, args);
    va_end(args);
    write_room_except_both(rm, vtext, u1, u2);
}

/*
 * Write to everyone in room rm except for "u1" and "u2"
 */
void
write_room_except_both(RM_OBJECT rm, const char *str, UR_OBJECT u1,
        UR_OBJECT u2)
{
    char buff[ARR_SIZE * 2];
    UR_OBJECT u;

    for (u = user_first; u; u = u->next) {
        if (u->login || !u->room || (u->room != rm && rm)
                || (u->ignall && !force_listen)
                || (u->ignshouts && (com_num == SHOUT || com_num == SEMOTE))
                || (u->ignlogons && logon_flag)
                || (u->igngreets && com_num == GREET)
                || u == u1 || u == u2) {
            continue;
        }
        if (u->type == CLONE_TYPE) {
            if (u->clone_hear == CLONE_HEAR_NOTHING || u->owner->ignall) {
                continue;
            }
            /*
               Ignore anything not in clones room, eg shouts, system messages
               and semotes since the clones owner will hear them anyway.
             */
            if (rm != u->room) {
                continue;
            }
            if (u->clone_hear == CLONE_HEAR_SWEARS) {
                if (!contains_swearing(str)) {
                    continue;
                }
            }
            *buff = '\0';
            sprintf(buff, "~FC[ %s ]:~RS %s", u->room->name, str);
            write_user(u->owner, buff);
        } else {
            write_user(u, str);
        }
    }
}

/*
 * a vargs wrapper for the write_room_ignore function.  This will enable you
 * to send arguments directly to this function
 */
void
vwrite_room_ignore(RM_OBJECT rm, UR_OBJECT user, const char *str, ...)
{
    va_list args;

    *vtext = '\0';
    va_start(args, str);
    vsprintf(vtext, str, args);
    va_end(args);
    write_room_ignore(rm, user, vtext);
}

/*
 * Write to everyone in room rm except for "u1" and "u2"
 */
void
write_room_ignore(RM_OBJECT rm, UR_OBJECT user, const char *str)
{
    char buff[ARR_SIZE * 2];
    UR_OBJECT u;

    for (u = user_first; u; u = u->next) {
        if (u->login || !u->room || (u->room != rm && rm)
                || (u->ignall && !force_listen)
                || (u->ignshouts && (com_num == SHOUT || com_num == SEMOTE))
                || (u->ignlogons && logon_flag)
                || (u->igngreets && com_num == GREET)
                || (check_igusers(u, user) && user->level < ARCH)
                ) {
            continue;
        }
        if (u->type == CLONE_TYPE) {
            if (u->clone_hear == CLONE_HEAR_NOTHING || u->owner->ignall) {
                continue;
            }
            /*
               Ignore anything not in clones room, eg shouts, system messages
               and semotes since the clones owner will hear them anyway.
             */
            if (rm != u->room) {
                continue;
            }
            if (u->clone_hear == CLONE_HEAR_SWEARS) {
                if (!contains_swearing(str)) {
                    continue;
                }
            }
            *buff = '\0';
            sprintf(buff, "~FC[ %s ]:~RS %s", u->room->name, str);
            write_user(u->owner, buff);
        } else {
            write_user(u, str);
        }
    }
}

/*
 * Write to everyone on the users friends list
 * if revt=1 then record to the friends tell buffer
 */
void
write_friends(UR_OBJECT user, const char *str, int revt)
{
    UR_OBJECT u;

    for (u = user_first; u; u = u->next) {
        if (u->login
                || !u->room || u->type == CLONE_TYPE || (u->ignall && !force_listen)
                || u == user) {
            continue;
        }
        if (check_igusers(u, user) && user->level < GOD) {
            continue;
        }
        if (!user_is_friend(user, u)) {
            continue;
        }
        write_user(u, str);
        if (revt) {
            record_tell(user, u, str);
        }
    }
}

/*
 * Write a string to system log
 * type = what syslog(s) to write to
 * write_time = whether or not you have a time stamp imcluded
 * str = string passed - possibly with %s, %d, etc
 * ... = variable length args passed
 */
void
write_syslog(int type, int write_time, const char *str, ...)
{
    char dstr[32];
    char filename[80];
    time_t now;
    va_list args;

    /* vtext is a global variable */
    time(&now);
    va_start(args, str);
    if (write_time) {
        strftime(vtext, ARR_SIZE * 2, "%Y-%m-%d %H:%M:%S: ", localtime(&now));
        vsprintf(vtext + strlen(vtext), str, args);
    } else {
        vsprintf(vtext, str, args);
    }
    va_end(args);
    strftime(dstr, 32, "%Y%m%d", localtime(&amsys->boot_time));
    type &= amsys->logging; /* Do not log to turned off logs */
    if (type & SYSLOG) {
        sprintf(filename, "%s/%s.%s", LOGFILES, MAINSYSLOG, dstr);
        /* even if do_write_syslog fails, continue incase trying to write to others */
        do_write_syslog(filename);
    }
    if (type & REQLOG) {
        sprintf(filename, "%s/%s.%s", LOGFILES, REQSYSLOG, dstr);
        do_write_syslog(filename);
    }
#ifdef NETLINKS
    if (type & NETLOG) {
        sprintf(filename, "%s/%s.%s", LOGFILES, NETSYSLOG, dstr);
        do_write_syslog(filename);
    }
#endif
    if (type & ERRLOG) {
        sprintf(filename, "%s/%s.%s", LOGFILES, ERRSYSLOG, dstr);
        do_write_syslog(filename);
    }
}

/*
 * writes a string to a syslog file
 */
int
do_write_syslog(const char *filename)
{
    FILE *fp;

    fp = fopen(filename, "a");
    if (!fp) {
        return 0;
    }
    fputs(vtext, fp);
    fclose(fp);
    return 1;
}

/*
 * this version of the the last command log - the two procedures below - are
 * thanks to Karri (The Bat) Kalpio who makes KTserv
 * record the last command executed - helps find crashes
 */
void
record_last_command(UR_OBJECT user, CMD_OBJECT cmd, size_t len)
{
    char dstr[32];
    time_t now;

    time(&now);
    strftime(dstr, 32, "%a %Y-%m-%d %H:%M:%S", localtime(&now));
    sprintf(cmd_history[amsys->last_cmd_cnt & 15],
            "[%5d] %s - %-15s - %3d/%-2d - %s", amsys->last_cmd_cnt, dstr,
            cmd->name, (int) len, word_count, user->name);
    ++amsys->last_cmd_cnt;
}

/*
 * write the commands to the files
 */
void
dump_commands(int sig)
{
    sds filename;
    char dstr[32];
    FILE *fp;
    int i, j;

    strftime(dstr, 32, "%Y%m%d", localtime(&amsys->boot_time));
    filename = sdscatfmt(sdsempty(), "%s/%s.%s", LOGFILES, LAST_CMD, dstr);
    fp = fopen(filename, "w");
    if (!fp) {
        sdsfree(filename);
        return;
    }
    fprintf(fp, "Caught signal %d:\n\n", sig);
    j = amsys->last_cmd_cnt - 16;
    for (i = j > 0 ? j : 0; i < amsys->last_cmd_cnt; ++i) {
        fprintf(fp, "%s\n", cmd_history[i & 15]);
    }
    fclose(fp);
    sdsfree(filename);
}

/*
 * shows the name of a user if they are invis.  Records to tell buffer if rec=1
 */
void
write_monitor(UR_OBJECT user, RM_OBJECT rm, int rec)
{
    UR_OBJECT u;
    CMD_OBJECT cmd;

    /* get the current level of monitor */
    for (cmd = first_command; cmd; cmd = cmd->next) {
        if (cmd->id == MONITOR) {
            break;
        }
    }
    /* write monitor */
    for (u = user_first; u; u = u->next) {
        if (u == user || u->login || u->type == CLONE_TYPE) {
            continue;
        }
        if (!u->monitor) {
            continue;
        }
        if (has_xcom(u, MONITOR)) {
            continue;
        }
        if ((!cmd || u->level < cmd->level) && !has_gcom(u, MONITOR)) {
            continue;
        }
        if (u->room == rm || !rm) {
            if (!u->ignall) {
                vwrite_user(u, "~BB~FG[%s]~RS ", user->name);
            }
            /* FIXME: This records variable text; text is not set */
            if (rec) {
                record_tell(user, u, text);
            }
        }
    }
}

/*
 * Page a file out to user. Colour commands in files will only work if
 * there is a valid user otherwise we dont know if the terminal can support
 * colour or not. Return values:
 * 0 = cannot find file, 1 = found file, 2 = found and finished
 */
int
more(UR_OBJECT user, int sock, const char *filename)
{
    char buff[OUT_BUFF_SIZE];
    const char *str;
    const char *s;
    FILE *fp;
    int num_chars, lines, retval, len, pager;
    size_t i, buffpos, cnt;

    /* see if we can open the file */
    fp = fopen(filename, "r");
    if (!fp) {
        if (user) {
            user->filepos = 0;
        }
        return 0;
    }
    if (!user) {
        pager = 23;
    } else {
        /* jump to reading posn in file */
        fseek(fp, user->filepos, 0);
        pager = user->pager < MAX_LINES || user->pager > 99 ? 23 : user->pager;
    }
    --pager;
    *text = '\0';
    buffpos = 0;
    num_chars = 0;
    len = 0;
    cnt = 0;
    /* If user is remote then only do 1 line at a time */
    if (user && user->type == REMOTE_TYPE) {
        lines = 1;
    } else {
        lines = 0;
    }
    /* Go through file */
    for (str = fgets(text, (sizeof text) - 1, fp);
            str; str = fgets(text, (sizeof text) - 1, fp)) {
        if (lines >= pager && user) {
            break;
        }
#ifdef NETLINKS
        if (message_nl(user, str)) {
            ++lines;
            num_chars += strlen(str);
            continue;
        }
#endif
        /* Process line from file */
        for (s = str; *s; ++s) {
            if (buffpos > OUT_BUFF_SIZE - (6 < USER_NAME_LEN ? USER_NAME_LEN : 6)) {
                send(sock, buff, buffpos, 0);
                buffpos = 0;
            }
            if (*s == '\n') {
                /* Reset terminal before every newline */
                if (user && user->colour) {
                    memcpy(buff + buffpos, colour_codes[0].esc_code,
                            strlen(colour_codes[0].esc_code));
                    buffpos += strlen(colour_codes[0].esc_code);
                }
                buff[buffpos++] = '\r';
                buff[buffpos++] = '\n';
                cnt = 0;
                continue;
            } else if (*s == '~') {
                /* process if colour variable */
                for (i = 0; colour_codes[i].txt_code; ++i) {
                    if (!strncmp(s + 1, colour_codes[i].txt_code,
                            strlen(colour_codes[i].txt_code))) {
                        break;
                    }
                }
                if (colour_codes[i].txt_code) {
                    if (user && user->colour) {
                        memcpy(buff + buffpos, colour_codes[i].esc_code,
                                strlen(colour_codes[i].esc_code));
                        buffpos += strlen(colour_codes[i].esc_code);
                    }
                    s += strlen(colour_codes[i].txt_code);
                    continue;
                }
                /* process if user name variable */
                if (s[1] == '$') {
                    if (user) {
                        memcpy(buff + buffpos, user->bw_recap, strlen(user->bw_recap));
                        buffpos += strlen(user->bw_recap);
                        cnt += strlen(user->bw_recap);
                    }
                    ++s;
                    continue;
                }
                if (s[1] == '~') {
                    ++s;
                }
            } else if (*s == '^') {
                if (s[1] == '~') {
                    ++s;
                }
            }
            buff[buffpos++] = *s;
            if (user && user->wrap && ++cnt >= SCREEN_WRAP) {
                buff[buffpos++] = '\r';
                buff[buffpos++] = '\n';
                cnt = 0;
            }
        }
        len = strlen(str);
        num_chars += len;
        lines += len / SCREEN_WRAP + (len < SCREEN_WRAP);
    }
    if (buffpos && sock != -1) {
        send(sock, buff, buffpos, 0);
    }
    /* if user is logging on dont page file */
    if (!user) {
        fclose(fp);
        return 2;
    }
    if (!str) {
        user->filepos = 0;
        user->pagecnt = 0;
        for (i = 0; i < MAX_PAGES; ++i) {
            user->pages[i] = 0;
        }
        *user->page_file = '\0';
        write_user(user, "\n");
        no_prompt = 0;
        retval = 2;
    } else {
        struct stat stbuf;

        /* store file position and file name */
        user->filepos += num_chars;
        user->pages[++user->pagecnt] = user->filepos;
        strcpy(user->page_file, filename);
        /*
           We use E here instead of Q because when on a remote system and
           in COMMAND mode the Q will be intercepted by the home system and
           quit the user
         */
        vwrite_user(user,
                "~BB~FG-=[~OL%d%%~RS~BB~FG]=- (~OLR~RS~BB~FG)EDISPLAY, (~OLB~RS~BB~FG)ACK, (~OLE~RS~BB~FG)XIT, <RETURN> TO CONTINUE:~RS ",
                fstat(fileno(fp), &stbuf) == -1
                ? -1 : (100 * user->filepos) / (int) stbuf.st_size);
        no_prompt = 1;
        retval = 1;
    }
    fclose(fp);
    return retval;
}

/*
 * Page out a list of users of the level given.  If user_page_lev is NUM_LEVELS
 * then page out all of the users
 */
int
more_users(UR_OBJECT user)
{
    UD_OBJECT entry;
    int i, lines;

    if (!first_user_entry) {
        return 0;
    }
    i = 0;
    lines = 0;
    for (entry = first_user_entry; entry; entry = entry->next) {
        if (user->user_page_lev != NUM_LEVELS
                && user->user_page_lev != entry->level) {
            /* skip users that are not part of the level user data set */
            continue;
        }
        if (i++ < user->user_page_pos) {
            /* skip to the position of the page in the user data */
            continue;
        }
        if (lines++ >= user->pager) {
            break;
        }
        ++user->user_page_pos;
        /* does not matter if did not boot in verbose mode as entry->date will be empty anyway */
        vwrite_user(user, "%d) %-*s : %s\n", entry->level, USER_NAME_LEN,
                entry->name, entry->date);
    }
    if (entry) {
        write_user(user, "~BB~FG-=[*]=- PRESS <RETURN>, E TO EXIT:~RS ");
        no_prompt = 1;
        user->misc_op = 16;
        return 1;
    }
    write_user(user, "\n");
    user->user_page_pos = 0;
    user->user_page_lev = NUM_LEVELS;
    user->misc_op = 0;
    return 2;
}

/*
 * adds a string to the user history list
 */
void
add_history(char *username, int showtime, const char *str, ...)
{
    sds filename;
    FILE *fp;
    va_list args;
    time_t now;

    strtoname(username);
    filename = sdscatfmt(sdsempty(), "%s/%s/%s.H", USERFILES, USERHISTORYS, username);
    fp = fopen(filename, "a");
    if (!fp) {
        sdsfree(filename);
        return;
    }
    time(&now);
    va_start(args, str);
    if (showtime) {
        strftime(vtext, ARR_SIZE * 2, "%Y-%m-%d %H:%M:%S", localtime(&now));
        fprintf(fp, "%s: ", vtext);
    }
    vfprintf(fp, str, args);
    va_end(args);
    fclose(fp);
    sdsfree(filename);
    write_syslog(SYSLOG, 1, "HISTORY added for %s.\n", username);
}



/******************************************************************************
 Logon/off functions
 *****************************************************************************/

/*
 * Login function. Lots of nice inline code :)
 */
void
login(UR_OBJECT user, char *inpstr)
{
    UR_OBJECT u;
    int i;
    char name[ARR_SIZE], passwd[ARR_SIZE], filename[80], motdname[80];

    *name = '\0';
    *passwd = '\0';
    switch (user->login) {
    case LOGIN_NAME:
        sscanf(inpstr, "%s", name);
        if (iscntrl(*name)) {
            write_user(user, "\nGive me a name: ");
            return;
        }
        if (!strcasecmp(name, "quit")) {
            write_user(user, "\n\n*** Abandoning login attempt ***\n\n");
            disconnect_user(user);
            return;
        }
        if (!strcasecmp(name, "who")) {
            /*
               if you do not like this logon who, then replace it with the normal
               one of who(user,0);
             */
            login_who(user);
            write_user(user, "\nGive me a name: ");
            return;
        }
        if (!strcasecmp(name, "version")) {
            vwrite_user(user, "\nAmnuts version %s\n\nGive me a name: ", AMNUTSVER);
            return;
        }
        /* see if only letters in login */
        for (i = 0; name[i]; ++i) {
            if (!isalpha(name[i])) {
                break;
            }
        }
        if (name[i]) {
            write_user(user, "\nOnly letters are allowed in a name.\n\n");
            attempts(user);
            return;
        }
        if (i < USER_NAME_MIN) {
            write_user(user, "\nName too short.\n\n");
            attempts(user);
            return;
        }
        if (i > USER_NAME_LEN) {
            write_user(user, "\nName too long.\n\n");
            attempts(user);
            return;
        }
        if (contains_swearing(name)) {
            write_user(user, "\nYou cannot use a name like that, sorry!\n\n");
            attempts(user);
            return;
        }
        strtolower(name);
        *name = toupper(*name);
        if (user_banned(name)) {
            write_user(user, "\nYou are banned from this talker.\n\n");
            disconnect_user(user);
            write_syslog(SYSLOG, 1, "Attempted login by banned user %s.\n", name);
            return;
        }
        strcpy(user->name, name);
        /* If user has hung on another login clear that session */
        for (u = user_first; u; u = u->next) {
            if (u->login && u != user && !strcmp(u->name, user->name)) {
                disconnect_user(u);
            }
        }
        if (!load_user_details(user)) {
#ifdef WIZPORT
            if (user->wizport) {
                write_user(user,
                        "\nSorry, new logins cannot be created on this port.\n\n");
                disconnect_user(user);
                return;
            }
#endif
            if (amsys->minlogin_level != NUM_LEVELS) {
                write_user(user,
                        "\nSorry, new logins cannot be created at this time.\n\n");
                disconnect_user(user);
                return;
            }
            if (site_banned(user->site, 1)) {
                write_user(user,
                        "\nSorry, new accounts from your site have been banned.\n\n");
                write_syslog(SYSLOG, 1,
                        "Attempted login by a new user from banned new users site %s.\n",
                        user->site);
                disconnect_user(user);
                return;
            }
            write_user(user, "New user...\n");
        } else {
#ifdef WIZPORT
            if (user->wizport && user->level < amsys->wizport_level) {
                vwrite_user(user,
                        "\nSorry, only users of level %s and above can log in on this port.\n\n",
                        user_level[amsys->wizport_level].name);
                disconnect_user(user);
                return;
            }
#endif
            if (amsys->minlogin_level != NUM_LEVELS
                    && user->level < amsys->minlogin_level) {
                write_user(user,
                        "\nSorry, the talker is currently locked out to users of your level.\n\n");
                disconnect_user(user);
                return;
            }
        }
        write_user(user, "Give me a password: ");
        echo_off(user);
        user->login = LOGIN_PASSWD;
        return;

    case LOGIN_PASSWD:
        sscanf(inpstr, "%s", passwd);
        /* if new user... */
        if (!*user->pass) {
            i = strlen(passwd);
            if (i < PASS_MIN) {
                write_user(user, "\n\nPassword too short.\n\n");
                attempts(user);
                return;
            }
            /* Via use of crypt() */
            if (i > 8) {
                write_user(user,
                        "\n\nWARNING: Only the first eight characters of password will be used!\n\n");
            }
            strcpy(user->pass, crypt(passwd, crypt_salt));
            write_user(user, "\n");
            sprintf(filename, "%s/%s", MISCFILES, RULESFILE);
            if (more(NULL, user->socket, filename)) {
                write_user(user,
                        "\nBy typing your password in again you are accepting the above rules.\n");
                write_user(user,
                        "If you do not agree with the rules, then disconnect now.\n");
            }
            write_user(user, "\nPlease confirm password: ");
            user->login = LOGIN_CONFIRM;
        } else {
            if (strcmp(user->pass, crypt(passwd, user->pass))) {
                write_user(user, "\n\nIncorrect login.\n\n");
                attempts(user);
                return;
            }
            echo_on(user);
            ++amsys->logons_old;
#ifdef IDENTD
            /* check for ident user ident */
            if (!strcmp(user->name, IDENTUSER)) {
                start_ident(user);
                return;
            }
#endif
            /*
               Instead of connecting the user with:  "connect_user(user);  return;"
               Show the user the MOTD2 so that they can read it.  If you wanted, you
               could make showing the MOTD2 optional, in which case use an "if" clause
               to either do the above or the following...
             */
            cls(user);
            /* If there is no motd2 files then do not display them */
            if (amsys->motd2_cnt) {
                sprintf(motdname, "%s/motd2/motd%d", MOTDFILES, (get_motd_num(2)));
                more(user, user->socket, motdname);
            }
            write_user(user, "Press return to continue: ");
            user->login = LOGIN_PROMPT;
        }
        return;

    case LOGIN_CONFIRM:
        sscanf(inpstr, "%s", passwd);
        if (strcmp(user->pass, crypt(passwd, user->pass))) {
            write_user(user, "\n\nPasswords do not match.\n\n");
            attempts(user);
            return;
        }
        echo_on(user);
        strcpy(user->desc, "is a newbie");
        strcpy(user->in_phrase, "enters");
        strcpy(user->out_phrase, "goes");
        strcpy(user->date, (long_date(1)));
        strcpy(user->recap, user->name);
        strcpy(user->bw_recap, colour_com_strip(user->recap));
        *user->last_site = '\0';
        user->level = NEW;
        user->unarrest = NEW;
        user->muzzled = JAILED; /* FIXME: Use sentinel other JAILED */
        user->command_mode = 0;
        user->prompt = amsys->prompt_def;
        user->colour = amsys->colour_def;
        user->charmode_echo = amsys->charecho_def;
        save_user_details(user, 1);
        add_user_node(user->name, user->level);
        add_user_date_node(user->name, (long_date(1)));
        add_history(user->name, 1, "Was initially created.\n");
        write_syslog(SYSLOG, 1, "New user \"%s\" created.\n", user->name);
        ++amsys->logons_new;
        /* Check out above for explaination of this */
        cls(user);
        /* If there is no motd2 files then do not display them */
        if (amsys->motd2_cnt) {
            sprintf(motdname, "%s/motd2/motd%d", MOTDFILES, (get_motd_num(2)));
            more(user, user->socket, motdname);
        }
        write_user(user, "Press return to continue: ");
        user->login = LOGIN_PROMPT;
        return;

    case LOGIN_PROMPT:
        user->login = 0;
        write_user(user, "\n\n");
        connect_user(user);
        return;
    }
}

/*
 * Count up attempts made by user to login
 */
void
attempts(UR_OBJECT user)
{
    ++user->attempts;
    if (user->attempts == LOGIN_ATTEMPTS) {
        write_user(user, "\nMaximum attempts reached.\n\n");
        disconnect_user(user);
        return;
    }
    reset_user(user);
    user->login = LOGIN_NAME;
    *user->pass = '\0';
    write_user(user, "Give me a name: ");
    echo_on(user);
}

/*
 * Display better stats when logging in.  I personally use this rather than the MOTD2
 * but you can use it where you want.  Gives better output than that "you last logged in"
 * line tht was in connect_user
 */
void
show_login_info(UR_OBJECT user)
{
    static const char *const see[] = {"~OL~FYinvisible", "~OL~FCvisible"};
    static const char *const myoffon[] = {"~OL~FCoff", "~OL~FRon "};
    static const char *const times[] = {"morning", "afternoon", "evening"};
    char temp[ARR_SIZE], text2[ARR_SIZE];
    time_t now;
    const struct tm *date;
    int yes, cnt, phase, exline;

    yes = exline = 0;
    write_user(user,
            "\n+----------------------------------------------------------------------------+\n");
    time(&now);
    date = localtime(&now);
    if (date->tm_hour >= 0 && date->tm_hour < 12) {
        phase = 0;
    } else if (date->tm_hour >= 12 && date->tm_hour < 18) {
        phase = 1;
    } else {
        phase = 2;
    }
    sprintf(text, "Good %s, %s~RS, and welcome to %s", times[phase],
            user->recap, TALKER_NAME);
    cnt = 74 + teslen(text, 74);
    vwrite_user(user, "| %-*.*s |\n", cnt, cnt, text);
    sprintf(text, "You are joining us ~OL%s~RS", long_date(1));
    cnt = 74 + teslen(text, 74);
    vwrite_user(user, "| %-*.*s |\n", cnt, cnt, text);
    write_user(user,
            "+----------------------------------------------------------------------------+\n");
    if (*user->last_site) {
        strftime(temp, ARR_SIZE, "%a %Y-%m-%d %H:%M:%S",
                localtime(&user->last_login));
        sprintf(text, "Last login date : ~OL%-25s~RS   Your level is : ~OL%s~RS",
                temp, user_level[user->level].name);
        cnt = 74 + teslen(text, 74);
        vwrite_user(user, "| %-*.*s |\n", cnt, cnt, text);
        sprintf(text, "Last login site : ~OL%s~RS", user->last_site);
        cnt = 74 + teslen(text, 74);
        vwrite_user(user, "| %-*.*s |\n", cnt, cnt, text);
        ++exline;
    }
    if (user->level >= (enum lvl_value) command_table[INVIS].level) {
        sprintf(text, "You are currently %s~RS", see[user->vis]);
        if (user->level >= (enum lvl_value) command_table[MONITOR].level) {
            sprintf(text2, " and your monitor is %s~RS", myoffon[user->monitor]);
            strcat(text, text2);
        } else {
            strcat(text, "\n");
        }
        cnt = 74 + teslen(text, 74);
        vwrite_user(user, "| %-*.*s |\n", cnt, cnt, text);
        ++exline;
    } else if (user->level >= (enum lvl_value) command_table[MONITOR].level) {
        sprintf(text, "Your monitor is currently %s~RS", myoffon[user->monitor]);
        cnt = 74 + teslen(text, 74);
        vwrite_user(user, "| %-*.*s |\n", cnt, cnt, text);
        ++exline;
    }
    *text2 = '\0';
    *text = '\0';
    *temp = '\0';
    yes = 0;
    if (user->ignall) {
        strcat(text2, "~FR~OLEVERYTHING!");
        ++yes;
    }
    if (!yes) {
        if (user->igntells) {
            strcat(text2, "Tells   ");
            ++yes;
        }
        if (user->ignshouts) {
            strcat(text2, "Shouts   ");
            ++yes;
        }
        if (user->ignpics) {
            strcat(text2, "Pics   ");
            ++yes;
        }
        if (user->ignlogons) {
            strcat(text2, "Logons   ");
            ++yes;
        }
        if (user->ignwiz) {
            strcat(text2, "Lawtells   ");
            ++yes;
        }
        if (user->igngreets) {
            strcat(text2, "Greets   ");
            ++yes;
        }
        if (user->ignbeeps) {
            strcat(text2, "Beeps");
            ++yes;
        }
    }
    if (yes) {
        sprintf(text, "Ignoring : ~OL%s~RS", text2);
        cnt = 74 + teslen(text, 74);
        vwrite_user(user, "| %-*.*s |\n", cnt, cnt, text);
        ++exline;
    }
    if (exline) {
        write_user(user,
                "+----------------------------------------------------------------------------+\n");
    }
    if (user->level >= (enum lvl_value) command_table[REMINDER].level) {
        cnt = has_reminder_today(user);
        sprintf(text, "You have ~OL%d~RS reminder%s for today", cnt,
                PLTEXT_S(cnt));
        vwrite_user(user, "| %-80.80s |\n", text);
        cnt = remove_old_reminders(user);
        if (cnt) {
            sprintf(text,
                    "There %s ~OL%d~RS reminder%s removed due to date being passed",
                    PLTEXT_WAS(cnt), cnt, PLTEXT_S(cnt));
            vwrite_user(user, "| %-80.80s |\n", text);
        }
        write_user(user,
                "+----------------------------------------------------------------------------+\n");
    }
}

/*
 * Connect the user to the talker proper
 */
void
connect_user(UR_OBJECT user)
{
    sds rmname;
    const char *bp;
    UR_OBJECT u;
    int was_private, cnt, newmail;

    /* See if user already connected */
    for (u = user_first; u; u = u->next) {
        if (user != u && user->type != CLONE_TYPE && !strcmp(user->name, u->name)) {
            break;
        }
    }
    if (u) {
#ifdef NETLINKS
        /* deal with a login if the user is connected to a remote talker */
        if (remove_nl(u)) {
            /* FIXME: This does not handle several things like disconnect_user() */
            vwrite_room_except(u->room, u, "%s~RS vanishes.\n", u->recap);
            write_user(u, "\n~FB~OLYou are pulled back through cyberspace...\n");
            reset_access(u->room);
            destruct_user(u);
            --amsys->num_of_users;
        } else
#endif
        {
            UR_OBJECT u2;

            write_user(user,
                    "\n\nYou are already connected - switching to old session...\n");
            write_syslog(SYSLOG, 1, "%s swapped sessions (%s)\n", user->name,
                    user->site);
            shutdown(u->socket, SHUT_WR);
            close(u->socket);
            u->socket = user->socket;
            strcpy(u->ipsite, user->ipsite);
            strcpy(u->site, user->site);
            strcpy(u->site_port, user->site_port);
#ifdef WIZPORT
            u->wizport = user->wizport;
#endif
            /* Reset the sockets on any clones */
            for (u2 = user_first; u2; u2 = u2->next) {
                if (u2->type == CLONE_TYPE && u2->owner == u) {
                    u2->socket = u->socket;
                }
            }
            destruct_user(user);
            --amsys->num_of_logins;
#ifdef NETLINKS
            if (!action_nl(user, "look", NULL))
#endif
            {
                look(u);
                prompt(u);
            }
            return;
        }
    }
    logon_flag = 1;
    if (!user->vis && user->level < (enum lvl_value) command_table[INVIS].level) {
        user->vis = 1;
    }
    if (user->vis) {
#ifdef WIZPORT
        vwrite_level(ARCH, 1, NORECORD, user,
                "%s~RS ~FC(Site %s : Site Port %s : Talker Port %s)\n",
                user->recap, user->site, user->site_port,
                !user->wizport ? amsys->mport_port : amsys->wport_port);
#else
        vwrite_level(ARCH, 1, NORECORD, user,
                "%s~RS ~FC(Site %s : Site Port %s : Talker Port %s)\n",
                user->recap, user->site, user->site_port, amsys->mport_port);
#endif
    } else {
#ifdef WIZPORT
        vwrite_level(user->level < ARCH ? ARCH : user->level, 1, NORECORD, user,
                "~OL~FY[ INVIS ]~RS %s~RS ~FC(Site %s : Site Port %s : Talker Port : %s)\n",
                user->recap, user->site, user->site_port,
                !user->wizport ? amsys->mport_port : amsys->wport_port);
#else
        vwrite_level(user->level < ARCH ? ARCH : user->level, 1, NORECORD, user,
                "~OL~FY[ INVIS ]~RS %s~RS ~FC(Site %s : Site Port %s : Talker Port : %s)\n",
                user->recap, user->site, user->site_port, amsys->mport_port);
#endif
    }
    was_private = check_start_room(user);
    if (user->room == room_first) {
        rmname = sdsempty();
    } else if (is_personal_room(user->room)) {
        rmname = sdscatfmt(sdsempty(), " (%s~RS)", user->room->show_name);
    } else {
        rmname = sdscatfmt(sdsempty(), " (%s)", user->room->name);
    }
    if (user->level == JAILED) {
        vwrite_room_except(NULL, user,
                "~OL[Being thrown into jail is:~RS %s~RS %s~RS~OL]\n",
                user->recap, user->desc);
        vwrite_room_except(user->room, user, "%s~RS %s.\n", user->recap,
                user->in_phrase);
        vwrite_user(user, "\nYou have been ~FRarrested~RS - connecting%s.\n\n",
                rmname);

    } else {
        if (user->vis) {
            if (user->level < WIZ) {
                vwrite_room(NULL, "~OL[Entering~RS%s~OL is:~RS %s~RS %s~RS~OL]\n",
                        rmname, user->recap, user->desc);
            } else {
                vwrite_room(NULL, "\007~OL[Entering~RS%s~OL is:~RS %s~RS %s~RS~OL]\n",
                        rmname, user->recap, user->desc);
            }
            vwrite_room_except(user->room, user, "%s~RS %s.\n", user->recap,
                    user->in_phrase);
        } else {
            vwrite_level(user->level < WIZ ? WIZ : user->level, 1, NORECORD, user,
                    "~OL~FY[ INVIS ]~RS ~OL[Entering~RS%s~OL is:~RS %s~RS %s~RS~OL]\n",
                    rmname, user->recap, user->desc);
            if (user->level < GOD) {
                write_room_except(user->room, invisenter, user);
            }
        }
        if (user->lroom == 2) {
            vwrite_user(user, "\nYou have been ~FRshackled~RS - connecting%s.\n\n",
                    rmname);
        } else if (was_private) {
            vwrite_user(user,
                    "\nThe room you logged out of is now private - connecting%s.\n\n",
                    rmname);
        } else {
            vwrite_user(user, "\nYou are connecting%s.\n\n", rmname);
        }
    }
    sdsfree(rmname);
    logon_flag = 0;
    ++user->logons;
    alert_friends(user);
    show_login_info(user);
    user->last_login = time(0); /* set to now */
    look(user);
    bp = user->ignbeeps ? "" : "\007";
    /* show how much mail the user has */
    newmail = mail_sizes(user->name, 1);
    if (newmail) {
        vwrite_user(user,
                "%s~FC~OL*** YOU HAVE ~RS~OL%d~FC UNREAD MAIL MESSAGE%s ***\n",
                bp, newmail, newmail == 1 ? "" : "S");
    } else {
        cnt = mail_sizes(user->name, 0);
        if (cnt) {
            vwrite_user(user,
                    "~FC*** You have ~RS~OL%d~RS~FC message%s in your mail box ***\n",
                    cnt, PLTEXT_S(cnt));
        }
    }
    /* should they get the autopromote message? */
    if (user->accreq != -1 && amsys->auto_promote) {
        vwrite_user(user,
                "\n%s~OL~FY****************************************************************************\n",
                bp);
        write_user(user,
                "~OL~FY*               ~FRTO BE AUTO-PROMOTED PLEASE READ CAREFULLY~FY                  *\n");
        write_user(user,
                "~OL~FY* You must set your description (.desc), set your gender (.set gender) and *\n");
        write_user(user,
                "~OL~FY*   use the .accreq command--once you do all these you will be promoted    *\n");
        write_user(user,
                "~OL~FY****************************************************************************\n\n");
    }
    prompt(user);
    record_last_login(user->name);
#ifdef WIZPORT
    write_syslog(SYSLOG, 1, "%s logged in on port %s from %s:%s.\n", user->name,
            !user->wizport ? amsys->mport_port : amsys->wport_port,
            user->site, user->site_port);
#else
    write_syslog(SYSLOG, 1, "%s logged in on port %s from %s:%s.\n", user->name,
            amsys->mport_port, user->site, user->site_port);
#endif
    ++amsys->num_of_users;
    --amsys->num_of_logins;
}

/*
 * Disconnect user from talker
 */
void
disconnect_user(UR_OBJECT user)
{
    RM_OBJECT rm;
    long int onfor, hours, mins;

    rm = user->room;
    if (user->login) {
        shutdown(user->socket, SHUT_WR);
        close(user->socket);
        destruct_user(user);
        --amsys->num_of_logins;
        return;
    }
    if (user->type == CLONE_TYPE) {
        destruct_user(user);
        reset_access(rm);
        return;
    }
#ifdef NETLINKS
    if (remove_nl(user)) {
        vwrite_room_except(rm, user, "%s~RS ~FR~OLis banished from here!\n",
                user->recap);
        write_user(user,
                "\n~FR~OLYou are pulled back in disgrace to your own domain...\n");
        write_syslog(NETLOG, 1, "NETLINK: Remote user %s removed.\n", user->name);
    } else
#endif
    {
        char dstr[32];
        time_t now;

        time(&now);
        onfor = now - user->last_login;
        hours = (onfor % 86400) / 3600;
        mins = (onfor % 3600) / 60;
        if (user->malloc_start) {
            memset(user->malloc_start, 0, MAX_LINES * 81);
            free(user->malloc_start);
            user->malloc_start = NULL;
            user->malloc_end = NULL;
            user->misc_op = 0;
            user->edit_op = 0;
            user->edit_line = 0;
            /* reset ignore status--incase user was in the editor */
            user->ignall = user->ignall_store;
        }
        save_user_details(user, 1);
        write_syslog(SYSLOG, 1, "%s logged out.\n", user->name);
        write_user(user, "\n~OL~FBYou are removed from this reality...\n\n");
        vwrite_user(user, "You were logged on from site %s\n", user->site);
        strftime(dstr, 32, "%a %Y-%m-%d %H:%M:%S", localtime(&now));
        vwrite_user(user, "On %s, for a total of %d hour%s and %d minute%s.\n\n",
                dstr, (int) hours, PLTEXT_S(hours), (int) mins,
                PLTEXT_S(mins));
        shutdown(user->socket, SHUT_WR);
        close(user->socket);
        logon_flag = 1;
        if (user->vis) {
            vwrite_room(NULL, "~OL[Leaving is:~RS %s~RS %s~RS~OL]\n", user->recap,
                    user->desc);
        } else {
            vwrite_level(WIZ, 1, NORECORD, NULL,
                    "~OL~FY[ INVIS ]~RS ~OL[Leaving is:~RS %s~RS %s~RS~OL]\n",
                    user->recap, user->desc);
        }
        logon_flag = 0;
#ifdef NETLINKS
        release_nl(user);
#endif
    }
    --amsys->num_of_users;
    record_last_logout(user->name);
    destroy_user_clones(user);
    destruct_all_review_buffer(user);
    destruct_all_flagged_users(user);
    destruct_user(user);
    reset_access(rm);
}



/******************************************************************************
 Misc and line editor functions
 *****************************************************************************/

/*** Stuff that is neither speech nor a command is dealt with here ***/
int
misc_ops(UR_OBJECT user, char *inpstr)
{
    char filename[80];
    int i = 0;

    switch (user->misc_op) {
    case 1:
        if (tolower(*inpstr) == 'y') {
            if (amsys->rs_countdown && !amsys->rs_which) {
                if (amsys->rs_countdown > 60) {
                    vwrite_room(NULL,
                            "\n\07~OLSYSTEM: ~FR~LISHUTDOWN INITIATED, shutdown in %d minute%s, %d second%s!\n\n",
                            amsys->rs_countdown / 60,
                            PLTEXT_S(amsys->rs_countdown / 60),
                            amsys->rs_countdown % 60,
                            PLTEXT_S(amsys->rs_countdown % 60));
                } else {
                    vwrite_room(NULL,
                            "\n\07~OLSYSTEM: ~FR~LISHUTDOWN INITIATED, shutdown in %d second%s!\n\n",
                            amsys->rs_countdown, PLTEXT_S(amsys->rs_countdown));
                }
                write_syslog(SYSLOG, 1,
                        "%s initiated a %d second%s SHUTDOWN countdown.\n",
                        user->name, amsys->rs_countdown,
                        PLTEXT_S(amsys->rs_countdown));
                amsys->rs_user = user;
                amsys->rs_announce = time(0);
                user->misc_op = 0;
                prompt(user);
                return 1;
            }
            talker_shutdown(user, NULL, 0);
        }
        /* This will reset any reboot countdown that was started, oh well */
        amsys->rs_countdown = 0;
        amsys->rs_announce = 0;
        amsys->rs_which = -1;
        amsys->rs_user = NULL;
        user->misc_op = 0;
        prompt(user);
        return 1;

    case 2:
        if (tolower(*inpstr) == 'e') {
            user->misc_op = 0;
            user->filepos = 0;
            *user->page_file = '\0';
            for (i = 0; i < MAX_PAGES; ++i) {
                user->pages[i] = 0;
            }
            user->pagecnt = 0;
            prompt(user);
            return 1;
        } else if (tolower(*inpstr) == 'r') {
            user->pagecnt = user->pagecnt < 1 ? 0 : user->pagecnt - 1;
            user->filepos = user->pages[user->pagecnt];
        } else if (tolower(*inpstr) == 'b') {
            user->pagecnt = user->pagecnt < 2 ? 0 : user->pagecnt - 2;
            user->filepos = user->pages[user->pagecnt];
        }
        if (more(user, user->socket, user->page_file) != 1) {
            user->misc_op = 0;
            user->filepos = 0;
            *user->page_file = '\0';
            for (i = 0; i < MAX_PAGES; ++i) {
                user->pages[i] = 0;
            }
            user->pagecnt = 0;
            prompt(user);
        }
        return 1;

    case 3:
        /* writing on board */
    case 4:
        /* Writing mail */
    case 5:
        /* doing profile */
        editor(user, inpstr);
        return 1;

    case 6:
        if (tolower(*inpstr) == 'y') {
            delete_user(user, 1);
        } else {
            user->misc_op = 0;
            prompt(user);
        }
        return 1;

    case 7:
        if (tolower(*inpstr) == 'y') {
            if (amsys->rs_countdown && amsys->rs_which > 0) {
                if (amsys->rs_which == 1) {
                    if (amsys->rs_countdown > 60) {
                        vwrite_room(NULL,
                                "\n\07~OLSYSTEM: ~FY~LIREBOOT INITIATED, rebooting in %d minute%s, %d second%s!\n\n",
                                amsys->rs_countdown / 60,
                                PLTEXT_S(amsys->rs_countdown / 60),
                                amsys->rs_countdown % 60,
                                PLTEXT_S(amsys->rs_countdown % 60));
                    } else {
                        vwrite_room(NULL,
                                "\n\07~OLSYSTEM: ~FY~LIREBOOT INITIATED, rebooting in %d second%s!\n\n",
                                amsys->rs_countdown, PLTEXT_S(amsys->rs_countdown));
                    }
                } else {
                    if (amsys->rs_countdown > 60) {
                        vwrite_room(NULL,
                                "\n\07~OLSYSTEM: ~FY~LISEAMLESS REBOOT INITIATED, rebooting in %d minute%s, %d second%s!\n\n",
                                amsys->rs_countdown / 60,
                                PLTEXT_S(amsys->rs_countdown / 60),
                                amsys->rs_countdown % 60,
                                PLTEXT_S(amsys->rs_countdown % 60));
                    } else {
                        vwrite_room(NULL,
                                "\n\07~OLSYSTEM: ~FY~LISEAMLESS REBOOT INITIATED, rebooting in %d second%s!\n\n",
                                amsys->rs_countdown, PLTEXT_S(amsys->rs_countdown));
                    }
                }
                write_syslog(SYSLOG, 1, "%s initiated a %d second%s %s countdown.\n",
                        user->name, amsys->rs_countdown,
                        PLTEXT_S(amsys->rs_countdown),
                        (amsys->rs_which == 1 ? "REBOOT" : "SEAMLESS REBOOT"));
                amsys->rs_user = user;
                amsys->rs_announce = time(0);
                user->misc_op = 0;
                prompt(user);
                return 1;
            }
            user->misc_op = 0;
            talker_shutdown(user, NULL, amsys->rs_which);
            return 1;
        }
        if (amsys->rs_which > 0 && amsys->rs_countdown && !amsys->rs_user) {
            amsys->rs_countdown = 0;
            amsys->rs_announce = 0;
            amsys->rs_which = -1;
        }
        user->misc_op = 0;
        prompt(user);
        return 1;

    case 8:
        /* Doing suggestion */
    case 9:
        /* Level specific mail */
        editor(user, inpstr);
        return 1;

    case 10:
        if (tolower(*inpstr) == 'e') {
            user->misc_op = 0;
            user->wrap_room = NULL;
            prompt(user);
        } else {
            rooms(user, 0, 1);
        }
        return 1;

    case 11:
        if (tolower(*inpstr) == 'e') {
            user->misc_op = 0;
            user->wrap_room = NULL;
            prompt(user);
        } else {
            rooms(user, 1, 1);
        }
        return 1;

    case 12:
        if (!*inpstr) {
            write_user(user, "Abandoning your samesite look-up.\n");
            user->misc_op = 0;
            user->samesite_all_store = 0;
            *user->samesite_check_store = '\0';
            prompt(user);
        } else {
            user->misc_op = 0;
            *word[0] = toupper(*word[0]);
            strcpy(user->samesite_check_store, word[0]);
            samesite(user, 1);
        }
        return 1;

    case 13:
        if (!*inpstr) {
            write_user(user, "Abandoning your samesite look-up.\n");
            user->misc_op = 0;
            user->samesite_all_store = 0;
            *user->samesite_check_store = '\0';
            prompt(user);
        } else {
            user->misc_op = 0;
            strcpy(user->samesite_check_store, word[0]);
            samesite(user, 2);
        }
        return 1;

    case 16:
        if (tolower(*inpstr) == 'e' || more_users(user) != 1) {
            user->user_page_pos = 0;
            user->user_page_lev = NUM_LEVELS;
            user->misc_op = 0;
            prompt(user);
        }
        return 1;

    case 17:
        recount_users(user, inpstr);
        prompt(user);
        return 1;

    case 18:
        if (tolower(*inpstr) == 'y') {
            sprintf(filename, "%s/%s/%s.M", USERFILES, USERMAILS, user->name);
            remove(filename);
            write_user(user, "\n~OL~FRAll mail messages deleted.\n\n");
        } else {
            write_user(user, "\nNo mail messages were deleted.\n\n");
        }
        user->misc_op = 0;
        prompt(user);
        return 1;

    case 19:
        /* decorating room */
        editor(user, inpstr);
        return 1;

    case 20:
        show_reminders(user, 1);
        return 1;

    case 21:
        show_reminders(user, 2);
        return 1;

    case 22:
        show_reminders(user, 3);
        return 1;

    case 23:
        *user->reminder[user->reminder_pos].msg = '\0';
        strncat(user->reminder[user->reminder_pos].msg, inpstr, REMINDER_LEN - 1);
        show_reminders(user, 4);
        return 1;

    case 24:
        /* friends mail */
        editor(user, inpstr);
        return 1;

    case 25:
        if (tolower(*inpstr) == 'r')
            rewind_pager(user, 0);
        if (tolower(*inpstr) == 'b')
            rewind_pager(user, 1);
        if (tolower(*inpstr) == 't')
            rewind_pager(user, 2);
        if (tolower(*inpstr) == 'e' || display_pm(user) == -1) {
            end_pager(user);
            user->misc_op = 0;
            prompt(user);
        }
        return 1;

    }
    return 0;
}


/******************************************************************************
 User command functions and their subsids
 *****************************************************************************/

/*** Deal with user input ***/
int
exec_com(UR_OBJECT user, char *inpstr, enum cmd_value defaultcmd)
{
    char filename[80], *comword;
    const struct cmd_entry *com_tab;
    CMD_OBJECT cmd;
    enum cmd_value cmdid;
    size_t len;

    /* FIXME: Should not have to look this up everytime */
    for (cmd = first_command; cmd; cmd = cmd->next) {
        if ((enum cmd_value) cmd->id == defaultcmd) {
            break;
        }
    }
    /* sort out shortcuts and commands */
    inpstr = process_input_string(inpstr, cmd);
    if (!inpstr) {
        return 0;
    }
    comword = *word[0] != '.' ? word[0] : word[0] + 1;
    if (!*comword) {
        write_user(user, "Unknown command.\n");
        return 0;
    }
    len = strlen(comword);
    for (com_tab = command_table; com_tab->name; ++com_tab) {
        if (!strncmp(com_tab->name, comword, len)) {
            break;
        }
    }
    /*
     * You may wonder why I am using command_table[] above and then
     * scrolling through the command linked list again.  This is because
     * many people like to put their commands in a certain order, even
     * though they want them viewed alphabetically.  So that is they type .h
     * they will get help rather than hangman.  Using the commands as they
     * were originally entered (command_table[]) allows you to do this.
     * But to get the number of times the command has been used we still
     * need to increment that command's node, hence scrolling through the
     * linked list.
     *
     * Also have to check the level using the command list nodes because
     * the level of the commands can now be altered, therefore rendering
     * the hard-coded levels from command_table[] is wrong.
     *
     * Then again, you might not be wondering ;)
     */

    cmdid = (enum cmd_value) (com_tab - command_table);
    for (cmd = first_command; cmd; cmd = cmd->next) {
        if ((enum cmd_value) cmd->id == cmdid) {
            break;
        }
    }
    if (user->room) {
        /* If command could not be found, try some soundex matching */
        if (!cmd) {
            char possible[ARR_SIZE], code1[6], code2[6];

            get_soundex(comword, code1);
            *possible = '\0';
            for (com_tab = command_table; com_tab->name; ++com_tab) {
                if (user->level < (enum lvl_value) com_tab->level) {
                    continue;
                }
                get_soundex(com_tab->name, code2);
                if (!strcmp(code1, code2)) {
                    strcat(possible, "  ");
                    strcat(possible, com_tab->name);
                }
            }
            write_user(user, "Unknown command.\n");
            if (strlen(possible)) {
                vwrite_user(user, "Possible suggestions:%s\n", possible);
            }
            return 0;
        }
        if (!has_gcom(user, cmd->id)) {
            if (has_xcom(user, cmd->id)) {
                write_user(user, "You cannot currently use that command.\n");
                return 0;
            }
            if (user->level < cmd->level) {
                write_user(user, "Unknown command.\n");
                return 0;
            }
        }
    }
    if (cmd) {
        ++cmd->count;
        record_last_command(user, cmd, strlen(inpstr));
    }
    /* FIXME: Get rid of this! Needed for some legacy testing for now */
    com_num = cmd ? (enum cmd_value) cmd->id : COUNT;

#ifdef NETLINKS
    /*
     * See if user has gone across a netlink and if so then intercept
     * certain commands to be run on home site
     */
    if (!user->room) {
        switch (cmd ? (enum cmd_value) cmd->id : COUNT) {
        case HOME:
        case QUIT:
        case MODE:
        case PROMPT:
        case COLOUR:
        case SUICIDE:
        case CHARECHO:
            write_user(user, "~FY~OL*** Home execution ***\n");
            break;
        default:
            action_nl(user, word[0], inpstr);
            no_prompt = 1;
            return 1;
            break; /* should not need this */
        }
    }
    /* Dont want certain commands executed by remote users */
    if (user->type == REMOTE_TYPE) {
        switch (cmd->id) {
        case PASSWD:
        case ENTPRO:
        case ACCREQ:
        case CONN:
        case DISCONN:
        case SHUTDOWN:
        case REBOOT:
        case SETAUTOPROMO:
        case DELETE:
        case SET:
        case PURGE:
        case EXPIRE:
        case LOGGING:
            write_user(user, "Sorry, remote users cannot use that command.\n");
            return 0;
            break; /* should not need this */
        default:
            break;
        }
    }
#endif

    /* Main switch */
    switch (com_num) {
    case QUIT:
        disconnect_user(user);
        break;
    case LOOK:
        look(user);
        break;
    case MODE:
        toggle_mode(user);
        break;
    case SAY:
        say(user, inpstr);
        break;
    case SHOUT:
        shout(user, inpstr);
        break;
    case STO:
        sto(user, inpstr);
        break;
    case TELL:
        tell_user(user, inpstr);
        break;
    case EMOTE:
        emote(user, inpstr);
        break;
    case SEMOTE:
        semote(user, inpstr);
        break;
    case PEMOTE:
        pemote(user, inpstr);
        break;
    case ECHO:
        echo(user, inpstr);
        break;
    case GO:
        go(user);
        break;
    case IGNALL:
        toggle_ignall(user);
        break;
    case PROMPT:
        toggle_prompt(user);
        break;
    case DESC:
        set_desc(user, inpstr);
        break;
    case INPHRASE:
    case OUTPHRASE:
        set_iophrase(user, inpstr);
        break;
    case PUBCOM:
        set_room_access(user, 0);
        break;
    case PRIVCOM:
        set_room_access(user, 1);
        break;
    case LETMEIN:
        letmein(user);
        break;
    case INVITE:
        invite(user);
        break;
    case TOPIC:
        set_topic(user, inpstr);
        break;
    case MOVE:
        move(user);
        break;
    case BCAST:
        bcast(user, inpstr, 0);
        break;
    case WHO:
        who(user, 0);
        break;
    case PEOPLE:
        who(user, 2);
        break;
    case HELP:
        help(user);
        break;
    case SHUTDOWN:
        shutdown_com(user);
        break;
    case NEWS:
        sprintf(filename, "%s/%s", MISCFILES, NEWSFILE);
        switch (more(user, user->socket, filename)) {
        case 0:
            write_user(user, "There is no news.\n");
            break;
        case 1:
            user->misc_op = 2;
            break;
        }
        break;
    case READ:
        read_board(user);
        break;
    case WRITE:
        write_board(user, inpstr);
        break;
    case WIPE:
        wipe_board(user);
        break;
    case SEARCH:
        search_boards(user);
        break;
    case REVIEW:
        review(user);
        break;
#ifdef NETLINKS
    case HOME:
        home(user);
        break;
#endif
    case STATUS:
        status(user);
        break;
    case VER:
        show_version(user);
        break;
    case RMAIL:
        rmail(user);
        break;
    case SMAIL:
        smail(user, inpstr);
        break;
    case DMAIL:
        dmail(user);
        break;
    case FROM:
        mail_from(user);
        break;
    case ENTPRO:
        enter_profile(user, inpstr);
        break;
    case EXAMINE:
        examine(user);
        break;
    case RMST:
        rooms(user, 1, 0);
        break;
#ifdef NETLINKS
    case RMSN:
        rooms(user, 0, 0);
        break;
    case NETSTAT:
        netstat(user);
        break;
    case NETDATA:
        netdata(user);
        break;
    case CONN:
        connect_netlink(user);
        break;
    case DISCONN:
        disconnect_netlink(user);
        break;
#endif
    case PASSWD:
        change_pass(user);
        break;
    case KILL:
        kill_user(user);
        break;
    case PROMOTE:
        promote(user);
        break;
    case DEMOTE:
        demote(user);
        break;
    case LISTBANS:
        listbans(user);
        break;
    case BAN:
        ban(user);
        break;
    case UNBAN:
        unban(user);
        break;
    case VIS:
        visibility(user, 1);
        break;
    case INVIS:
        visibility(user, 0);
        break;
    case SITE:
        site(user);
        break;
    case WAKE:
        wake(user);
        break;
    case WIZSHOUT:
        wizshout(user, inpstr);
        break;
    case MUZZLE:
        muzzle(user);
        break;
    case UNMUZZLE:
        unmuzzle(user);
        break;
    case MAP:
        if (!user->room) {
            write_user(user,
                    "You do not need a map--where you are is where it is at!\n");
            return 0;
        }
        sprintf(filename, "%s/%s.map", DATAFILES, user->room->map);
        switch (more(user, user->socket, filename)) {
        case 0:
            write_user(user,
                    "You do not need a map--where you are is where it is at!\n");
            break;
        case 1:
            user->misc_op = 2;
            break;
        }
        break;
    case LOGGING:
        logging(user);
        break;
    case MINLOGIN:
        minlogin(user);
        break;
    case SYSTEM:
        system_details(user);
        break;
    case CHARECHO:
        toggle_charecho(user);
        break;
    case CLEARLINE:
        clearline(user);
        break;
    case FIX:
        change_room_fix(user, 1);
        break;
    case UNFIX:
        change_room_fix(user, 0);
        break;
    case VIEWLOG:
        viewlog(user);
        break;
    case ACCREQ:
        account_request(user, inpstr);
        break;
    case REVCLR:
        revclr(user);
        break;
    case CREATE:
        create_clone(user);
        break;
    case DESTROY:
        destroy_clone(user);
        break;
    case MYCLONES:
        myclones(user);
        break;
    case ALLCLONES:
        allclones(user);
        break;
    case SWITCH:
        clone_switch(user);
        break;
    case CSAY:
        clone_say(user, inpstr);
        break;
    case CHEAR:
        clone_hear(user);
        break;
#ifdef NETLINKS
    case RSTAT:
        remote_stat(user);
        break;
#endif
    case SWBAN:
        toggle_swearban(user);
        break;
    case AFK:
        afk(user, inpstr);
        break;
    case CLS:
        cls(user);
        break;
    case COLOUR:
        display_colour(user);
        break;
    case IGNSHOUTS:
        set_ignore(user);
        break;
    case IGNTELLS:
        set_ignore(user);
        break;
    case SUICIDE:
        suicide(user);
        break;
    case DELETE:
        delete_user(user, 0);
        break;
    case REBOOT:
        reboot_com(user);
        break;
    case RECOUNT:
        check_messages(user, 2);
        break;
    case REVTELL:
        revtell(user);
        break;
    case PURGE:
        purge_users(user);
        break;
    case HISTORY:
        user_history(user);
        break;
    case EXPIRE:
        user_expires(user);
        break;
    case BBCAST:
        bcast(user, inpstr, 1);
        break;
    case SHOW:
        show(user, inpstr);
        break;
    case RANKS:
        show_ranks(user);
        break;
    case WIZLIST:
        wiz_list(user);
        break;
    case TIME:
        get_time(user);
        break;
    case CTOPIC:
        clear_topic(user);
        break;
    case COPYTO:
    case NOCOPIES:
        copies_to(user);
        break;
    case SET:
        set_attributes(user);
        break;
    case MUTTER:
        mutter(user, inpstr);
        break;
    case MKVIS:
        make_vis(user);
        break;
    case MKINVIS:
        make_invis(user);
        break;
    case SOS:
        plead(user, inpstr);
        break;
    case PTELL:
        picture_tell(user);
        break;
    case PREVIEW:
        preview(user);
        break;
    case PICTURE:
        picture_all(user);
        break;
    case GREET:
        greet(user, inpstr);
        break;
    case THINKIT:
        think_it(user, inpstr);
        break;
    case SINGIT:
        sing_it(user, inpstr);
        break;
    case WIZEMOTE:
        wizemote(user, inpstr);
        break;
    case SUG:
    case RSUG:
        suggestions(user, inpstr);
        break;
    case DSUG:
        delete_suggestions(user);
        break;
    case LAST:
        show_last_login(user);
        break;
    case MACROS:
        macros(user);
        break;
    case RULES:
        sprintf(filename, "%s/%s", MISCFILES, RULESFILE);
        switch (more(user, user->socket, filename)) {
        case 0:
            write_user(user, "\nThere are currrently no rules...\n");
            break;
        case 1:
            user->misc_op = 2;
            break;
        }
        break;
    case UNINVITE:
        uninvite(user);
        break;
    case LMAIL:
        level_mail(user, inpstr);
        break;
    case ARREST:
        arrest(user);
        break;
    case UNARREST:
        unarrest(user);
        break;
    case VERIFY:
        verify_email(user);
        break;
    case ADDHISTORY:
        manual_history(user, inpstr);
        break;
    case FORWARDING:
        amsys->forwarding = !amsys->forwarding;
        if (amsys->forwarding) {
            write_user(user, "You have turned ~FGon~RS smail auto-forwarding.\n");
            write_syslog(SYSLOG, 1, "%s turned ON mail forwarding.\n", user->name);
        } else {
            write_user(user, "You have turned ~FRoff~RS smail auto-forwarding.\n");
            write_syslog(SYSLOG, 1, "%s turned OFF mail forwarding.\n", user->name);
        }
        break;
    case REVSHOUT:
        revshout(user);
        break;
    case CSHOUT:
        clear_shouts();
        write_user(user, "Shouts buffer has now been cleared.\n");
        break;
    case CTELLS:
        clear_tells(user);
        write_user(user, "Your tells have now been cleared.\n");
        break;
    case MONITOR:
        user->monitor = !user->monitor;
        if (user->monitor) {
            write_user(user, "You will now monitor certain things.\n");
        } else {
            write_user(user, "You will no longer monitor certain things.\n");
        }
        break;
    case QCALL:
        quick_call(user);
        break;
    case UNQCALL:
        *user->call = '\0';
        write_user(user, "You no longer have your quick call set.\n");
        break;
    case IGNUSER:
        set_igusers(user);
        break;
    case IGNPICS:
        set_ignore(user);
        break;
    case IGNWIZ:
        set_ignore(user);
        break;
    case IGNLOGONS:
        set_ignore(user);
        break;
    case IGNGREETS:
        set_ignore(user);
        break;
    case IGNBEEPS:
        set_ignore(user);
        break;
    case IGNLIST:
        show_ignlist(user);
        break;
    case ACCOUNT:
        create_account(user);
        break;
    case SAMESITE:
        samesite(user, 0);
        break;
    case BFROM:
        board_from(user);
        break;
    case SAVEALL:
        force_save(user);
        break;
    case JOIN:
        join(user);
        break;
    case SHACKLE:
        shackle(user);
        break;
    case UNSHACKLE:
        unshackle(user);
        break;
    case REVAFK:
        revafk(user);
        break;
    case CAFK:
        clear_afk(user);
        break;
    case REVEDIT:
        revedit(user);
        break;
    case CEDIT:
        clear_edit(user);
        write_user(user, "Your EDIT review buffer has now been cleared.\n");
        break;
    case CEMOTE:
        clone_emote(user, inpstr);
        break;
    case LISTEN:
        user_listen(user);
        break;
    case RETIRE:
        retire_user(user);
        break;
    case UNRETIRE:
        unretire_user(user);
        break;
    case CMDCOUNT:
        show_command_counts(user);
        break;
    case RCOUNTU:
        recount_users(user, inpstr);
        break;
    case RECAPS:
        amsys->allow_recaps = !amsys->allow_recaps;
        if (amsys->allow_recaps) {
            write_user(user, "You ~FGallow~RS names to be recapped.\n");
            write_syslog(SYSLOG, 1, "%s turned ON recapping of names.\n",
                    user->name);
        } else {
            write_user(user, "You ~FRdisallow~RS names to be recapped.\n");
            write_syslog(SYSLOG, 1, "%s turned OFF recapping of names.\n",
                    user->name);
        }
        break;
    case SETCMDLEV:
        set_command_level(user);
        break;
    case GREPUSER:
        grep_users(user);
        break;
    case XCOM:
        user_xcom(user);
        break;
    case GCOM:
        user_gcom(user);
        break;
    case SFROM:
        suggestions_from(user);
        break;
    case RLOADRM:
        reload_room_description(user);
        break;
    case SETAUTOPROMO:
        amsys->auto_promote = !amsys->auto_promote;
        if (amsys->auto_promote) {
            write_user(user,
                    "You have turned ~FGon~RS auto-promotes for new users.\n");
            write_syslog(SYSLOG, 1, "%s turned ON auto-promotes.\n", user->name);
        } else {
            write_user(user,
                    "You have turned ~FRoff~RS auto-promotes for new users.\n");
            write_syslog(SYSLOG, 1, "%s turned OFF auto-promotes.\n", user->name);
        }
        break;
    case SAYTO:
        say_to(user, inpstr);
        break;
    case FRIENDS:
        friends(user);
        break;
    case FSAY:
        friend_say(user, inpstr);
        break;
    case FEMOTE:
        friend_emote(user, inpstr);
        break;
    case BRING:
        bring(user);
        break;
    case FORCE:
        force(user, inpstr);
        break;
    case CALENDAR:
        show_calendar(user);
        break;
    case FWHO:
        who(user, 1);
        break;
    case MYROOM:
        personal_room(user);
        break;
    case MYLOCK:
        personal_room_lock(user);
        break;
    case VISIT:
        personal_room_visit(user);
        break;
    case MYPAINT:
        personal_room_decorate(user, inpstr);
        break;
    case MYNAME:
        personal_room_rename(user, inpstr);
        break;
    case BEEP:
        beep(user, inpstr);
        break;
    case RMADMIN:
        personal_room_admin(user);
        break;
    case MYKEY:
        personal_room_key(user);
        break;
    case MYBGONE:
        personal_room_bgone(user);
        break;
    case WIZRULES:
        sprintf(filename, "%s/%s", MISCFILES, WIZRULESFILE);
        switch (more(user, user->socket, filename)) {
        case 0:
            write_user(user, "\nThere are currrently no admin rules...\n");
            break;
        case 1:
            user->misc_op = 2;
            break;
        }
        break;
    case DISPLAY:
        display_files(user, 0);
        break;
    case DISPLAYADMIN:
        display_files(user, 1);
        break;
    case DUMPCMD:
        dump_to_file(user);
        break;
    case TEMPRO:
        temporary_promote(user);
        break;
    case MORPH:
        change_user_name(user);
        break;
    case FMAIL:
        forward_specific_mail(user);
        break;
    case REMINDER:
        show_reminders(user, 0);
        break;
    case FSMAIL:
        friend_smail(user, inpstr);
        break;
    case SREBOOT:
        sreboot_com(user);
        break;
#ifdef IDENTD
    case RESITE:
        resite(user);
        break;
#endif
#ifdef GAMES
    case HANGMAN:
        play_hangman(user);
        break;
    case GUESS:
        guess_hangman(user);
        break;
    case SHOOT:
        shoot_user(user);
        break;
    case RELOAD:
        reload_gun(user);
        break;
    case DONATE:
        donate_cash(user);
        break;
    case CASH:
        show_money(user);
        break;
    case MONEY:
        global_money(user);
        break;
    case BANK:
        bank_money(user);
        break;
#endif
    case FLAGGED:
        show_flagged_users(user);
        break;
    case SPODLIST:
        show_spodlist(user);
        break;
    default:
        write_user(user, "Command not executed.\n");
        break;
    }
    return 1;
}

/*
 * Display a short, non-colour version for the .who for those looking at it from the
 * login prompt.  Thanks to Xan, Arny and Squirt for this idea (even though the code is mine ;)
 */
void
login_who(UR_OBJECT user)
{
    sds userText, lineText;
    UR_OBJECT u;
    int invis, on, len;

    write_user(user,
            "\n+----------------------------------------------------------------------------+\n");
    write_user(user,
            align_string(1, 78, 0, NULL, "Current users %s", long_date(1)));
    write_user(user,
            "+----------------------------------------------------------------------------+\n\n");

    invis = on = 0;
    lineText = sdsempty();

    for (u = user_first; u; u = u->next) {
        if (u->login || u->type == CLONE_TYPE) {
            continue;
        }
        if (!u->vis) {
            ++invis;
            continue;
        }
        userText = sdscatfmt(sdsempty(), "%s %s", u->bw_recap, (u->afk ? "<AFK> " : u->malloc_start ? "<EDIT>" : "      "));
        if ((len = ((USER_NAME_LEN + 7) - (int)sdslen(userText)))) {
            userText = sdscat(userText, repeat_string(" ", len));
        }
        lineText = sdscat(lineText, userText);
        if (!(++on % 4)) {
            lineText = sdscat(lineText, "\n");
            write_user(user, lineText);
            lineText = sdsempty();
        }
    }
    if (on % 4) {
        lineText = sdscat(lineText, "\n");
        write_user(user, lineText);
    }
    if (!(on + invis)) {
        write_user(user,
                align_string(1, 78, 0, NULL,
                "No users are currently logged on\n"));
    } else {
        write_user(user, "\n");
        write_user(user,
                align_string(1, 78, 0, NULL,
                "%d user%s logged on, %d %s invis", on + invis,
                PLTEXT_S(on + invis), invis, PLTEXT_IS(invis)));
    }
    write_user(user,
            "+----------------------------------------------------------------------------+\n");
    sdsfree(lineText);
    sdsfree(userText);
}

/*
 * Show the command available listed by level
 */
void
help_commands_level(UR_OBJECT user)
{
    int cnt, total, highlight;
    enum lvl_value lvl;
    sds temp, temp1;
    CMD_OBJECT cmd;

    start_pager(user);
    write_user(user,
            "\n+----------------------------------------------------------------------------+\n");
    write_user(user,
            "| All commands start with a \".\" (when in ~FYspeech~RS mode) and can be abbreviated |\n");
    write_user(user,
            "| Remember, a \".\" by itself will repeat your last command or speech          |\n");
    write_user(user,
            "+----------------------------------------------------------------------------+\n");
    write_user(user,
            align_string(1, 78, 1, "|",
            "  Commands available to you (level ~OL%s~RS) ",
            user_level[user->level].name));
    write_user(user,
            "+----------------------------------------------------------------------------+\n");
    total = 0;
    for (lvl = JAILED; lvl < NUM_LEVELS; lvl = (enum lvl_value) (lvl + 1)) {
        if (user->level < lvl) {
            break;
        }
        cnt = 0;
        *text = '\0';
        sprintf(text, "  ~FG~OL%-1.1s)~RS ~FC", user_level[lvl].name);
        highlight = 1;
        /* scroll through all commands, format and print */
        for (cmd = first_command; cmd; cmd = cmd->next) {
            temp1 = sdsempty();
            if (cmd->level != lvl) {
                continue;
            }
            if (has_xcom(user, cmd->id)) {
                temp1 = sdscatfmt(sdsempty(), "~FR%s~RS%s %s", cmd->name, highlight ? "~FC" : "",
                        cmd->alias);
            } else {
                temp1 = sdscatfmt(sdsempty(), "%s %s", cmd->name, cmd->alias);
            }
            if (++cnt == 5) {
                strcat(text, temp1);
                strcat(text, "~RS");
                write_user(user, align_string(0, 78, 1, "|", "%s", text));
                cnt = 0;
                highlight = 0;
                *text = '\0';
            } else {
                temp = sdscatprintf(sdsempty(), "%-*s  ", 11 + (int) teslen(temp1, 0), temp1);
                strcat(text, temp);
            }
            if (!cnt) {
                strcat(text, "     ");
            }
        }
        if (cnt > 0 && cnt < 5)
            write_user(user, align_string(0, 78, 1, "|", "%s", text));
    }
    /* count up total number of commands for user level */
    for (cmd = first_command; cmd; cmd = cmd->next) {
        if (cmd->level > user->level) {
            continue;
        }
        ++total;
    }
    write_user(user,
            "+----------------------------------------------------------------------------+\n");
    write_user(user,
            align_string(0, 78, 1, "|",
            "  There is a total of ~OL%d~RS command%s that you can use ",
            total, PLTEXT_S(total)));
    write_user(user,
            "+----------------------------------------------------------------------------+\n");
    stop_pager(user);
    sdsfree(temp);
    sdsfree(temp1);
}

/*
 * Show the command available listed by function
 */
void
help_commands_function(UR_OBJECT user)
{
    sds temp, temp1;
    CMD_OBJECT cmd;
    int cnt, total, function, found;

    start_pager(user);
    write_user(user,
            "\n+----------------------------------------------------------------------------+\n");
    write_user(user,
            "| All commands start with a \".\" (when in ~FYspeech~RS mode) and can be abbreviated |\n");
    write_user(user,
            "| Remember, a \".\" by itself will repeat your last command or speech          |\n");
    write_user(user,
            "+----------------------------------------------------------------------------+\n");
    write_user(user,
            align_string(1, 78, 1, "|",
            "  Commands available to you (level ~OL%s~RS) ",
            user_level[user->level].name));
    write_user(user,
            "+----------------------------------------------------------------------------+\n");
    /* scroll through all the commands listing by function */
    total = 0;
    for (function = 0; command_types[function]; ++function) {
        cnt = 0;
        found = 0;
        *text = '\0';
        /* scroll through all commands, format and print */
        for (cmd = first_command; cmd; cmd = cmd->next) {
            temp1 = sdsempty();
            if (cmd->level > user->level || cmd->function != function) {
                continue;
            }
            if (!found++) {
                write_user(user,
                        align_string(0, 78, 1, "|", "  ~OL~FG%s~RS ",
                        command_types[function]));
                strcpy(text, "     ");
            }
            if (has_xcom(user, cmd->id)) {
                temp1 = sdscatfmt(sdsempty(), "~FR%s~RS %s", cmd->name, cmd->alias);
            } else {
                temp1 = sdscatfmt(sdsempty(), "%s %s", cmd->name, cmd->alias);
            }
            if (++cnt == 5) {
                strcat(text, temp1);
                strcat(text, "~RS");
                write_user(user, align_string(0, 78, 1, "|", "%s", text));
                cnt = 0;
                *text = '\0';
            } else {
                temp = sdscatprintf(sdsempty(), "%-*s  ", 11 + (int) teslen(temp1, 0), temp1);
                strcat(text, temp);
            }
            if (!cnt) {
                strcat(text, "     ");
            }
        }
        if (cnt > 0 && cnt < 5)
            write_user(user, align_string(0, 78, 1, "|", "%s", text));
    }
    /* count up total number of commands for user level */
    for (cmd = first_command; cmd; cmd = cmd->next) {
        if (cmd->level > user->level) {
            continue;
        }
        ++total;
    }
    write_user(user,
            "+----------------------------------------------------------------------------+\n");
    write_user(user,
            align_string(0, 78, 1, "|",
            "  There is a total of ~OL%d~RS command%s that you can use ",
            total, PLTEXT_S(total)));
    write_user(user,
            "+----------------------------------------------------------------------------+\n");
    stop_pager(user);
    sdsfree(temp);
    sdsfree(temp1);
}

/*
 * Show NUTS credits
 */
void
help_nuts_credits(UR_OBJECT user)
{
    write_user(user,
            "\n~BB*** NUTS Credits :) (for Amnuts credits, see \".help credits\") ***\n\n");
    vwrite_user(user,
            "~BRNUTS version %s, Copyright (C) Neil Robertson 1996.\n\n",
            NUTSVER);
    write_user(user,
            "~BM             ~BB             ~BC             ~BG             ~BY             ~BR             \n");
    write_user(user,
            "NUTS stands for Neil's Unix Talk Server, a program which started out as a\n");
    write_user(user,
            "university project in autumn 1992 and has progressed from thereon. In no\n");
    write_user(user,
            "particular order thanks go to the following people who helped me develop or\n");
    write_user(user, "debug this code in one way or another over the years:\n");
    write_user(user,
            "   ~FCDarren Seryck, Steve Guest, Dave Temple, Satish Bedi, Tim Bernhardt,\n");
    write_user(user,
            "   ~FCKien Tran, Jesse Walton, Pak Chan, Scott MacKenzie and Bryan McPhail.\n");
    write_user(user,
            "Also thanks must go to anyone else who has emailed me with ideas and/or bug\n");
    write_user(user,
            "reports and all the people who have used NUTS over the intervening years.\n");
    write_user(user,
            "I know I have said this before but this time I really mean it--this is the final\n");
    write_user(user,
            "version of NUTS 3. In a few years NUTS 4 may spring forth but in the meantime\n");
    write_user(user, "that, as they say, is that. :)\n\n");
    write_user(user,
            "If you wish to email me my address is \"~FGneil@ogham.demon.co.uk~RS\" and should\n");
    write_user(user,
            "remain so for the forseeable future.\n\nNeil Robertson - November 1996.\n");
    write_user(user,
            "~BM             ~BB             ~BC             ~BG             ~BY             ~BR             \n\n");
}

/*
 * Show the credits. Add your own credits here if you wish but PLEASE leave
 * my credits intact. Thanks.
 */
void
help_amnuts_credits(UR_OBJECT user)
{
    write_user(user,
            "~BM             ~BB             ~BC             ~BG             ~BY             ~BR             \n\n");
    vwrite_user(user,
            "~OL~FCAmnuts version %s~RS, Copyright (C) Andrew Collington, 2003\n",
            AMNUTSVER);
    write_user(user,
            "Brought to you by the Amnuts Development Group (Andy, Ardant and Uzume)\n\n");
    write_user(user,
            "Amnuts stands for ~OLA~RSndy's ~OLM~RSodified ~OLNUTS~RS, a Unix talker server written in C.\n\n");
    write_user(user,
            "Many thanks to everyone who has helped out with Amnuts.  Special thanks go to\n");
    write_user(user,
            "Ardant, Uzume, Arny (of Paris fame), Silver (of PG+ fame), and anyone else who\n");
    write_user(user,
            "has contributed at all to the development of Amnuts.\n\n");
    write_user(user,
            "If you are interested, you can purchase Amnuts t-shirts, mugs, mousemats, and\n");
    write_user(user,
            "more, from http://www.cafepress.com/amnuts/\n\nWe hope you enjoy the talker!\n\n");
    write_user(user,
            "   -- The Amnuts Development Group\n\n(for NUTS credits, see \".help nuts\")\n");
    write_user(user,
            "\n~BM             ~BB             ~BC             ~BG             ~BY             ~BR             \n\n");
}

void
show_attributes(UR_OBJECT user)
{
    static const char *const onoff[] = {"Off", "On"};
    static const char *const shide[] = {"Showing", "Hidden"};
    static const char *const rm[] = {"Main room", "Last room in"};
    static const char *const cmd[] = {"Level", "Function"};
    static const char *const revbuf[] = {"Normal", "Reversed"};
    int i, cnt;

    write_user(user,
            "+----------------------------------------------------------------------------+\n");
    write_user(user,
            "| ~FC~OLStatus of your set attributes~RS                                              |\n");
    write_user(user,
            "+----------------------------------------------------------------------------+\n");
    for (i = 0; setstr[i].type; ++i) {
        *text = '\0';
        switch (i) {
        case SETGEND:
            vwrite_user(user, "| %-10.10s : ~OL%-61.61s~RS |\n", setstr[i].type,
                    sex[user->gender]);
            break;
        case SETAGE:
            if (!user->age) {
                sprintf(text, "unset");
            } else {
                sprintf(text, "~OL%d~RS", user->age);
            }
            cnt = 61 + teslen(text, 61);
            vwrite_user(user, "| %-10.10s : %-*.*s |\n", setstr[i].type, cnt, cnt,
                    text);
            break;
        case SETWRAP:
            vwrite_user(user, "| %-10.10s : ~OL%-61.61s~RS |\n", setstr[i].type,
                    onoff[user->wrap]);
            break;
        case SETEMAIL:
            if (!*user->email) {
                strcpy(text, "unset");
            } else {
                if (user->mail_verified) {
                    sprintf(text, "~OL%s~RS - verified", user->email);
                } else {
                    sprintf(text, "~OL%s~RS - not yet verified", user->email);
                }
            }
            cnt = 61 + teslen(text, 61);
            vwrite_user(user, "| %-10.10s : %-*.*s |\n", setstr[i].type, cnt, cnt,
                    text);
            break;
        case SETHOMEP:
            if (!*user->homepage) {
                strcpy(text, "unset");
            } else {
                sprintf(text, "~OL%s~RS", user->homepage);
            }
            cnt = 61 + teslen(text, 61);
            vwrite_user(user, "| %-10.10s : %-*.*s |\n", setstr[i].type, cnt, cnt,
                    text);
            break;
        case SETHIDEEMAIL:
            vwrite_user(user, "| %-10.10s : ~OL%-61.61s~RS |\n", setstr[i].type,
                    shide[user->hideemail]);
            break;
        case SETCOLOUR:
            vwrite_user(user, "| %-10.10s : ~FC~OL%-61.61s~RS |\n", setstr[i].type,
                    onoff[user->colour]);
            break;
        case SETPAGER:
            sprintf(text, "%d", user->pager);
            vwrite_user(user, "| %-10.10s : ~OL%-61.61s~RS |\n", setstr[i].type,
                    text);
            break;
        case SETROOM:
            vwrite_user(user, "| %-10.10s : ~OL%-61.61s~RS |\n", setstr[i].type,
                    rm[user->lroom]);
            break;
        case SETFWD:
            vwrite_user(user, "| %-10.10s : ~OL%-61.61s~RS |\n", setstr[i].type,
                    onoff[user->autofwd]);
            break;
        case SETPASSWD:
            vwrite_user(user, "| %-10.10s : ~OL%-61.61s~RS |\n", setstr[i].type,
                    onoff[user->show_pass]);
            break;
        case SETRDESC:
            vwrite_user(user, "| %-10.10s : ~OL%-61.61s~RS |\n", setstr[i].type,
                    onoff[user->show_rdesc]);
            break;
        case SETCOMMAND:
            vwrite_user(user, "| %-10.10s : ~OL%-61.61s~RS |\n", setstr[i].type,
                    cmd[user->cmd_type]);
            break;
        case SETRECAP:
            cnt = 61 + teslen(user->recap, 61);
            vwrite_user(user, "| %-10.10s : %-*.*s~RS |\n", setstr[i].type, cnt,
                    cnt, user->recap);
            break;
        case SETICQ:
            if (!*user->icq) {
                sprintf(text, "unset");
            } else {
                sprintf(text, "~OL%s~RS", user->icq);
            }
            cnt = 61 + teslen(text, 61);
            vwrite_user(user, "| %-10.10s : %-*.*s |\n", setstr[i].type, cnt, cnt,
                    text);
            break;
        case SETALERT:
            vwrite_user(user, "| %-10.10s : ~OL%-61.61s~RS |\n", setstr[i].type,
                    onoff[user->alert]);
            break;
        case SETREVBUF:
            vwrite_user(user, "| %-10.10s : ~OL%-61.61s~RS |\n", setstr[i].type,
                    revbuf[user->reverse_buffer]);
            break;
        }
    }
    write_user(user,
            "+----------------------------------------------------------------------------+\n");
    return;
}

/*
 * User prompt
 */
void
prompt(UR_OBJECT user)
{
    char dstr[32];
    time_t now;
    int hr, min, ign;

    if (no_prompt) {
        return;
    }
#ifdef NETLINKS
    if (prompt_nl(user)) {
        return;
    }
#endif
    ign = 0;
    if (user->ignall) {
        ++ign;
    }
    if (user->igntells) {
        ++ign;
    }
    if (user->ignshouts) {
        ++ign;
    }
    if (user->ignpics) {
        ++ign;
    }
    if (user->ignlogons) {
        ++ign;
    }
    if (user->ignwiz) {
        ++ign;
    }
    if (user->igngreets) {
        ++ign;
    }
    if (user->ignbeeps) {
        ++ign;
    }
    if (user->command_mode && !user->misc_op) {
        vwrite_user(user, "~FCCOM%s%s> ", !user->vis ? "+" : "",
                ign > 0 ? "!" : "");
        return;
    }
    if (!user->prompt || user->misc_op) {
        return;
    }
    time(&now);
    hr = (int) (now - user->last_login) / 3600;
    min = ((int) (now - user->last_login) % 3600) / 60;
    strftime(dstr, 32, "%H:%M", localtime(&now));
    vwrite_user(user, "~FC<%s, %.2d:%.2d, %s%s%s>\n", dstr, hr, min,
            user->bw_recap, !user->vis ? "+" : "", ign > 0 ? "!" : "");
}

/*
 * Get user macros
 */
void
get_macros(UR_OBJECT user)
{
    char filename[80];
    FILE *fp;
    int i, l;

#ifdef NETLINKS
    if (user->type == REMOTE_TYPE) {
        return;
    }
#endif
    sprintf(filename, "%s/%s/%s.MAC", USERFILES, USERMACROS, user->name);

    fp = fopen(filename, "r");
    if (!fp) {
        return;
    }
    for (i = 0; i < 10; ++i) {
        fgets(user->macros[i], MACRO_LEN, fp);
        l = strlen(user->macros[i]);
        user->macros[i][l - 1] = '\0';
    }
    fclose(fp);
}

/*
 * See if command just executed by the user was a macro
 */
void
check_macros(UR_OBJECT user, char *inpstr)
{
    size_t macnum;

    if (*inpstr != '.' || !isdigit(inpstr[1])) {
        return;
    }
#ifdef NETLINKS
    if (user->type == REMOTE_TYPE) {
        write_user(user, "Remote users cannot use macros at this time.\n");
        return;
    }
#endif
    macnum = inpstr[1] - '0';
    if (inpstr[2] != '=') {
        char line[ARR_SIZE];

        *line = '\0';
        strncat(line, inpstr + 2, ARR_SIZE - 1);
        /* FIXME: Bounds checking */
        strcpy(inpstr, user->macros[macnum]);
        strcat(inpstr, line);
    } else {
        char filename[80];
        FILE *fp;

        if (strlen(inpstr + 3) >= MACRO_LEN) {
            write_user(user, "That macro length was too long.\n");
            *inpstr = '\0';
            return;
        }
        *user->macros[macnum] = '\0';
        strncat(user->macros[macnum], inpstr + 3, MACRO_LEN - 1);
        *inpstr = '\0';
        sprintf(filename, "%s/%s/%s.MAC", USERFILES, USERMACROS, user->name);
        fp = fopen(filename, "w");
        if (!fp) {
            write_user(user, "Your macro file could not be accessed.\n");
            write_syslog(SYSLOG, 1, "%s could not access macros file.\n",
                    user->name);
            return;
        }
        for (macnum = 0; macnum < 10; ++macnum) {
            fprintf(fp, "%s\n", user->macros[macnum]);
        }
        fclose(fp);
        write_user(user, "You have now set your macro.\n");
    }
}

/*
 * Set list of users that you ignore
 */
void
show_igusers(UR_OBJECT user)
{
    char text2[ARR_SIZE];
    FU_OBJECT fu;
    int found = 0, cnt = 0;

    *text2 = '\0';
    for (fu = user->fu_first; fu; fu = fu->next) {
        if (fu->flags & fufIGNORE) {
            if (!found++) {
                write_user(user,
                        "+----------------------------------------------------------------------------+\n");
                write_user(user,
                        "| ~OL~FCYou are currently ignoring the following people~RS                            |\n");
                write_user(user,
                        "+----------------------------------------------------------------------------+\n");
            }
            switch (++cnt) {
            case 1:
                sprintf(text, "| %-24s", fu->name);
                strcat(text2, text);
                break;
            case 2:
                sprintf(text, " %-24s", fu->name);
                strcat(text2, text);
                break;
            default:
                sprintf(text, " %-24s |\n", fu->name);
                strcat(text2, text);
                write_user(user, text2);
                cnt = 0;
                *text2 = '\0';
                break;
            }
        }
    }
    if (!found) {
        write_user(user, "You are not ignoring any users.\n");
        return;
    }
    if (cnt == 1) {
        strcat(text2, "                                                   |\n");
        write_user(user, text2);
    } else if (cnt == 2) {
        strcat(text2, "                          |\n");
        write_user(user, text2);
    }
    write_user(user,
            "+----------------------------------------------------------------------------+\n");
}

/*
 * check to see if user is ignoring person with the ignoring->name
 */
int
check_igusers(UR_OBJECT user, UR_OBJECT ignoring)
{
    FU_OBJECT fu;

    if (!user || !ignoring) {
        return 0;
    }
    for (fu = user->fu_first; fu; fu = fu->next) {
        if (!strcasecmp(fu->name, ignoring->name)) {
            break;
        }
    }
    return fu && (fu->flags & fufIGNORE);
}

/*****************************************************************************
 Friend commands and their subsids
 *****************************************************************************/


int
count_friends(UR_OBJECT user)
{
    FU_OBJECT fu;
    int count;

    count = 0;
    for (fu = user->fu_first; fu; fu = fu->next) {
        if (fu->flags & fufFRIEND) {
            ++count;
        }
    }
    return count;
}

/*
 * Determine whether user has u listed on their friends list
 */
int
user_is_friend(UR_OBJECT user, UR_OBJECT u)
{
    FU_OBJECT fu;

    for (fu = user->fu_first; fu; fu = fu->next) {
        if (!strcasecmp(fu->name, u->name)) {
            break;
        }
    }
    return fu && (fu->flags & fufFRIEND);
}

/*
 * Alert anyone logged on who has user in their friends
 * list that the user has just loged on
 */
void
alert_friends(UR_OBJECT user)
{
    UR_OBJECT u;

    for (u = user_first; u; u = u->next) {
        if (!u->alert || u->login) {
            continue;
        }
        if (user_is_friend(u, user) && user->vis) {
            vwrite_user(u,
                    "\n\07~FG~OL~LIHEY!~RS~OL~FG  Your friend ~FC%s~FG has just logged on\n\n",
                    user->name);
        }
    }
}





/*****************************************************************************
              based upon scalar date routines by Ray Gardner
                and CAL - a calendar for DOS by Bob Stout
 *****************************************************************************/

/*
 * determine if year is a leap year
 */
int
is_leap(int yr)
{
    return !(yr % 400) || (!(yr % 4) && (yr % 100));
}

/*
 * convert months to days
 */
long
months_to_days(int mn)
{
    return (mn * 3057 - 3007) / 100;
}

/*
 * convert years to days
 */
long
years_to_days(int yr)
{
    return yr * 365L + yr / 4 - yr / 100 + yr / 400;
}

/*
 * convert a given date (y/m/d) to a scalar
 */
long
ymd_to_scalar(int yr, int mo, int dy)
{
    long scalar;

    scalar = dy + months_to_days(mo);
    /* adjust if past February */
    if (mo > 2) {
        scalar -= is_leap(yr) ? 1 : 2;
    }
    --yr;
    scalar += years_to_days(yr);
    return scalar;
}

/*
 * converts a scalar date to y/m/d
 */
void
scalar_to_ymd(long scalar, int *yr, int *mo, int *dy)
{
    long n;

    /* 146097 == years_to_days(400) */
    for (*yr = (scalar * 400L) / 146097; years_to_days(*yr) < scalar;
            ++(*yr)) {
        ;
    }
    n = scalar - years_to_days(*yr - 1);
    /* adjust if past February */
    if (n > 59) {
        n += 2;
        if (is_leap(*yr)) {
            n -= n > 62 ? 1 : 2;
        }
    }
    /* inverse of months_to_days() */
    *mo = (n * 100 + 3007) / 3057;
    *dy = n - months_to_days(*mo);
}

/*
 * determine if the y/m/d given is todays date
 */
int
is_ymd_today(int yr, int mo, int dy)
{
    const struct tm *date;
    time_t now;

    time(&now);
    date = localtime(&now);
    return yr == 1900 + date->tm_year && mo == 1 + date->tm_mon
            && dy == date->tm_mday;
}

/*
 * check to see if a user has a reminder for a given date
 */
int
has_reminder(UR_OBJECT user, int dy, int mo, int yr)
{
    long date;
    int i, cnt;

    date = ymd_to_scalar(yr, mo, dy);
    cnt = 0;
    for (i = 0; i < MAX_REMINDERS; ++i) {
        if (!*user->reminder[i].msg) {
            continue;
        }
        if (user->reminder[i].date == date) {
            ++cnt;
        }
    }
    return cnt;
}

/*
 * check to see if a user has a reminder for today
 */
int
has_reminder_today(UR_OBJECT user)
{
    const struct tm *date;
    time_t now;

    time(&now);
    date = localtime(&now);
    return has_reminder(user, date->tm_mday, 1 + date->tm_mon,
            1900 + date->tm_year);
}

/*
 * cleans any reminders that have expired - no point keeping them!
 */
int
remove_old_reminders(UR_OBJECT user)
{
    const struct tm *date;
    time_t now;
    long today;
    int count, i;

    time(&now);
    date = localtime(&now);
    today =
            ymd_to_scalar(1900 + date->tm_year, 1 + date->tm_mon, date->tm_mday);
    count = 0;
    for (i = 0; i < MAX_REMINDERS; ++i) {
        if (!*user->reminder[i].msg) {
            continue;
        }
        if (user->reminder[i].date < today) {
            user->reminder[i].date = -1;
            *user->reminder[i].msg = '\0';
            ++count;
        }
    }
    if (count) {
        write_user_reminders(user);
    }
    return count;
}

/*
 * read in user reminder file
 */
int
read_user_reminders(UR_OBJECT user)
{
    char filename[80], line[REMINDER_LEN + 1], *s;
    FILE *fp;
    int dd, mm, yy, alert;
    int ln, i;

    sprintf(filename, "%s/%s/%s.REM", USERFILES, USERREMINDERS, user->name);
    fp = fopen(filename, "r");
    if (!fp) {
        return 0;
    }
    i = 0;
    ln = 0;
    for (s = fgets(line, REMINDER_LEN, fp); s;
            s = fgets(line, REMINDER_LEN, fp)) {
        if (i > MAX_REMINDERS) {
            break;
        }
        if (!ln) {
            sscanf(s, "%d %d %d %d\n", &dd, &mm, &yy, &alert);
            user->reminder[i].date = ymd_to_scalar(yy, mm, dd);
            ln = 1;
        } else {
            s[strlen(s) - 1] = '\0';
            strcpy(user->reminder[i].msg, s);
            ++i;
            ln = 0;
        }
    }
    fclose(fp);
    return i;
}

/*
 * store user reminders
 */
int
write_user_reminders(UR_OBJECT user)
{
    char filename[80];
    FILE *fp;
    int dd, mm, yy, alert = 0;
    int i, cnt;

    sprintf(filename, "%s/%s/%s.REM", USERFILES, USERREMINDERS, user->name);
    fp = fopen(filename, "w");
    if (!fp) {
        write_syslog(SYSLOG | ERRLOG, 0,
                "ERROR: Could not open %s reminder file for writing in write_reminders()\n",
                user->name);
        return 0;
    }
    cnt = 0;
    for (i = 0; i < MAX_REMINDERS; ++i) {
        if (!*user->reminder[i].msg) {
            continue;
        }
        scalar_to_ymd(user->reminder[i].date, &yy, &mm, &dd);
        fprintf(fp, "%d %d %d %d\n", dd, mm, yy, alert);
        fputs(user->reminder[i].msg, fp);
        fprintf(fp, "\n");
        ++cnt;
    }
    fclose(fp);
    if (!cnt) {
        remove(filename);
    }
    return 1;
}




/*****************************************************************************/
