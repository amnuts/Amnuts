/****************************************************************************
         Amnuts version 2.3.0 - Copyright (C) Andrew Collington, 2003
                      Last update: 2003-08-04

                              amnuts@talker.com
                          http://amnuts.talker.com/

                                   based on

   NUTS version 3.3.3 (Triple Three :) - Copyright (C) Neil Robertson 1996
 ***************************************************************************/

#ifndef AMNUTS_GLOBALS_H
#define AMNUTS_GLOBALS_H

#define ML_ENTRY(a) ML_EXPAND a

/* Levels */
#define LVL_LIST \
  ML_ENTRY((JAILED,     "JAILED", "|" )) \
  ML_ENTRY((NEW,        "NEW",    "_" )) \
  ML_ENTRY((USER,       "USER",   ""  )) \
  ML_ENTRY((SUPER,      "SUPER",  ""  )) \
  ML_ENTRY((WIZ,        "WIZ",    "^" )) \
  ML_ENTRY((ARCH,       "ARCH",   "+" )) \
  ML_ENTRY((GOD,        "GOD",    "*" )) \
  ML_ENTRY((NUM_LEVELS, NULL,     NULL))

/*
 * Object types
 */
typedef struct user_struct *UR_OBJECT;
typedef struct room_struct *RM_OBJECT;
#ifdef NETLINKS
typedef struct netlink_struct *NL_OBJECT;
#endif
typedef struct list_users_struct *UD_OBJECT;
typedef struct command_struct *CMD_OBJECT;
typedef struct system_struct *SYS_OBJECT;
typedef struct flagged_user *FU_OBJECT;
typedef struct review_buffer *RB_OBJECT;
typedef struct buf_mesg *PM_OBJECT;


/*
 * Enumerations
 */
enum lvl_value
{
#define ML_EXPAND(value,name,alias) value,
  LVL_LIST
#undef ML_EXPAND
};

enum swear_ban
{ SBOFF, SBMIN, SBMAX };

/* user and clone types */
enum user_type
{ USER_TYPE, CLONE_TYPE, REMOTE_TYPE };
enum user_clone_hear
{ CLONE_HEAR_NOTHING, CLONE_HEAR_SWEARS, CLONE_HEAR_ALL };

/* logon prompt stuff */
enum user_login
{ LOGIN_LOGGED, LOGIN_NAME, LOGIN_PASSWD, LOGIN_CONFIRM, LOGIN_PROMPT };

#ifdef NETLINKS
enum nl_type
{ UNCONNECTED, INCOMING, OUTGOING };
enum nl_stage
{ DOWN, VERIFYING, UP };
enum nl_allow
{ ALL, IN, OUT };
#endif

struct reminder_struct
{
  long date;
  char msg[REMINDER_LEN];
};

/*
 * user variables - some are saved in the user file, and some are not
 */
