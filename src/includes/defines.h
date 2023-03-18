/****************************************************************************
             Amnuts - Copyright (C) Andrew Collington, 1996-2023
                        Last update: Sometime in 2023

                   talker@amnuts.net - https://amnuts.net/

                                 based on

   NUTS version 3.3.3 (Triple Three :) - Copyright (C) Neil Robertson 1996
 ***************************************************************************/

#ifndef AMNUTS_DEFINES_H
#define AMNUTS_DEFINES_H

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <strings.h>


/*
 * version information
 * you can add and check against your own version number
 * but the Amnuts and NUTS must stay the same as listed below
 */
#define AMNUTSVER   "2.3.0CVS"
#define NUTSVER     "3.3.3"
#define USERVER     "0.3"
#define TALKER_NAME "Your Talker's Name Here"

/* general directories */
#define BASE_STORAGE_DIR "files"
#define ADMINFILES BASE_STORAGE_DIR "/adminfiles"
#define DATAFILES  BASE_STORAGE_DIR "/datafiles"
#define DUMPFILES  BASE_STORAGE_DIR "/dumpfiles"
#define HELPFILES  BASE_STORAGE_DIR "/helpfiles"
#define LOGFILES   BASE_STORAGE_DIR "/logfiles"
#define MAILSPOOL  BASE_STORAGE_DIR "/mailspool"
#define MISCFILES  BASE_STORAGE_DIR "/miscfiles"
#define MOTDFILES  BASE_STORAGE_DIR "/motds"
#define PICTFILES  BASE_STORAGE_DIR "/pictfiles"
#define TEXTFILES  BASE_STORAGE_DIR "/textfiles"

/* user directories */
#define USERFILES     BASE_STORAGE_DIR "/userfiles"
#define USERMAILS     "mail"
#define USERPROFILES  "profiles"
#define USERHISTORYS  "historys"
#define USERCOMMANDS  "xgcoms"
#define USERMACROS    "macros"
#define USERROOMS     "rooms"
#define USERREMINDERS "reminders"
#define USERFLAGGED   "flagged"

/* seamless reboot */
#define REBOOTING_DIR           BASE_STORAGE_DIR "/reboot"
#define USER_LIST_FILE          REBOOTING_DIR "/_ulist"
#define TALKER_SYSINFO_FILE     REBOOTING_DIR "/_sysinfo"
#define CHILDS_PID_FILE         REBOOTING_DIR "/_child_pid"
#define ROOM_LIST_FILE          REBOOTING_DIR "/_rlist"
#define LAST_USERS_FILE         REBOOTING_DIR "/_last"

/* files */
#define CONFIGFILE   "config"
#define NEWSFILE     "newsfile"
#define SITEBAN      "siteban"
#define USERBAN      "userban"
#define NEWBAN       "newban"
#define SUGBOARD     "suggestions"
#define RULESFILE    "rules"
#define WIZRULESFILE "wizrules"
#define HANGDICT     "hangman_words"
#define SHOWFILES    "showfiles"

/* system logs */
#define LAST_CMD   "last_command"
#define MAINSYSLOG "syslog"
#define REQSYSLOG  "reqlog"
#define NETSYSLOG  "netlog"
#define ERRSYSLOG  "errlog"
#define SYSLOG BIT(0)
#define REQLOG BIT(1)
#define NETLOG BIT(2)
#define ERRLOG BIT(3)

/* general defines */
#define MAXADDR 16              /* XXX: Use NI_MAXHOST, INET_ADDRSTRLEN, INET6_ADDRSTRLEN */
#define MAXHOST 1025            /* XXX: Use NI_MAXHOST */
#define MAXSERV 32              /* XXX: Use NI_MAXSERV */
#define OUT_BUFF_SIZE 1000
#define MAX_WORDS 10
#define WORD_LEN 80
#define ARR_SIZE 1000
#define MAX_LINES 15
#define REVIEW_LINES 45
#define REVTELL_LINES 45
#define REVIEW_LEN 400
#define BUFSIZE 1000
#define ROOM_NAME_LEN 20
#define ROOM_LABEL_LEN 5
#define LASTLOGON_NUM 15
#define LOGIN_FLOOD_CNT 20
#define UFILE_WORDS 16          /* must be at least 1 longer than max words in the _options */
#define UFILE_WORD_LEN 255
#define RECORD 1
#define NORECORD 0
#define ALIGN_LEFT 0
#define ALIGN_CENTRE 1
#define ALIGN_RIGHT 2


/* netlink defines */
#define VERIFY_LEN 20
#ifdef NETLINKS
#define SERV_NAME_LEN 80
#endif

/* user defines */
#define USER_NAME_MIN 3         /* probably bad to allow extremely short names */
#define USER_NAME_LEN 16
#define RECAP_NAME_LEN (USER_NAME_LEN*4 + 3)
#define USER_DESC_LEN 40
#define AFK_MESG_LEN 60
#define PHRASE_LEN 40
#define PASS_MIN 3              /* probably bad to allow extremely short passwords */
#define PASS_LEN 13             /* crypt() generates this many plus nul */
#define ROOM_DESC_LEN (MAX_LINES*81)    /* MAX_LINES lines of 80 chars each + MAX_LINES nl */
#define TOPIC_LEN 60
#define ICQ_LEN 20
#define NEUTER 0
#define MALE   1
#define FEMALE 2
#define NEWBIE_EXPIRES 20       /* days */
#define USER_EXPIRES   40       /* days */
#define SCREEN_WRAP 80          /* how many characters to wrap to */
#define MAX_COPIES 6            /* of smail */
#define MACRO_LEN 65
#define MAX_XCOMS 10
#define MAX_GCOMS 10
#define MAX_PAGES 1000          /* should be enough! */
#define MAX_REMINDERS 30
#define REMINDER_LEN  70

/* rooms */
#define MAX_LINKS 20
#define PRIVATE  BIT(0)
#define FIXED    BIT(1)
#define PERSONAL BIT(2)
#define PERSONAL_ROOMNAME_LEN 80

/* logon prompt stuff */
#define LOGIN_ATTEMPTS 3

#ifdef IDENTD
/* ident daemon stuff */
#define IDENTUSER "Identduser"
#endif

/*
 * some macros that are used in the code
 * these are for grammar
 */
#define PLTEXT_S(n) ((1==(n))?"":"s")
#define PLTEXT_ES(n) ((1==(n))?"":"es")
#define PLTEXT_IS(n) ((1==(n))?"is":"are")
#define PLTEXT_WAS(n) ((1==(n))?"was":"were")
#define SIZEOF(table) ((sizeof (table))/(sizeof *(table)))
/* these are for bit manipulation */
#define BIT(pos) (1L<<(pos))
/* utils */
#define STRLEN(s) (sizeof(s) / sizeof(s[0]) - 1)

/* money code */
#define DEFAULT_MONEY 1000
#define DEFAULT_BANK 3000
#define MAX_DONATION 5000
#define CREDITS_PER_HOUR 20
#define MIN_CREDIT_UPDATE_LEVEL SUPER

/* flags */
#define GL_PRESERVE_ORIG BIT(20)
#define fufROOMKEY BIT(0)
#define fufIGNORE  BIT(1)
#define fufFRIEND  BIT(2)
#define rbfTELL    BIT(0)
#define rbfEDIT    BIT(1)
#define rbfAFK     BIT(2)

#endif
