/****************************************************************************
             Amnuts - Copyright (C) Andrew Collington, 1996-2023
                        Last update: Sometime in 2023

                   talker@amnuts.net - https://amnuts.net/

                                 based on

   NUTS version 3.3.3 (Triple Three :) - Copyright (C) Neil Robertson 1996
 ***************************************************************************/

#ifndef AMNUTS_COMMANDS_H
#define AMNUTS_COMMANDS_H

/* This is the command general function type list */
#define CT_LIST \
  ML_ENTRY((GENERAL, "General")) \
  ML_ENTRY((SOCIAL,  "Social")) \
  ML_ENTRY((MSG,     "Messages")) \
  ML_ENTRY((USER,    "User")) \
  ML_ENTRY((IGNORE,  "Ignores")) \
  ML_ENTRY((MOVE,    "Movement")) \
  ML_CLONE((CLONE,   "Clones")) \
  ML_ENTRY((ADMIN,   "Admin")) \
  ML_NLINK((NETLINK, "Netlink")) \
  ML_ENTRY((COUNT,   NULL))


/* This is the command list */
#define CMD_LIST \
  ML_ENTRY((QUIT,         "quit",       "",    JAILED,  GENERAL)) \
  ML_ENTRY((LOOK,         "look",       "",    NEW,     GENERAL)) \
  ML_ENTRY((MODE,         "mode",       "",    NEW,     USER   )) \
  ML_ENTRY((SAY,          "say",        "",    JAILED,  SOCIAL )) \
  ML_ENTRY((SHOUT,        "shout",      "[",   SUPER,   SOCIAL )) \
  ML_ENTRY((STO,          "sto",        "[-",  SUPER,   SOCIAL )) \
  ML_ENTRY((TELL,         "tell",       "",    NEW,     SOCIAL )) \
  ML_ENTRY((EMOTE,        "emote",      ":;",  NEW,     SOCIAL )) \
  ML_ENTRY((SEMOTE,       "semote",     "&!",  SUPER,   SOCIAL )) \
  ML_ENTRY((PEMOTE,       "pemote",     "</",  USER,    SOCIAL )) \
  ML_ENTRY((ECHO,         "echo",       "+",   USER,    SOCIAL )) \
  ML_ENTRY((GO,           "go",         "",    NEW,     MOVE   )) \
  ML_ENTRY((IGNLIST,      "ignlist",    "",    USER,    IGNORE )) \
  ML_ENTRY((PROMPT,       "prompt",     "",    NEW,     USER   )) \
  ML_ENTRY((DESC,         "desc",       "",    NEW,     USER   )) \
  ML_ENTRY((INPHRASE,     "inmsg",      "",    USER,    USER   )) \
  ML_ENTRY((OUTPHRASE,    "outmsg",     "",    USER,    USER   )) \
  ML_ENTRY((PUBCOM,       "public",     "",    USER,    GENERAL)) \
  ML_ENTRY((PRIVCOM,      "private",    "",    USER,    GENERAL)) \
  ML_ENTRY((LETMEIN,      "knock",      "",    USER,    GENERAL)) \
  ML_ENTRY((INVITE,       "invite",     "",    USER,    GENERAL)) \
  ML_ENTRY((TOPIC,        "topic",      "",    USER,    SOCIAL )) \
  ML_ENTRY((MOVE,         "move",       "",    WIZ,     MOVE   )) \
  ML_ENTRY((BCAST,        "bcast",      "",    WIZ,     SOCIAL )) \
  ML_ENTRY((WHO,          "who",        "@",   JAILED,  GENERAL)) \
  ML_ENTRY((PEOPLE,       "people",     "",    WIZ,     GENERAL)) \
  ML_ENTRY((HELP,         "help",       "",    JAILED,  GENERAL)) \
  ML_ENTRY((SHUTDOWN,     "shutdown" ,  "",    GOD,     ADMIN  )) \
  ML_ENTRY((NEWS,         "news",       "",    USER,    MSG    )) \
  ML_ENTRY((READ,         "read",       "",    NEW,     MSG    )) \
  ML_ENTRY((WRITE,        "write",      "",    USER,    MSG    )) \
  ML_ENTRY((WIPE,         "wipe",       "",    USER,    MSG    )) \
  ML_ENTRY((SEARCH,       "search",     "",    SUPER,   GENERAL)) \
  ML_ENTRY((REVIEW,       "review",     "",    USER,    GENERAL)) \
  ML_NLINK((HOME,         "home",       "",    USER,    MOVE   )) \
  ML_ENTRY((STATUS,       "ustat",      "",    USER,    USER   )) \
  ML_ENTRY((VER,          "version",    "",    NEW,     GENERAL)) \
  ML_ENTRY((RMAIL,        "rmail",      "",    NEW,     MSG    )) \
  ML_ENTRY((SMAIL,        "smail",      "",    USER,    MSG    )) \
  ML_ENTRY((DMAIL,        "dmail",      "",    USER,    MSG    )) \
  ML_ENTRY((FROM,         "from",       "",    USER,    MSG    )) \
  ML_ENTRY((ENTPRO,       "entpro",     "",    USER,    USER   )) \
  ML_ENTRY((EXAMINE,      "examine",    "",    USER,    USER   )) \
  ML_ENTRY((RMST,         "rooms",      "",    USER,    GENERAL)) \
  ML_NLINK((RMSN,         "rnet",       "",    USER,    NETLINK)) \
  ML_NLINK((NETSTAT,      "netstat",    "",    SUPER,   NETLINK)) \
  ML_NLINK((NETDATA,      "netdata",    "",    ARCH,    NETLINK)) \
  ML_NLINK((CONN,         "connect",    "",    GOD,     NETLINK)) \
  ML_NLINK((DISCONN,      "disconnect", "",    GOD,     NETLINK)) \
  ML_ENTRY((PASSWD,       "passwd",     "",    USER,    USER   )) \
  ML_ENTRY((KILL,         "kill",       "",    ARCH,    ADMIN  )) \
  ML_ENTRY((PROMOTE,      "promote",    "",    WIZ,     ADMIN  )) \
  ML_ENTRY((DEMOTE,       "demote",     "",    WIZ,     ADMIN  )) \
  ML_ENTRY((LISTBANS,     "lban",       "",    WIZ,     ADMIN  )) \
  ML_ENTRY((BAN,          "ban",        "",    ARCH,    ADMIN  )) \
  ML_ENTRY((UNBAN,        "unban",      "",    ARCH,    ADMIN  )) \
  ML_ENTRY((VIS,          "vis",        "",    ARCH,    USER   )) \
  ML_ENTRY((INVIS,        "invis",      "",    ARCH,    USER   )) \
  ML_ENTRY((SITE,         "site",       "",    WIZ,     ADMIN  )) \
  ML_ENTRY((WAKE,         "wake",       "",    USER,    SOCIAL )) \
  ML_ENTRY((WIZSHOUT,     "twiz",       "",    WIZ,     SOCIAL )) \
  ML_ENTRY((MUZZLE,       "muzzle",     "",    WIZ,     ADMIN  )) \
  ML_ENTRY((UNMUZZLE,     "unmuzzle",   "",    WIZ,     ADMIN  )) \
  ML_ENTRY((MAP,          "map",        "",    USER,    GENERAL)) \
  ML_ENTRY((LOGGING,      "logging",    "",    GOD,     ADMIN  )) \
  ML_ENTRY((MINLOGIN,     "minlogin",   "",    GOD,     ADMIN  )) \
  ML_ENTRY((SYSTEM,       "system",     "",    SUPER,   ADMIN  )) \
  ML_ENTRY((CHARECHO,     "charecho",   "",    NEW,     USER   )) \
  ML_ENTRY((CLEARLINE,    "clearline",  "",    ARCH,    ADMIN  )) \
  ML_ENTRY((FIX,          "fix",        "",    GOD,     GENERAL)) \
  ML_ENTRY((UNFIX,        "unfix",      "",    GOD,     GENERAL)) \
  ML_ENTRY((VIEWLOG,      "viewlog",    "",    WIZ,     ADMIN  )) \
  ML_ENTRY((ACCREQ,       "accreq",     "",    NEW,     USER   )) \
  ML_ENTRY((REVCLR,       "cbuff",      "*",   USER,    SOCIAL )) \
  ML_CLONE((CREATE,       "clone",      "",    ARCH,    CLONE  )) \
  ML_CLONE((DESTROY,      "destroy",    "",    ARCH,    CLONE  )) \
  ML_CLONE((MYCLONES,     "myclones",   "",    ARCH,    CLONE  )) \
  ML_CLONE((ALLCLONES,    "allclones",  "",    SUPER,   CLONE  )) \
  ML_CLONE((SWITCH,       "switch",     "",    ARCH,    CLONE  )) \
  ML_CLONE((CSAY,         "csay",       "",    ARCH,    CLONE  )) \
  ML_CLONE((CHEAR,        "chear",      "",    ARCH,    CLONE  )) \
  ML_NLINK((RSTAT,        "rstat",      "",    WIZ,     NETLINK)) \
  ML_SWEAR((SWBAN,        "swban",      "",    ARCH,    ADMIN  )) \
  ML_ENTRY((AFK,          "afk",        "",    USER,    USER   )) \
  ML_ENTRY((CLS,          "cls",        "",    NEW,     GENERAL)) \
  ML_ENTRY((COLOUR,       "colour",     "",    NEW,     GENERAL)) \
  ML_ENTRY((SUICIDE,      "suicide",    "",    NEW,     USER   )) \
  ML_ENTRY((DELETE,       "nuke",       "",    GOD,     ADMIN  )) \
  ML_ENTRY((REBOOT,       "reboot",     "",    GOD,     ADMIN  )) \
  ML_ENTRY((RECOUNT,      "recount",    "",    GOD,     MSG    )) \
  ML_ENTRY((REVTELL,      "revtell",    "",    USER,    SOCIAL )) \
  ML_ENTRY((PURGE,        "purge",      "",    GOD,     ADMIN  )) \
  ML_ENTRY((HISTORY,      "history",    "",    WIZ,     ADMIN  )) \
  ML_ENTRY((EXPIRE,       "expire",     "",    GOD,     ADMIN  )) \
  ML_ENTRY((BBCAST,       "bbcast",     "",    ARCH,    SOCIAL )) \
  ML_ENTRY((SHOW,         "show",       "'",   SUPER,   SOCIAL )) \
  ML_ENTRY((RANKS,        "ranks",      "",    NEW,     GENERAL)) \
  ML_ENTRY((WIZLIST,      "wizlist",    "",    NEW,     GENERAL)) \
  ML_ENTRY((TIME,         "time",       "",    USER,    GENERAL)) \
  ML_ENTRY((CTOPIC,       "ctopic",     "",    SUPER,   SOCIAL )) \
  ML_ENTRY((COPYTO,       "copyto",     "",    USER,    MSG    )) \
  ML_ENTRY((NOCOPIES,     "nocopys",    "",    USER,    MSG    )) \
  ML_ENTRY((SET,          "set",        "",    NEW,     USER   )) \
  ML_ENTRY((MUTTER,       "mutter",     "",    USER,    SOCIAL )) \
  ML_ENTRY((MKVIS,        "makevis",    "",    ARCH,    USER   )) \
  ML_ENTRY((MKINVIS,      "makeinvis",  "",    ARCH,    USER   )) \
  ML_ENTRY((SOS,          "sos",        "",    JAILED,  SOCIAL )) \
  ML_ENTRY((PTELL,        "ptell",      "",    USER,    SOCIAL )) \
  ML_ENTRY((PREVIEW,      "preview",    "",    USER,    SOCIAL )) \
  ML_ENTRY((PICTURE,      "picture",    "",    SUPER,   SOCIAL )) \
  ML_ENTRY((GREET,        "greet",      "",    SUPER,   SOCIAL )) \
  ML_ENTRY((THINKIT,      "think",      "",    USER,    SOCIAL )) \
  ML_ENTRY((SINGIT,       "sing",       "",    USER,    SOCIAL )) \
  ML_ENTRY((WIZEMOTE,     "ewiz",       "",    WIZ,     SOCIAL )) \
  ML_ENTRY((SUG,          "suggest",    "",    USER,    MSG    )) \
  ML_ENTRY((RSUG,         "rsug",       "",    WIZ,     MSG    )) \
  ML_ENTRY((DSUG,         "dsug",       "",    GOD,     MSG    )) \
  ML_ENTRY((LAST,         "last",       "",    USER,    USER   )) \
  ML_ENTRY((MACROS,       "macros",     "",    USER,    USER   )) \
  ML_ENTRY((RULES,        "rules",      "",    JAILED,  GENERAL)) \
  ML_ENTRY((UNINVITE,     "uninvite",   "",    USER,    GENERAL)) \
  ML_ENTRY((LMAIL,        "lmail",      "",    ARCH,    MSG    )) \
  ML_ENTRY((ARREST,       "arrest",     "",    WIZ,     ADMIN  )) \
  ML_ENTRY((UNARREST,     "unarrest",   "",    WIZ,     ADMIN  )) \
  ML_ENTRY((VERIFY,       "verify",     "",    NEW,     MSG    )) \
  ML_ENTRY((ADDHISTORY,   "addhistory", "",    WIZ,     ADMIN  )) \
  ML_ENTRY((FORWARDING,   "forwarding", "",    GOD,     ADMIN  )) \
  ML_ENTRY((REVSHOUT,     "revshout",   "",    USER,    SOCIAL )) \
  ML_ENTRY((CSHOUT,       "cshout",     "",    SUPER,   SOCIAL )) \
  ML_ENTRY((CTELLS,       "ctells",     "",    USER,    SOCIAL )) \
  ML_ENTRY((MONITOR,      "monitor",    "",    WIZ,     ADMIN  )) \
  ML_ENTRY((QCALL,        "call",       ",",   USER,    SOCIAL )) \
  ML_ENTRY((UNQCALL,      "uncall",     "",    USER,    SOCIAL )) \
  ML_ENTRY((IGNSHOUTS,    "ignshout",   "",    USER,    IGNORE )) \
  ML_ENTRY((IGNTELLS,     "igntell",    "",    USER,    IGNORE )) \
  ML_ENTRY((IGNLOGONS,    "ignlogons",  "",    USER,    IGNORE )) \
  ML_ENTRY((IGNPICS,      "ignpics",    "",    USER,    IGNORE )) \
  ML_ENTRY((IGNGREETS,    "igngreets",  "",    USER,    IGNORE )) \
  ML_ENTRY((IGNBEEPS,     "ignbeeps",   "",    USER,    IGNORE )) \
  ML_ENTRY((IGNWIZ,       "ignwiz",     "",    WIZ,     IGNORE )) \
  ML_ENTRY((IGNALL,       "ignall",     "",    USER,    IGNORE )) \
  ML_ENTRY((ACCOUNT,      "create",     "",    WIZ,     USER   )) \
  ML_ENTRY((BFROM,        "bfrom",      "",    USER,    MSG    )) \
  ML_ENTRY((SAMESITE,     "samesite",   "",    WIZ,     ADMIN  )) \
  ML_ENTRY((SAVEALL,      "save",       "",    ARCH,    ADMIN  )) \
  ML_ENTRY((SHACKLE,      "shackle",    "",    ARCH,    ADMIN  )) \
  ML_ENTRY((UNSHACKLE,    "unshackle",  "",    ARCH,    ADMIN  )) \
  ML_ENTRY((JOIN,         "join",       "",    SUPER,   MOVE   )) \
  ML_CLONE((CEMOTE,       "cemote",     "",    ARCH,    CLONE  )) \
  ML_ENTRY((REVAFK,       "revafk",     "",    USER,    SOCIAL )) \
  ML_ENTRY((CAFK,         "cafk",       "",    USER,    SOCIAL )) \
  ML_ENTRY((REVEDIT,      "revedit",    "",    USER,    SOCIAL )) \
  ML_ENTRY((CEDIT,        "cedit",      "",    USER,    SOCIAL )) \
  ML_ENTRY((LISTEN,       "listen",     "",    USER,    IGNORE )) \
  ML_ENTRY((HANGMAN,      "hangman",    "",    USER,    GENERAL)) \
  ML_ENTRY((GUESS,        "guess",      "",    USER,    GENERAL)) \
  ML_ENTRY((RETIRE,       "retire",     "",    GOD,     ADMIN  )) \
  ML_ENTRY((UNRETIRE,     "unretire",   "",    GOD,     ADMIN  )) \
  ML_ENTRY((CMDCOUNT,     "cmdcount",   "",    ARCH,    ADMIN  )) \
  ML_ENTRY((RCOUNTU,      "rcountu",    "",    GOD,     ADMIN  )) \
  ML_ENTRY((RECAPS,       "recaps",     "",    GOD,     ADMIN  )) \
  ML_ENTRY((SETCMDLEV,    "setcmdlev",  "",    ARCH,    ADMIN  )) \
  ML_ENTRY((GREPUSER,     "grepu",      "",    WIZ,     GENERAL)) \
  ML_ENTRY((XCOM,         "xcom",       "",    ARCH,    ADMIN  )) \
  ML_ENTRY((GCOM,         "gcom",       "",    ARCH,    ADMIN  )) \
  ML_ENTRY((SFROM,        "sfrom",      "",    WIZ,     MSG    )) \
  ML_ENTRY((RLOADRM,      "rloadrm",    "",    GOD,     ADMIN  )) \
  ML_ENTRY((SETAUTOPROMO, "autopromo",  "",    GOD,     ADMIN  )) \
  ML_ENTRY((SAYTO,        "sayto",      "-",   USER,    SOCIAL )) \
  ML_ENTRY((FRIENDS,      "friends",    "",    USER,    SOCIAL )) \
  ML_ENTRY((FSAY,         "fsay",       "",    USER,    SOCIAL )) \
  ML_ENTRY((FEMOTE,       "femote",     "",    USER,    SOCIAL )) \
  ML_ENTRY((BRING,        "bring",      "",    SUPER,   MOVE   )) \
  ML_ENTRY((FORCE,        "force",      "",    GOD,     ADMIN  )) \
  ML_ENTRY((CALENDAR,     "calendar",   "",    USER,    GENERAL)) \
  ML_ENTRY((FWHO,         "fwho",       "",    USER,    GENERAL)) \
  ML_ENTRY((MYROOM,       "myroom",     "",    SUPER,   MOVE   )) \
  ML_ENTRY((MYLOCK,       "mylock",     "",    SUPER,   GENERAL)) \
  ML_ENTRY((VISIT,        "visit",      "",    USER,    MOVE   )) \
  ML_ENTRY((MYPAINT,      "mypaint",    "",    SUPER,   GENERAL)) \
  ML_ENTRY((MYNAME,       "myname",     "",    SUPER,   GENERAL)) \
  ML_ENTRY((IGNUSER,      "ignuser",    "",    USER,    IGNORE )) \
  ML_ENTRY((BEEP,         "beep",       "",    SUPER,   SOCIAL )) \
  ML_ENTRY((RMADMIN,      "rmadmin",    "",    ARCH,    ADMIN  )) \
  ML_ENTRY((MYKEY,        "mykey",      "",    SUPER,   GENERAL)) \
  ML_ENTRY((MYBGONE,      "mybgone",    "",    SUPER,   GENERAL)) \
  ML_ENTRY((WIZRULES,     "wrules",     "",    WIZ,     GENERAL)) \
  ML_ENTRY((DISPLAY,      "files",      "",    USER,    GENERAL)) \
  ML_ENTRY((DISPLAYADMIN, "adminfiles", "",    WIZ,     ADMIN  )) \
  ML_ENTRY((DUMPCMD,      "dump",       "",    GOD,     ADMIN  )) \
  ML_ENTRY((TEMPRO,       "tpromote",   "",    WIZ,     ADMIN  )) \
  ML_ENTRY((MORPH,        "cname",      "",    ARCH,    ADMIN  )) \
  ML_ENTRY((FMAIL,        "fmail",      "",    SUPER,   MSG    )) \
  ML_ENTRY((REMINDER,     "reminder",   "",    SUPER,   MSG    )) \
  ML_ENTRY((FSMAIL,       "fsmail",     "",    USER,    MSG    )) \
  ML_ENTRY((SREBOOT,      "sreboot",    "",    ARCH,    ADMIN  )) \
  ML_IDENT((RESITE,       "resite",     "",    WIZ,     ADMIN  )) \
  ML_GAMES((SHOOT,        "shoot",      "",    USER,    GENERAL)) \
  ML_GAMES((RELOAD,       "reload",     "",    USER,    GENERAL)) \
  ML_GAMES((DONATE,       "donate",     "",    USER,    GENERAL)) \
  ML_GAMES((CASH,         "cash",       "",    USER,    GENERAL)) \
  ML_GAMES((MONEY,        "money",      "",    ARCH,    GENERAL)) \
  ML_GAMES((BANK,         "bank",       "",    USER,    GENERAL)) \
  ML_ENTRY((FLAGGED,      "flagged",    "",    USER,    GENERAL)) \
  ML_ENTRY((SPODLIST,     "spodlist",   "",    SUPER,   GENERAL)) \
  ML_ENTRY((COUNT,        NULL,         NULL,  GOD+1,   COUNT  ))