struct user_struct
{
  char name[USER_NAME_LEN + 1];
  char desc[USER_DESC_LEN + 1];
  char pass[PASS_LEN + 1];
  char in_phrase[PHRASE_LEN + 1];
  char out_phrase[PHRASE_LEN + 1];
  char buff[BUFSIZE];
  char site[MAXHOST];           /* XXX: Use NI_MAXHOST */
  char ipsite[MAXADDR];         /* XXX: Use NI_MAXHOST, INET_ADDRSTRLEN, INET6_ADDRSTRLEN */
  char last_site[MAXHOST];      /* XXX: Use NI_MAXHOST */
  char site_port[MAXSERV];      /* XXX: Use NI_MAXSERV */
  char page_file[81];
  char mail_to[WORD_LEN + 1];
  char afk_mesg[AFK_MESG_LEN + 1];
  char inpstr_old[REVIEW_LEN + 1];
  char logout_room[ROOM_NAME_LEN + 1];
  char version[10];
  char copyto[MAX_COPIES][USER_NAME_LEN + 1];
  char invite_by[USER_NAME_LEN + 1];
  char date[80];
  char email[81];
  char homepage[81];
  char recap[RECAP_NAME_LEN];
  char samesite_check_store[ARR_SIZE];
  char bw_recap[USER_NAME_LEN + 1];
  char call[USER_NAME_LEN + 1];
  char macros[10][MACRO_LEN];
  char verify_code[80];
#ifdef GAMES
  char hang_word[WORD_LEN + 1];
  char hang_word_show[WORD_LEN + 1];
  char hang_guess[WORD_LEN + 1];
#endif
  char *malloc_start;
  char *malloc_end;
  char icq[ICQ_LEN + 1];
  int type;
  int login;
  int attempts;
  int vis;
  int ignall;
  int command_mode;
  int prompt;
  int colour;
  int charmode_echo;
  int show_pass;
  int gender;
  int hideemail;
  int edit_line;
  int warned;
  int accreq;
  int ignall_store;
  int igntells;
  int afk;
  int clone_hear;
  int ignshouts;
  int pager;
  int expire;
  int lroom;
  int monitor;
  int show_rdesc;
  int wrap;
  int alert;
  int mail_verified;
  int autofwd;
  int pagecnt;
  int pages[MAX_PAGES];
  int ignpics;
  int ignlogons;
  int ignwiz;
  int igngreets;
  int ignbeeps;
  int hang_stage;
  int samesite_all_store;
#ifdef WIZPORT
  int wizport;
#endif
  int socket;
  int buffpos;
  int filepos;
  int charcnt;
  int misc_op;
  int last_login_len;
  int edit_op;
  int wipe_from;
  int wipe_to;
  int logons;
  int cmd_type;
  int user_page_pos;
  int lmail_all;
  enum lvl_value user_page_lev;
  enum lvl_value lmail_lev;
  enum lvl_value muzzled;
  enum lvl_value arrestby;
  enum lvl_value unarrest;
  enum lvl_value real_level;
  enum lvl_value level;
  int age;
  int misses;
  int hits;
  int kills;
  int deaths;
  int bullets;
  int hps;
  int login_prompt;
  int retired;
  int gcoms[MAX_GCOMS];
  int xcoms[MAX_XCOMS];
  int reverse_buffer;
  RM_OBJECT room;
  RM_OBJECT invite_room;
  RM_OBJECT wrap_room;
  UR_OBJECT prev;
  UR_OBJECT next;
  UR_OBJECT owner;
  struct reminder_struct reminder[MAX_REMINDERS];
  int reminder_pos;
  time_t last_input;
  time_t last_login;
  time_t total_login;
  time_t read_mail;
  time_t t_expire;
#ifdef NETLINKS
  NL_OBJECT netlink;
  NL_OBJECT pot_netlink;
#endif
  int pm_count;
  int pm_currcount;
  int universal_pager;
  PM_OBJECT pm_current;
  PM_OBJECT pm_first;
  PM_OBJECT pm_last;
  FU_OBJECT fu_first;
  FU_OBJECT fu_last;
  RB_OBJECT rb_first;
  RB_OBJECT rb_last;
  int money;
  int bank;
  int inctime;
};


/*
 * structure to see who last logged on
 */
struct last_login_struct
{
  char name[USER_NAME_LEN + 1], time[80];
  int on;
};


/*
 * room informaytion structure
 */
struct room_struct
{
  char name[ROOM_NAME_LEN + 1];
  char label[ROOM_LABEL_LEN + 1];
  char desc[ROOM_DESC_LEN + 1];
  char topic[TOPIC_LEN + 1];
  char revbuff[REVIEW_LINES][REVIEW_LEN + 2];
  char map[ROOM_NAME_LEN + 1];
  char show_name[PERSONAL_ROOMNAME_LEN];
  char owner[USER_NAME_LEN + 1];
  int access;                   /* public, private, etc. */
  int revline;                  /* line number for review */
  int mesg_cnt;
  char link_label[MAX_LINKS][ROOM_LABEL_LEN + 1];       /* temp store for parse */
  RM_OBJECT link[MAX_LINKS];
  RM_OBJECT next;
  RM_OBJECT prev;
#ifdef NETLINKS
  int inlink;                   /* 1 if room accepts incoming net links */
  char netlink_name[SERV_NAME_LEN + 1]; /* temp store for config parse */
  NL_OBJECT netlink;            /* for net links, 1 per room */
#endif
};


