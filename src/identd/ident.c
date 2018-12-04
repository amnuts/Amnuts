/************************************************
 * ArIEN - Ar Interactive Entertainment Network *
 * Ident Daemon v2.0.2                          *
 * -------------------------------------------- *
 ** Copyright (c) 1999 by Gordon Chiu (Ardant) **
 ** http://seashell.dune.net     dune.net:1998 **
 * -------------------------------------------- *
 * ArIdent Daemon v2.0.2                        *
 * Daemon that performs reverse lookups without *
 * lagging or hanging the talker. Now does auth *
 * lookups as well. Sockets used to communicate *
 * with the main talker.                        *
 * ---------------------------------------------*
 * double_fork() function from Amnuts           *
 * host() function by SquirT                    *
 * connect and wordfind() functions from NUTS   *
 * ---------------------------------------------*
 * All Rights Reserved. ArIEN SIG:Ar6m9EhsGPQvQ *
 ************************************************/

/*** This software is distributed under the
     terms of the GPL license.                ***/

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>

#define HOSTBIN    "/usr/bin/host"
#define TALKERNAME "Amnuts"
#define SERVER     "localhost"
#define HOSTPORT   "2402"
#define USERNAME   "identduser"
#define PASSWORD   "identdpass"
#define MAX_WORDS 10
#define WORD_LEN 150

/* Function Prototypes */
int main(int argc, char **argv);
static int socket_connect(const char *site, const char *serv);
static void authenticate_host(int host_socket);
static const char *get_proc(const char *site);
static int double_fork(void);
static int wordfind(const char *inpstr,
        char word[MAX_WORDS + 1][WORD_LEN + 1]);

int
main(int argc, char **argv)
{
    char inpstr[2000], authstr[2000];
    char buffer[1000], *miasma;
    char *next, *current;
    int len, i, ret;
    int host_socket;
    fd_set readmask;
    pid_t ident_pid;

    printf("*** ArIdent Daemon Version 2.0.2\n*** Forking...\n");

    /* not even think in turning this into a switch. Been there, done that. */
    ret = (int) fork();
    if (ret == -1) {
        exit(1);
    }
    if (ret != 0) {
        _exit(0);
    }

    setsid();
    if (argc) {
        /* make it look pwetty */
        sprintf(argv[0], "[ArIdent Daemon for %s]", TALKERNAME);
    }
    host_socket = socket_connect(SERVER, HOSTPORT);
    if (host_socket < 0) {
        printf("Error in socket_connect() to %s:%s.\n", SERVER, HOSTPORT);
        exit(0);
    }
    authenticate_host(host_socket);
    ident_pid = getpid();
    printf("*** Booted successfully with PID %d ***\n", ident_pid);
    for (;;) {
        FD_ZERO(&readmask);
        FD_SET(host_socket, &readmask);
        len = select(1 + host_socket, &readmask, NULL, NULL, NULL);
        if (len == -1) {
            continue;
        }
        len = recv(host_socket, inpstr, (sizeof inpstr) - 3, 0);
        if (!len) {
#ifdef DEBUG
            printf("Disconnected from host.\n");
#endif
            shutdown(host_socket, SHUT_WR);
            close(host_socket);
            host_socket = -1;
            do {
                sleep(5);
                host_socket = socket_connect(SERVER, HOSTPORT);
            } while (host_socket < 0);
            authenticate_host(host_socket);
            continue;
        }
        inpstr[len] = '\0';
        inpstr[len + 1] = 127;
#ifdef DEBUG
        printf("RECEIVED: %s\n", inpstr);
#endif
        next = inpstr - 1;
        while (*(++next) != 127) {
            current = next;
            while (*next && *next != '\n') {
                ++next;
            }
            *next = '\0';
            if (!strncmp(current, "EXIT", 4)) {
                shutdown(host_socket, SHUT_WR);
                close(host_socket);
                exit(0);
            }
            switch (double_fork()) {
            case -1:
                exit(1); /* fork failure */
            case 0:
                break; /* child continues */
            default:
                continue; /* parent carries on the fine family tradition */
            }
            if (argc) {
                sprintf(argv[0], "[ArIdent Child for %s]", TALKERNAME);
            }
            if (!strncmp(current, "PID", 3)) {
                sprintf(buffer, "PRETURN: %u\n", ident_pid);
#ifdef DEBUG
                printf("[PID] %s\n", buffer);
#endif
                send(host_socket, buffer, strlen(buffer), 0);
                _exit(0);
            }
            if (!strncmp(current, "SITE:", 5)) {
                /* They want a site. So call the site function and send a message back. */
                miasma = current + 6;
                sprintf(buffer, "RETURN: %s %s\n", miasma, get_proc(miasma));
#ifdef DEBUG
                printf("[SITE] %s\n", buffer);
#endif
                send(host_socket, buffer, strlen(buffer), 0);
                _exit(0);
            }
            if (!strncmp(current, "AUTH:", 5)) {
                char word[MAX_WORDS + 1][WORD_LEN + 1];
                struct timeval t_struct;
                int auth_socket;

                /* They want a username. So setup nice sockets stuff. */
                miasma = current + 6;
                wordfind(miasma, word);
                miasma = strchr(word[3], '!');
                if (miasma) {
                    *miasma = '\0';
                }
                auth_socket = socket_connect(word[3], "113");
                if (auth_socket < 0) {
                    _exit(0);
                }
                sprintf(buffer, "%s, %s\n", word[1], word[2]);
                send(auth_socket, buffer, strlen(buffer), 0);
                for (;;) {
                    FD_ZERO(&readmask);
                    FD_SET(auth_socket, &readmask);
                    t_struct.tv_sec = 10;
                    t_struct.tv_usec = 0;
                    len = select(1 + auth_socket, &readmask, NULL, NULL, &t_struct);
                    if (len == -1) {
                        continue;
                    }
                    if (!len) {
                        shutdown(auth_socket, SHUT_WR);
                        close(auth_socket);
                        _exit(0);
                    }
                    len = recv(auth_socket, authstr, (sizeof authstr) - 3, 0);
                    if (!len) {
                        shutdown(auth_socket, SHUT_WR);
                        close(auth_socket);
                        _exit(0);
                    }
                    if (len > 255 || len < 5) {
                        shutdown(auth_socket, SHUT_WR);
                        close(auth_socket);
                        _exit(0);
                    }
                    authstr[len] = '\0'; /* Find the last "word" in inpstr. */
                    if (strstr(authstr, "ERROR")) {
                        shutdown(auth_socket, SHUT_WR);
                        close(auth_socket);
                        _exit(0);
                    }
                    for (i = len - 1; i > 2; --i) {
                        if (authstr[i] == ' ' || authstr[i] == ':') {
                            miasma = authstr + i + 1;
                            sprintf(buffer, "ARETURN: %s %s %s\n", word[1],
                                    word[0], miasma);
#ifdef DEBUG
                            printf("[AUTH] %s\n", buffer);
#endif
                            send(host_socket, buffer, strlen(buffer), 0);
                            shutdown(auth_socket, SHUT_WR);
                            close(auth_socket);
                            _exit(0);
                        }
                    }
                    shutdown(auth_socket, SHUT_WR);
                    close(auth_socket);
                    _exit(0);
                }
            }
            _exit(0);
        }
    }
    return 0;
}