/* This is the 'set' command attribute list */
#define SET_LIST \
  ML_ENTRY((SHOW,      "show",     "show the current attributes setting"                                   )) \
  ML_ENTRY((GEND,      "gender",   "sets your gender (male, female, or neuter)"                            )) \
  ML_ENTRY((AGE,       "age",      "set your age for people to see"                                        )) \
  ML_ENTRY((EMAIL,     "email",    "enter your email address"                                              )) \
  ML_ENTRY((HOMEP,     "www",      "enter your homepage address"                                           )) \
  ML_ENTRY((HIDEEMAIL, "hide",     "makes your email visible to only you and the law, or everyone (toggle)")) \
  ML_ENTRY((WRAP,      "wrap",     "sets screen wrap to be on or off (toggle)"                             )) \
  ML_ENTRY((PAGER,     "pager",    "sets how many lines per page of the pager you get"                     )) \
  ML_ENTRY((COLOUR,    "colour",   "display in colour or not (toggle)"                                     )) \
  ML_ENTRY((ROOM,      "room",     "lets you log back into the room you left from, if public (toggle)"     )) \
  ML_ENTRY((FWD,       "autofwd",  "lets you receive smails via your email address."                       )) \
  ML_ENTRY((PASSWD,    "password", "lets you see your password when entering it at the login (toggle)"     )) \
  ML_ENTRY((RDESC,     "rdesc",    "lets you ignore room descriptions (toggle)"                            )) \
  ML_ENTRY((COMMAND,   "command",  "displays the command lisiting differently (toggle)"                    )) \
  ML_ENTRY((RECAP,     "recap",    "allows you to have caps in your name"                                  )) \
  ML_ENTRY((ICQ,       "icq",      "allows you to put in your ICQ number"                                  )) \
  ML_ENTRY((ALERT,     "alert",    "lets you know when someone in your friends list logs on (toggle)"      )) \
  ML_ENTRY((REVBUF,    "revbuf",   "lets you reverse the viewing of your review buffers (toggle)"          )) \
  ML_ENTRY((COUNT,     NULL,       NULL                                                                    ))