#ifdef NETLINKS
/*
 * Structure for net links, ie server initiates them
 */
struct netlink_struct
{
  char service[SERV_NAME_LEN + 1];
  char site[MAXHOST];           /* XXX: Use NI_MAXHOST */
  char port[MAXSERV];           /* XXX: Use NI_MAXSERV */
  char verification[VERIFY_LEN + 1];
  char buffer[ARR_SIZE * 2];
  char mail_to[WORD_LEN + 1];
  char mail_from[WORD_LEN + 1];
  FILE *mailfile;
  time_t last_recvd;
  int socket;
  enum nl_type type;
  int connected;
  enum nl_stage stage;
  int lastcom;
  enum nl_allow allow;
  int warned;
  int keepalive_cnt;
  int ver_major;
  int ver_minor;
  int ver_patch;
  UR_OBJECT mesg_user;
  RM_OBJECT connect_room;
  NL_OBJECT prev;
  NL_OBJECT next;
};
#endif


/*
 * main user list structure
 */
struct list_users_struct
{
  char name[USER_NAME_LEN + 1];
  char date[80];
  enum lvl_value level;
  int retired;
  UD_OBJECT next;
  UD_OBJECT prev;
};


/*
 * command list
 */
struct command_struct
{
  char name[15];                /* 15 characters should be long enough */
  char alias[5];                /* 5 characters should be long enough */
  int id;                       /* FIXME: Should be enum cmd_value */
  enum lvl_value level;
  int function;                 /* FIXME: Should be enum ct_value */
  int count;
  CMD_OBJECT next;
  CMD_OBJECT prev;
};


/*
 * system structure
 */
struct system_struct
{
  int heartbeat;
#ifdef NETLINKS
  int keepalive_interval;
  int net_idle_time;
#endif
  int ignore_sigterm;
  int mesg_life;
  int mesg_check_hour;
  int mesg_check_min;
  time_t mesg_check_done;
  time_t auto_purge_date;
  int login_idle_time;
  int user_idle_time;
  int time_out_afks;
  int crash_action;
  int num_of_users;
  int num_of_logins;
  int logons_old;
  int logons_new;
  int purge_count;
  int purge_skip;
  int users_purged;
  int user_count;
  int max_users;
  int max_clones;
  int min_private_users;
  int prompt_def;
  int colour_def;
  int charecho_def;
  int passwordecho_def;
  enum lvl_value time_out_maxlevel;
#ifdef WIZPORT
  enum lvl_value wizport_level;
#endif
  enum lvl_value minlogin_level;
  enum lvl_value gatecrash_level;
  enum lvl_value ignore_mp_level;
#ifdef NETLINKS
  enum lvl_value rem_user_maxlevel;
  enum lvl_value rem_user_deflevel;
#endif
  int auto_promote;
  int ban_swearing;
  int personal_rooms;
  int startup_room_parse;
#ifdef NETLINKS
  int auto_connect;
#endif
  int allow_recaps;
  int suggestion_count;
  int random_motds;
  int motd1_cnt;
  int motd2_cnt;
  int forwarding;
  int sbuffline;
  int resolve_ip;
  int rs_countdown;
  int level_count[GOD + 1];
  int last_cmd_cnt;
  int flood_protect;
  char default_warp[ROOM_NAME_LEN + 1];
  char default_jail[ROOM_NAME_LEN + 1];
#ifdef GAMES
  char default_bank[ROOM_NAME_LEN + 1];
  char default_shoot[ROOM_NAME_LEN + 1];
#endif
  int mport_socket;
  char mport_port[MAXSERV];     /* XXX: Use NI_MAXSERV */
#ifdef WIZPORT
  int wport_socket;
  char wport_port[MAXSERV];     /* XXX: Use NI_MAXSERV */
#endif
#ifdef NETLINKS
  int nlink_socket;
  char nlink_port[MAXSERV];     /* XXX: Use NI_MAXSERV */
  char verification[SERV_NAME_LEN + 1];
#endif
#ifdef IDENTD
  int ident_pid;
  int ident_state;
  int ident_socket;
#endif
  int boot_off_min;
  int stop_logins;
  int is_pager;
  unsigned logging;
  struct utsname uts;
  char shoutbuff[REVIEW_LINES][REVIEW_LEN + 2];
  time_t boot_time;
  time_t rs_announce;
  time_t rs_which;
  time_t sreb_time;
  UR_OBJECT rs_user;
};