/*** Taken out of Amnuts, then modified. ***/
static int
socket_connect(const char *host, const char *serv)
{
    struct sockaddr_in sa;
    int s;

    sa.sin_family = AF_INET;
    sa.sin_port = htons(atoi(serv));
    sa.sin_addr.s_addr = inet_addr(host);
    if (sa.sin_addr.s_addr == (uint32_t) - 1) {
        struct hostent *he;

        /* This may hang. */
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
#ifdef DEBUG
    printf("Attempting connect to %s:%s... ", host, serv);
#endif
    /* This may hang. */
    if (connect(s, (struct sockaddr *) &sa, (sizeof sa)) == -1) {
#ifdef DEBUG
        printf("Unsuccessful.\n");
#endif
        return -1;
    }
#ifdef DEBUG
    printf("Successful.\n");
#endif
    return s;
}


#ifdef MANDNS

static char *remove_first(char *);

/*
 * Return position of second word in the given string
 */
static char *
remove_first(char *inpstr)
{
    char *str;

    str = inpstr;
    for (; *str; ++str) {
        if (!isspace(*str)) {
            break;
        }
    }
    for (; *str; ++str) {
        if (isspace(*str)) {
            break;
        }
    }
    for (; *str; ++str) {
        if (!isspace(*str)) {
            break;
        }
    }
    return str;
}

/* Mmmm...this function looks a lot like SquirT's host function. Thanks SquirT! */
static const char *
get_proc(const char *site)
{
    FILE *pp;
    static char str[256], temp[256];
    char temp2[5][256];
    int i;

    sprintf(str, "%s %s", HOSTBIN, site);
#ifdef DEBUG
    printf("[PROC] Getting: %s\n", str);
#endif
    pp = popen(str, "r");
    if (!pp) {
#ifdef DEBUG
        printf("[PROC] Not opened: %s\n", str);
#endif
        return site;
    }
    *str = '\0';
    *temp = '\0';
    for (i = 0; i < 5; ++i) {
        *temp2[i] = '\0';
    }
    fgets(str, 255, pp);
    pclose(pp);
#ifdef DEBUG
    printf("[PROC] Determined: %s\n", str);
#endif
    if (strstr(str, "Host not found")) {
#ifdef DEBUG
        printf("[PROC] Host not found\n");
#endif
        return site;
    }
    if (strstr(str, "domain name pointer")) {
#ifdef DEBUG
        printf("[PROC] Domain name pointer\n");
#endif
        return site;
    }
#if !!0
    sscanf(str, "%s %s %s %s %s", temp2[0], temp2[1], temp2[2], temp2[3],
            temp2[4]);
    strcpy(temp, temp2[1]);
#endif
#ifdef DEBUG
    printf("[PROC] str = %s\n", str);
#endif
    sscanf(str, "%s %s %s %s %s", temp2[0], temp2[1], temp2[2],
            temp2[3], temp2[4]);
    if (!strcasecmp("Name:", temp2[0])) {
        strcpy(temp, remove_first(str));
    } else {
        strcpy(temp, site);
    }
    return temp;
}
#else

static const char *
get_proc(const char *site)
{
    struct sockaddr_in site_addr;
    struct hostent *he;

    site_addr.sin_addr.s_addr = inet_addr(site);
    if (site_addr.sin_addr.s_addr == (uint32_t) - 1) {
        return site;
    }
    site_addr.sin_family = AF_INET;
    he =
            gethostbyaddr((char *) &site_addr.sin_addr, (sizeof site_addr.sin_addr),
            site_addr.sin_family);
    if (!he) {
        return site;
    }
    return he->h_name;
}
#endif

/*
 * Get words from sentence. This function prevents the words in the
 * sentence from writing off the end of a word array element. This is
 * difficult to do with sscanf() hence I use this function instead.
 *
 * This function is c/o good old Neil Robertson
 */
static int
wordfind(const char *inpstr, char word[MAX_WORDS + 1][WORD_LEN + 1])
{
    const char *str;
    size_t wcnt, wpos;

    str = inpstr;
    for (wcnt = 0; wcnt < MAX_WORDS; ++wcnt) {
        for (; *str; ++str) {
            if (!isspace(*str)) {
                break;
            }
        }
        if (!*str) {
            break;
        }
        for (wpos = 0; wpos < WORD_LEN; ++wpos) {
            if (!*str || isspace(*str)) {
                break;
            }
            word[wcnt][wpos] = *str++;
        }
        word[wcnt][wpos] = '\0';
        for (; *str; ++str) {
            if (isspace(*str)) {
                break;
            }
        }
    }
    return wcnt;
}

/*** signal trapping not working, so fork twice ***/
static int
double_fork(void)
{
    pid_t pid;
    int status;

    pid = fork();
    if (!pid) {
        switch (fork()) {
        case 0:
            return 0;
        case -1:
            _exit(127);
        default:
            _exit(0);
        }
    }
    if (pid < 0 || waitpid(pid, &status, 0) < 0) {
        return -1;
    }
    if (WIFEXITED(status)) {
        if (!WEXITSTATUS(status)) {
            return 1;
        } else {
            errno = WEXITSTATUS(status);
        }
    } else {
        errno = EINTR;
    }
    return -1;
}

/* Login to the talker and send it the predetermined username and password */
static void
authenticate_host(int host_socket)
{
    char inpstr[500], buffer[500];
    fd_set readmask;
    int stage;
    int len;

#ifdef DEBUG
    printf("Authenticating... ");
#endif

    stage = 1;
    for (;;) {
        FD_ZERO(&readmask);
        FD_SET(host_socket, &readmask);
        len = select(1 + host_socket, &readmask, NULL, NULL, NULL);
        if (len == -1) {
            continue;
        }
        len = recv(host_socket, inpstr, (sizeof inpstr) - 3, 0);
        if (!len) {
#ifdef DEBUG
            printf("Disconnected from host.\n");
#endif
            exit(0);
        }
        inpstr[len] = '\0';
#ifdef DEBUG
        printf("RECEIVED: %s\n", inpstr);
#endif
        if (strstr(inpstr, "ame:") && stage == 1) {
            ++stage;
            sprintf(buffer, "%s\n", USERNAME);
            send(host_socket, buffer, strlen(buffer), 0);
        }
        if (strstr(inpstr, "assword:") && stage == 2) {
            sprintf(buffer, "%s\n", PASSWORD);
            send(host_socket, buffer, strlen(buffer), 0);
            break;
        }
    }
#ifdef DEBUG
    printf("Successful.\n");
#endif
}