/* Macro List Magic */
#define ML_ENTRY(a) ML_EXPAND a
#define ML_CLONE(a) ML_EXPAND a
#define ML_SWEAR(a) ML_EXPAND a
#ifndef GAMES
#define ML_GAMES(a)
#else
#define ML_GAMES(a) ML_EXPAND a
#endif
#ifndef IDENTD
#define ML_IDENT(a)
#else
#define ML_IDENT(a) ML_EXPAND a
#endif
#ifndef NETLINKS
#define ML_NLINK(a)
#else
#define ML_NLINK(a) ML_EXPAND a
#endif

/* XXX: Maybe find a better solution than enums? Pointers are unique */
enum ct_value {
#define ML_EXPAND(value,name) CT_ ## value,
    CT_LIST
#undef ML_EXPAND
};

enum cmd_value {
#define ML_EXPAND(value,name,alias,level,type) value,
    CMD_LIST
#undef ML_EXPAND
};

enum set_value {
#define ML_EXPAND(value,name,desc) SET ## value,
    SET_LIST
#undef ML_EXPAND
};

struct cmd_entry {
    const char *name;
    const char *alias;
    int level; /* FIXME: Should be enum lvl_value */
    enum ct_value function;
};

struct set_entry {
    const char *type;
    const char *desc;
};

/* XXX: Ug! Kill the globals */
extern enum cmd_value com_num;
extern const char *const command_types[];
extern const struct cmd_entry command_table[];
extern const struct set_entry setstr[];

/* XXX: These REALLY need to be some place else */
#ifdef __MAIN_FILE__

enum cmd_value com_num = COUNT;

const char *const command_types[] = {
#define ML_EXPAND(value,name) name,
    CT_LIST
#undef ML_EXPAND
};

const struct cmd_entry command_table[] = {
#define ML_EXPAND(value,name,alias,level,type) { name, alias, level, CT_ ## type },
    CMD_LIST
#undef ML_EXPAND
};

const struct set_entry setstr[] = {
#define ML_EXPAND(value,name,desc) { name, desc },
    SET_LIST
#undef ML_EXPAND
};

#endif

#endif