/*
 * pager buffer structure
 */
struct buf_mesg
{
  char *data;
  PM_OBJECT next;
  PM_OBJECT prev;
};


/*
 * flagged users
 */
struct flagged_user
{
  char *name;
  unsigned int flags;
  FU_OBJECT prev;
  FU_OBJECT next;
};


/*
 * review buffer structure
 */
struct review_buffer
{
  char *buffer;
  char *name;
  unsigned int flags;
  RB_OBJECT next;
  RB_OBJECT prev;
};


/*
 * for seamless reboot
 */
struct talker_system_info
{
  int mport_socket;
#ifdef WIZPORT
  int wport_socket;
#endif
#ifdef NETLINKS
  int nlink_socket;
#endif
  int reduce;
  char mr_name[ROOM_NAME_LEN + 1];
  char rebooter[USER_NAME_LEN + 1];
  time_t boot_time;
  int last_com;
  int num_of_logins;
  int new_user_logins;
  int old_user_logins;
  int ban_swearing;
};


/*
 * Constant structures
 */
struct user_level_struct
{
  const char *name;
  const char *alias;
};

struct colour_codes_struct
{
  const char *txt_code;
  const char *esc_code;
};


/* XXX: Ug! Kill the globals */
extern const struct user_level_struct user_level[];
extern const struct colour_codes_struct colour_codes[];
extern const char *const noyes[];
extern const char *const offon[];
extern const char *const minmax[];
extern const char *const sex[];
extern const char *const swear_words[];
extern const char swear_censor[];
extern const char noswearing[];
extern const char syserror[];
extern const char nosuchroom[];
extern const char nosuchuser[];
extern const char notloggedon[];
extern const char invisenter[];
extern const char invisleave[];
extern const char invisname[];
extern const char crypt_salt[];
extern const char default_personal_room_desc[];
extern const char talker_signature[];

extern struct last_login_struct last_login_info[LASTLOGON_NUM + 1];
extern UR_OBJECT user_first;
extern UR_OBJECT user_last;
extern RM_OBJECT room_first;
extern RM_OBJECT room_last;
#ifdef NETLINKS
extern NL_OBJECT nl_first;
extern NL_OBJECT nl_last;
#endif
extern UD_OBJECT first_user_entry;
extern UD_OBJECT last_user_entry;
extern CMD_OBJECT first_command;
extern CMD_OBJECT last_command;
extern SYS_OBJECT amsys;
extern char word[MAX_WORDS][WORD_LEN + 1];
extern char wrd[8][81];
extern char cmd_history[16][128];
extern char text[ARR_SIZE * 2];
extern char vtext[ARR_SIZE * 2];
extern char progname[40];
extern char confile[40];
extern int destructed;
extern int force_listen;
extern int no_prompt;
extern int logon_flag;
extern int config_line;
extern int word_count;
extern unsigned retrieve_user_type;


/* XXX: These REALLY need to be some place else */
#ifdef __MAIN_FILE__


/*
 * levels used on the talker
 */
const struct user_level_struct user_level[] = {
#define ML_EXPAND(value,name,alias) { name, alias },
  LVL_LIST
#undef ML_EXPAND
};

/*
 * colour code values
 */
const struct colour_codes_struct colour_codes[] = {
  /* Standard stuff */
  {"RS", "\033[0m"},            /* reset */
  {"OL", "\033[1m"},            /* bold */
  {"UL", "\033[4m"},            /* underline */
  {"LI", "\033[5m"},            /* blink */
  {"RV", "\033[7m"},            /* reverse */
  /* Foreground colour */
  {"FK", "\033[30m"},           /* black */
  {"FR", "\033[31m"},           /* red */
  {"FG", "\033[32m"},           /* green */
  {"FY", "\033[33m"},           /* yellow */
  {"FB", "\033[34m"},           /* blue */
  {"FM", "\033[35m"},           /* magenta */
  {"FC", "\033[36m"},           /* cyan */
  {"FW", "\033[37m"},           /* white */
  /* Background colour */
  {"BK", "\033[40m"},           /* black */
  {"BR", "\033[41m"},           /* red */
  {"BG", "\033[42m"},           /* green */
  {"BY", "\033[43m"},           /* yellow */
  {"BB", "\033[44m"},           /* blue */
  {"BM", "\033[45m"},           /* magenta */
  {"BC", "\033[46m"},           /* cyan */
  {"BW", "\033[47m"},           /* white */
  /* Some compatibility names */
  {"FT", "\033[36m"},           /* cyan AKA turquoise */
  {"BT", "\033[46m"},           /* cyan AKA turquoise */
  {NULL, NULL},                 /* end */
};


/*
 * some general arrays being defined
 */
const char *const noyes[] = { "NO", "YES" }; // 3 characters max
const char *const offon[] = { "OFF", "ON" };
const char *const minmax[] = { "OFF", "MIN", "MAX" };
const char *const sex[] = { "Neuter", "Male", "Female" };

/*
 * swear words array.  These must all be lowercase.  NULL is the stopping
 * clause and must remain in the array even if you have no words listed.
 */
const char *const swear_words[] = {
  "fuck", "shit", "cunt", "cock", "bastard", "dyke", "fag", "pussy", "bitch",
  NULL
};

/*
 * This is what replaces any swear words in a string if the swearban is set to MIN.
 */
const char swear_censor[] = "smeg";     /* alright, so I'm a Red Dwarf fan! */
const char noswearing[] = "Swearing is not allowed here.\n";

/*
 * other strings used on the talker
 */
const char syserror[] = "Sorry, a system error has occured";
const char nosuchroom[] = "There is no such room.\n";
const char nosuchuser[] = "There is no such user.\n";
const char notloggedon[] = "There is no one of that name logged on.\n";
const char invisenter[] = "A presence enters the room...\n";
const char invisleave[] = "A presence leaves the room...\n";
const char invisname[] = "A presence";
const char crypt_salt[] = "NU";

/*
 * you can set a standard room desc for those people who are creating a new
 * personal room
 */
const char default_personal_room_desc[] =
  "The walls are stark and the floors are bare. Either the owner is new\n"
  "or just plain lazy. Perhaps they just don't know how to use the .mypaint\n"
  "command yet?\n";

/*
 * you can change this for whatever sig you want - of just "" if you don't want
 * to have a sig file attached at the end of emails
 */
const char talker_signature[] =
  "+--------------------------------------------------------------------------+\n"
  "|  This message has been smailed to you on The Amnuts Talker, and this is  |\n"
  "|      your auto-forward.  Please do not reply directly to this email.     |\n"
  "|                                                                          |\n"
  "|               Amnuts - A talker running at foo.bar.com 666               |\n"
  "|         email 'me@my.place' if you have any questions/comments           |\n"
  "+--------------------------------------------------------------------------+\n";

struct last_login_struct last_login_info[LASTLOGON_NUM + 1] = { {"", "", 0} };

UR_OBJECT user_first = NULL;
UR_OBJECT user_last = NULL;
RM_OBJECT room_first = NULL;
RM_OBJECT room_last = NULL;
#ifdef NETLINKS
NL_OBJECT nl_first = NULL;
NL_OBJECT nl_last = NULL;
#endif
UD_OBJECT first_user_entry = NULL;
UD_OBJECT last_user_entry = NULL;
CMD_OBJECT first_command = NULL;
CMD_OBJECT last_command = NULL;
SYS_OBJECT amsys = NULL;


/*
 * Other global variables
 */
char word[MAX_WORDS][WORD_LEN + 1] = { "" };
char wrd[8][81] = { "" };
char cmd_history[16][128] = { "" };
char text[ARR_SIZE * 2] = "";
char vtext[ARR_SIZE * 2] = "";
char progname[40] = "";
char confile[40] = "";
int destructed = 0;
int force_listen = 0;
int no_prompt = 0;
int logon_flag = 0;
int config_line = 0;
int word_count = 0;
unsigned retrieve_user_type = 0;


#endif

#endif
