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

/*****************************************************************************/


/*
 * Shutdown the talker
 * sdboot: 0=shutdown, 1=reboot, 2=seamless reboot
 */
void
talker_shutdown(UR_OBJECT user, const char *str, int sdboot)
{
  static char *args[] = { progname, confile, NULL };
  UR_OBJECT u, next;
#ifdef NETLINKS
  NL_OBJECT nl, nlnext;
#endif
  const char *ptr;

  ptr = user ? user->bw_recap : str;
  switch (sdboot) {
  case 0:
    write_room(NULL, "\007\n~OLSYSTEM:~FR~LI Shutting down now!!\n\n");
    write_syslog(SYSLOG, 0, "*** SHUTDOWN initiated by %s ***\n", ptr);
    break;
  case 1:
    write_room(NULL, "\007\n~OLSYSTEM:~FY~LI Rebooting now!!\n\n");
    write_syslog(SYSLOG, 0, "*** REBOOT initiated by %s ***\n", ptr);
    break;
  case 2:
    write_level(WIZ, 1, NORECORD,
                "\007\n~OLSYSTEM:~FY~LI Seamless Rebooting now!!\n\n", NULL);
    write_room(NULL,
               "\n~OLThe ground suddenly shakes and starts to settle...\n\n");
    write_syslog(SYSLOG, 1, "*** SEAMLESS REBOOT initiated by %s ***\n", ptr);
#ifdef NETLINKS
    for (nl = nl_first; nl; nl = nlnext) {
      nlnext = nl->next;
      shutdown_netlink(nl);
    }
#endif
    do_sreboot(user);
    return;
    break;                      /* should not need, but good to have */
  }
#ifdef NETLINKS
  for (nl = nl_first; nl; nl = nlnext) {
    nlnext = nl->next;
    shutdown_netlink(nl);
  }
#endif
  for (u = user_first; u; u = next) {
    next = u->next;
    disconnect_user(u);
  }
  close(amsys->mport_socket);
#ifdef WIZPORT
  close(amsys->wport_socket);
#endif
#ifdef NETLINKS
  close(amsys->nlink_socket);
#endif
  if (sdboot) {
    /* If someone has changed the binary or the config filename while this
       prog has been running this will not work */
    /*
     * XXX: ISO C and historical compatibility introduce anomolies for
     * all exec functions.
     *
     * execv style will strip const from char specifiers because
     * historically there was no const and const cannot be tranparently
     * added to indirect types.
     *
     * execl style need a char null pointer because they are variadic
     * and technically not all null pointers are are physically
     * represented the same way.
     */
    execvp(progname, args);
    /* If we get this far it has not worked */
    write_syslog(SYSLOG, 0, "*** REBOOT FAILED %s: %s ***\n\n", long_date(1),
                 strerror(errno));
    exit(12);
  }
  write_syslog(SYSLOG, 0, "*** SHUTDOWN complete %s ***\n\n", long_date(1));
  exit(0);
}


/*
 * Shutdown talker interface func. Countdown time is entered in seconds so
 * we can specify less than a minute till reboot.
 */
void
shutdown_com(UR_OBJECT user)
{
  if (amsys->rs_which == 1) {
    write_user(user,
               "The reboot countdown is currently active, you must cancel it first.\n");
    return;
  }
  if (!strcmp(word[1], "cancel")) {
    if (!amsys->rs_countdown || amsys->rs_which) {
      write_user(user, "The shutdown countdown is not currently active.\n");
      return;
    }
    if (amsys->rs_countdown && !amsys->rs_which && !amsys->rs_user) {
      write_user(user,
                 "Someone else is currently setting the shutdown countdown.\n");
      return;
    }
    write_room(NULL, "~OLSYSTEM:~RS~FG Shutdown cancelled.\n");
    write_syslog(SYSLOG, 1, "%s cancelled the shutdown countdown.\n",
                 user->name);
    amsys->rs_countdown = 0;
    amsys->rs_announce = 0;
    amsys->rs_which = -1;
    amsys->rs_user = NULL;
    return;
  }
  if (word_count > 1 && !is_number(word[1])) {
    write_user(user, "Usage: shutdown [<secs>|cancel]\n");
    return;
  }
  if (amsys->rs_countdown && !amsys->rs_which) {
    write_user(user,
               "The shutdown countdown is currently active, you must cancel it first.\n");
    return;
  }
  if (word_count < 2) {
    amsys->rs_countdown = 0;
    amsys->rs_announce = 0;
    amsys->rs_which = -1;
    amsys->rs_user = NULL;
  } else {
    amsys->rs_countdown = atoi(word[1]);
    amsys->rs_which = 0;
  }
  write_user(user,
             "\n\07~FR~OL~LI*** WARNING - This will shutdown the talker! ***\n\nAre you sure about this (y|n)? ");
  user->misc_op = 1;
  no_prompt = 1;
}


/*
 * Reboot talker interface func
 */
void
reboot_com(UR_OBJECT user)
{
  if (!amsys->rs_which) {
    write_user(user,
               "The shutdown countdown is currently active, you must cancel it first.\n");
    return;
  }
  if (!strcmp(word[1], "cancel")) {
    if (!amsys->rs_countdown) {
      write_user(user, "The reboot countdown is not currently active.\n");
      return;
    }
    if (amsys->rs_countdown && !amsys->rs_user) {
      write_user(user,
                 "Someone else is currently setting the reboot countdown.\n");
      return;
    }
    write_room(NULL, "~OLSYSTEM:~RS~FG Reboot cancelled.\n");
    write_syslog(SYSLOG, 1, "%s cancelled the reboot countdown.\n",
                 user->name);
    amsys->rs_countdown = 0;
    amsys->rs_announce = 0;
    amsys->rs_which = -1;
    amsys->rs_user = NULL;
    return;
  }
  if (word_count > 1 && !is_number(word[1])) {
    write_user(user, "Usage: reboot [<secs>|cancel]\n");
    return;
  }
  if (amsys->rs_countdown) {
    write_user(user,
               "The reboot countdown is currently active, you must cancel it first.\n");
    return;
  }
  if (word_count < 2) {
    amsys->rs_countdown = 0;
    amsys->rs_announce = 0;
    amsys->rs_which = 1;
    amsys->rs_user = NULL;
  } else {
    amsys->rs_countdown = atoi(word[1]);
    amsys->rs_which = 1;
  }
  write_user(user,
             "\n\07~FY~OL~LI*** WARNING - This will reboot the talker! ***\n\nAre you sure about this (y|n)? ");
  user->misc_op = 7;
  no_prompt = 1;
}


/*
 * Seamless reboot talker interface func
 */
void
sreboot_com(UR_OBJECT user)
{
  if (!amsys->rs_which) {
    write_user(user,
               "The shutdown countdown is currently active, you must cancel it first.\n");
    return;
  }
  if (!strcmp(word[1], "cancel")) {
    if (!amsys->rs_countdown) {
      write_user(user,
                 "The seamless reboot countdown is not currently active.\n");
      return;
    }
    if (amsys->rs_countdown && !amsys->rs_user) {
      write_user(user,
                 "Someone else is currently setting the seamless reboot countdown.\n");
      return;
    }
    write_room(NULL, "~OLSYSTEM:~RS~FG Seamless reboot cancelled.\n");
    write_syslog(SYSLOG, 1, "%s cancelled the seamless reboot countdown.\n",
                 user->name);
    amsys->rs_countdown = 0;
    amsys->rs_announce = 0;
    amsys->rs_which = -1;
    amsys->rs_user = NULL;
    return;
  }
  if (word_count > 1 && !is_number(word[1])) {
    write_user(user, "Usage: sreboot [<secs>|cancel]\n");
    return;
  }
  if (amsys->rs_countdown) {
    write_user(user,
               "The seamless reboot countdown is currently active, you must cancel it first.\n");
    return;
  }
  if (word_count < 2) {
    amsys->rs_countdown = 0;
    amsys->rs_announce = 0;
    amsys->rs_which = 2;
    amsys->rs_user = NULL;
  } else {
    amsys->rs_countdown = atoi(word[1]);
    amsys->rs_which = 2;
  }
  write_user(user,
             "\n\07~FY~OL~LI*** WARNING - This will seamlessly reboot the talker! ***\n\nAre you sure about this (y|n)? ");
  user->misc_op = 7;
  no_prompt = 1;
}


#ifdef IDENTD
/*
 * start up the ArIdent ident daemon
 */
void
start_ident(UR_OBJECT user)
{
  if (amsys->ident_socket != -1) {
    shutdown(amsys->ident_socket, SHUT_WR);
    close(amsys->ident_socket);
  }
  amsys->ident_socket = user->socket;
  amsys->ident_state = 1;
  destruct_user(user);
  --amsys->num_of_logins;
  write_level(WIZ, 1, NORECORD, "~FY<ArIdent Daemon Started>~RS\n", NULL);
  write_sock(amsys->ident_socket, "\n");
  write_sock(amsys->ident_socket, "PID\n");
}

/*
 * For Ident, by Ardant
 */
void
resite(UR_OBJECT user)
{
  char buffer[255];
  UR_OBJECT u;

  if (word_count < 2) {
    write_user(user, "Usage: resite <user>|-a\n");
    return;
  }
  if (!amsys->ident_state) {
    write_user(user, "The ident daemon is not active.\n");
    return;
  }
  if (!strcmp(word[1], "-a")) {
    for (u = user_first; u; u = u->next) {
      sprintf(buffer, "SITE: %s\n", u->ipsite);
      write_sock(amsys->ident_socket, buffer);
#ifdef WIZPORT
      sprintf(buffer, "AUTH: %d %s %s %s\n", u->socket, u->site_port,
              !user->wizport ? amsys->mport_port : amsys->wport_port,
              u->ipsite);
#else
      sprintf(buffer, "AUTH: %d %s %s %s\n", u->socket, u->site_port,
              amsys->mport_port, u->ipsite);
#endif
      write_sock(amsys->ident_socket, buffer);
    }
    write_user(user, "Refreshed site lookup of all users.\n");
    return;
  }
  u = get_user_name(user, word[1]);
  if (!u) {
    write_user(user, notloggedon);
    return;
  }
  sprintf(buffer, "SITE: %s\n", u->ipsite);
  write_sock(amsys->ident_socket, buffer);
#ifdef WIZPORT
  sprintf(buffer, "AUTH: %d %s %s %s\n", u->socket, u->site_port,
          !user->wizport ? amsys->mport_port : amsys->wport_port, u->ipsite);
#else
  sprintf(buffer, "AUTH: %d %s %s %s\n", u->socket, u->site_port,
          amsys->mport_port, u->ipsite);
#endif
  write_sock(amsys->ident_socket, buffer);
  sprintf(text, "Refreshed site lookup for \"%s\".\n", u->name);
  write_user(user, text);
}
#endif


/*
 * See if users site is banned
 */
int
site_banned(char *sbanned, int newban)
{
  char line[82], filename[80];
  FILE *fp;
  int f;

  if (newban) {
    sprintf(filename, "%s/%s", DATAFILES, NEWBAN);
  } else {
    sprintf(filename, "%s/%s", DATAFILES, SITEBAN);
  }
  fp = fopen(filename, "r");
  if (!fp) {
    return 0;
  }
  for (f = fscanf(fp, "%s", line); f == 1; f = fscanf(fp, "%s", line)) {
    /* first do full name comparison */
    if (!strcmp(sbanned, line)) {
      break;
    }
    /* check using pattern matching */
    if (pattern_match(sbanned, line)) {
      break;
    }
  }
  fclose(fp);
  return f == 1;
}


/*
 * checks to see if someone is already in login-stage on the ports
 */
int
login_port_flood(char *asite)
{
  UR_OBJECT u;
  int cnt;

  cnt = 0;
  for (u = user_first; u; u = u->next) {
    if (u->login && (!strcmp(asite, u->site) || !strcmp(asite, u->ipsite))) {
      ++cnt;
    }
  }
  return cnt >= LOGIN_FLOOD_CNT;
}


/*
 * See if user is banned
 */
int
user_banned(char *name)
{
  char line[82], filename[80];
  FILE *fp;
  int f;

  sprintf(filename, "%s/%s", DATAFILES, USERBAN);
  fp = fopen(filename, "r");
  if (!fp) {
    return 0;
  }
  for (f = fscanf(fp, "%s", line); f == 1; f = fscanf(fp, "%s", line)) {
    if (!strcmp(line, name)) {
      break;
    }
  }
  fclose(fp);
  return f == 1;
}


/*
 * List banned sites or users
 */
void
listbans(UR_OBJECT user)
{
  char filename[80];
  int i;

  strtolower(word[1]);
  if (!strcmp(word[1], "sites")) {
    write_user(user, "\n~BB*** Banned sites and domains ***\n\n");
    sprintf(filename, "%s/%s", DATAFILES, SITEBAN);
    switch (more(user, user->socket, filename)) {
    case 0:
      write_user(user, "There are no banned sites and domains.\n\n");
      return;
    case 1:
      user->misc_op = 2;
      break;
    }
    return;
  }
  if (!strcmp(word[1], "users")) {
    write_user(user, "\n~BB*** Banned users ***\n\n");
    sprintf(filename, "%s/%s", DATAFILES, USERBAN);
    switch (more(user, user->socket, filename)) {
    case 0:
      write_user(user, "There are no banned users.\n\n");
      return;
    case 1:
      user->misc_op = 2;
      break;
    }
    return;
  }
  if (!strcmp(word[1], "swears")) {
    write_user(user, "\n~BB*** Banned swear words ***\n\n");
    for (i = 0; swear_words[i]; ++i) {
      write_user(user, swear_words[i]);
      write_user(user, "\n");
    }
    if (!i) {
      write_user(user, "There are no banned swear words.\n");
    }
    if (amsys->ban_swearing) {
      write_user(user, "\n");
    } else {
      write_user(user, "\n(Swearing ban is currently off)\n\n");
    }
    return;
  }
  if (strcmp(word[1], "new")) {
    write_user(user,
               "\n~BB*** New users banned from sites and domains **\n\n");
    sprintf(filename, "%s/%s", DATAFILES, NEWBAN);
    switch (more(user, user->socket, filename)) {
    case 0:
      write_user(user,
                 "There are no sites and domains where new users have been banned.\n\n");
      return;
    case 1:
      user->misc_op = 2;
      break;
    }
    return;
  }
  write_user(user, "Usage: lban sites|users|new|swears\n");
}


/*
 * Ban a site, domain or user
 */
void
ban(UR_OBJECT user)
{
  static const char usage[] =
    "Usage: ban site|user|new <site>|<user>|<site>\n";

  if (word_count < 3) {
    write_user(user, usage);
    return;
  }
  strtolower(word[1]);
  if (!strcmp(word[1], "site")) {
    ban_site(user);
    return;
  }
  if (!strcmp(word[1], "user")) {
    ban_user(user);
    return;
  }
  if (!strcmp(word[1], "new")) {
    ban_new(user);
    return;
  }
  write_user(user, usage);
}


/*
 * add a site without any comments to the site ban file and kick off
 * all those from the same site currently logging in
 */
void
auto_ban_site(char *asite)
{
  char filename[80];
  FILE *fp;
  UR_OBJECT u, next;

  sprintf(filename, "%s/%s", DATAFILES, SITEBAN);
  /* Write new ban to file */
  fp = fopen(filename, "a");
  if (!fp) {
    write_syslog(SYSLOG | ERRLOG, 0,
                 "ERROR: Could not open file to append in auto_ban_site().\n");
    return;
  }
  fprintf(fp, "%s\n", asite);
  fclose(fp);
  /* disconnect the users from that same site */
  for (u = user_first; u; u = next) {
    next = u->next;
    if (u->login && (!strcmp(asite, u->site) || !strcmp(asite, u->ipsite))) {
      write_user(u,
                 "\nYou have attempted to flood the talker ports.  We view this as\n\r");
      write_user(u,
                 "a sign that you are trying to hack this system.  Therefore you have now\n\r");
      write_user(u, "had logins from your site or domain banned.\n\n");
      write_syslog(SYSLOG, 1, "Line %d cleared due to port flood attempt.\n",
                   u->socket);
      disconnect_user(u);
    }
  }
  write_syslog(SYSLOG, 1,
               "BANNED site or domain \"%s\" due to attempted flood.\n",
               asite);
}


/*
 * Ban a site from logging onto the talker
 */
void
ban_site(UR_OBJECT user)
{
  char bsite[80];               /* XXX: Use NI_MAXHOST */
  char filename[80];
  FILE *fp;

  /* check for variations of wild card */
  if (!strcmp("*", word[2])) {
    write_user(user, "You cannot ban site \"*\".\n");
    return;
  }
  if (strstr(word[2], "**")) {
    write_user(user, "You cannot have \"**\" in your site to ban.\n");
    return;
  }
  if (strstr(word[2], "?*")) {
    write_user(user, "You cannot have \"?*\" in your site to ban.\n");
    return;
  }
  if (strstr(word[2], "*?")) {
    write_user(user, "You cannot have \"*?\" in your site to ban.\n");
    return;
  }
  /* check if full name matches the host name */
  if (!strncasecmp(word[2], amsys->uts.nodename, strlen(word[2]))) {
    write_user(user,
               "You cannot ban the machine that this program is running on.\n");
    return;
  }
  /* check if, with the wild cards, the name matches host name */
  if (pattern_match(amsys->uts.nodename, word[2])) {
    write_user(user,
               "You cannot ban the machine that that program is running on.\n");
    return;
  }
  sprintf(filename, "%s/%s", DATAFILES, SITEBAN);
  /* See if ban already set for given site */
  fp = fopen(filename, "r");
  if (fp) {
    int f;

    for (f = fscanf(fp, "%s", bsite); f == 1; f = fscanf(fp, "%s", bsite)) {
      if (!strcmp(bsite, word[2])) {
        break;
      }
    }
    if (f == 1) {
      write_user(user, "That site or domain is already banned.\n");
      fclose(fp);
      return;
    }
    fclose(fp);
  }
  /* Write new ban to file */
  fp = fopen(filename, "a");
  if (!fp) {
    vwrite_user(user, "%s: Cannot open file to append.\n", syserror);
    write_syslog(SYSLOG | ERRLOG, 0,
                 "ERROR: Cannot open file to append in ban_site().\n");
    return;
  }
  fprintf(fp, "%s\n", word[2]);
  fclose(fp);
  vwrite_user(user, "Site or domain \"%s\" now banned.\n", word[2]);
  write_syslog(SYSLOG, 1, "%s BANNED site or domain %s.\n", user->name,
               word[2]);
}


/*
 * Ban an individual user from logging onto the talker
 */
void
ban_user(UR_OBJECT user)
{
  char filename[80], name[USER_NAME_LEN + 1];
  FILE *fp;
  UR_OBJECT u;
  UD_OBJECT entry;

  *word[2] = toupper(*word[2]);
  if (!strcmp(user->name, word[2])) {
    write_user(user,
               "Trying to ban yourself is the seventh sign of madness.\n");
    return;
  }
  /* See if ban already set for given user */
  sprintf(filename, "%s/%s", DATAFILES, USERBAN);
  fp = fopen(filename, "r");
  if (fp) {
    int f;

    for (f = fscanf(fp, "%s", name); f == 1; f = fscanf(fp, "%s", name)) {
      if (!strcmp(name, word[2])) {
        break;
      }
    }
    if (f == 1) {
      write_user(user, "That user is already banned.\n");
      fclose(fp);
      return;
    }
    fclose(fp);
  }
  for (entry = first_user_entry; entry; entry = entry->next) {
    if (!strcmp(entry->name, word[2])) {
      break;
    }
  }
  if (!entry) {
    write_user(user, nosuchuser);
    return;
  }
  if (entry->level >= user->level) {
    write_user(user,
               "You cannot ban a user of equal or higher level than yourself.\n");
    return;
  }
  /* Write new ban to file */
  fp = fopen(filename, "a");
  if (!fp) {
    vwrite_user(user, "%s: Cannot open file to append.\n", syserror);
    write_syslog(SYSLOG | ERRLOG, 0,
                 "ERROR: Cannot open file to append in ban_user().\n");
    return;
  }
  fprintf(fp, "%s\n", word[2]);
  fclose(fp);
  write_user(user, "User banned.\n");
  write_syslog(SYSLOG, 1, "%s BANNED user %s.\n", user->name, word[2]);
  sprintf(text, "User name was ~FRbanned~RS by %s.\n", user->name);
  add_history(word[2], 1, "%s", text);
  /* See if already on */
  u = get_user(word[2]);
  if (u) {
    write_user(u,
               "\n\07~FR~OL~LIYou have been banished from here and banned from returning.\n\n");
    disconnect_user(u);
  }
}


/*
 * Ban any new accounts from a given site
 */
void
ban_new(UR_OBJECT user)
{
  char bsite[80];               /* XXX: Use NI_MAXHOST */
  char filename[80];
  FILE *fp;

  /* check for variations of wild card */
  if (!strcmp("*", word[2])) {
    write_user(user, "You cannot ban site \"*\".\n");
    return;
  }
  if (strstr(word[2], "**")) {
    write_user(user, "You cannot have \"**\" in your site to ban.\n");
    return;
  }
  if (strstr(word[2], "?*")) {
    write_user(user, "You cannot have \"?*\" in your site to ban.\n");
    return;
  }
  if (strstr(word[2], "*?")) {
    write_user(user, "You cannot have \"*?\" in your site to ban.\n");
    return;
  }
  /* check if full name matches the host name */
  if (!strncasecmp(word[2], amsys->uts.nodename, strlen(word[2]))) {
    write_user(user,
               "You cannot ban the machine that this program is running on.\n");
    return;
  }
  /* check if, with the wild cards, the name matches host name */
  if (pattern_match(amsys->uts.nodename, word[2])) {
    write_user(user,
               "You cannot ban the machine that that program is running on.\n");
    return;
  }
  sprintf(filename, "%s/%s", DATAFILES, NEWBAN);
  /* See if ban already set for given site */
  fp = fopen(filename, "r");
  if (fp) {
    int f;

    for (f = fscanf(fp, "%s", bsite); f == 1; f = fscanf(fp, "%s", bsite)) {
      if (!strcmp(bsite, word[2])) {
        break;
      }
    }
    if (f == 1) {
      write_user(user,
                 "New users from that site or domain have already been banned.\n");
      fclose(fp);
      return;
    }
    fclose(fp);
  }
  /* Write new ban to file */
  fp = fopen(filename, "a");
  if (!fp) {
    vwrite_user(user, "%s: Cannot open file to append.\n", syserror);
    write_syslog(SYSLOG | ERRLOG, 0,
                 "ERROR: Cannot open file to append in ban_new().\n");
    return;
  }
  fprintf(fp, "%s\n", word[2]);
  fclose(fp);
  write_user(user, "New users from site or domain banned.\n");
  write_syslog(SYSLOG, 1, "%s BANNED new users from site or domain %s.\n",
               user->name, word[2]);
}


/*
 * uban a site (or domain) or user
 */
void
unban(UR_OBJECT user)
{
  static const char usage[] =
    "Usage: unban site|user|new <site>|<user>|<site>\n";

  if (word_count < 3) {
    write_user(user, usage);
    return;
  }
  strtolower(word[1]);
  if (!strcmp(word[1], "site")) {
    unban_site(user);
    return;
  }
  if (!strcmp(word[1], "user")) {
    unban_user(user);
    return;
  }
  if (!strcmp(word[1], "new")) {
    unban_new(user);
    return;
  }
  write_user(user, usage);
}


/*
 * remove a ban for a whole site
 */
void
unban_site(UR_OBJECT user)
{
  char ubsite[80];              /* XXX: Use NI_MAXHOST */
  char filename[80];
  FILE *infp, *outfp;
  int found, cnt, f;

  sprintf(filename, "%s/%s", DATAFILES, SITEBAN);
  infp = fopen(filename, "r");
  if (!infp) {
    write_user(user, "That site or domain is not currently banned.\n");
    return;
  }
  outfp = fopen("tempfile", "w");
  if (!outfp) {
    vwrite_user(user, "%s: Cannot open tempfile.\n", syserror);
    write_syslog(SYSLOG | ERRLOG, 0,
                 "ERROR: Cannot open tempfile to write in unban_site().\n");
    fclose(infp);
    return;
  }
  found = cnt = 0;
  for (f = fscanf(infp, "%s", ubsite); f == 1; f = fscanf(infp, "%s", ubsite)) {
    if (!strcmp(word[2], ubsite)) {
      ++found;
      continue;
    }
    ++cnt;
    fprintf(outfp, "%s\n", ubsite);
  }
  fclose(infp);
  fclose(outfp);
  if (!found) {
    write_user(user, "That site or domain is not currently banned.\n");
    remove("tempfile");
    return;
  }
  rename("tempfile", filename);
  if (!cnt) {
    remove(filename);
  }
  write_user(user, "Site ban removed.\n");
  write_syslog(SYSLOG, 1, "%s UNBANNED site %s.\n", user->name, word[2]);
}


/*
 * unban a user from logging onto the talker
 */
void
unban_user(UR_OBJECT user)
{
  char filename[80], name[USER_NAME_LEN + 1];
  FILE *infp, *outfp;
  int found, cnt, f;

  sprintf(filename, "%s/%s", DATAFILES, USERBAN);
  infp = fopen(filename, "r");
  if (!infp) {
    write_user(user, "That user is not currently banned.\n");
    return;
  }
  outfp = fopen("tempfile", "w");
  if (!outfp) {
    vwrite_user(user, "%s: Cannot open tempfile.\n", syserror);
    write_syslog(SYSLOG | ERRLOG, 0,
                 "ERROR: Cannot open tempfile to write in unban_user().\n");
    fclose(infp);
    return;
  }
  *word[2] = toupper(*word[2]);
  found = cnt = 0;
  for (f = fscanf(infp, "%s", name); f == 1; f = fscanf(infp, "%s", name)) {
    if (!strcmp(word[2], name)) {
      ++found;
      continue;
    }
    ++cnt;
    fprintf(outfp, "%s\n", name);
  }
  fclose(infp);
  fclose(outfp);
  if (!found) {
    write_user(user, "That user is not currently banned.\n");
    remove("tempfile");
    return;
  }
  rename("tempfile", filename);
  if (!cnt) {
    remove(filename);
  }
  vwrite_user(user, "User \"%s\" ban removed.\n", word[2]);
  write_syslog(SYSLOG, 1, "%s UNBANNED user %s.\n", user->name, word[2]);
  sprintf(text, "User name was ~FGunbanned~RS by %s.\n", user->name);
  add_history(word[2], 0, "%s", text);
}


/*
 * unban new accounts from a given site
 */
void
unban_new(UR_OBJECT user)
{
  char ubsite[80];              /* XXX: Use NI_MAXHOST */
  char filename[80];
  FILE *infp, *outfp;
  int found, cnt, f;

  sprintf(filename, "%s/%s", DATAFILES, NEWBAN);
  infp = fopen(filename, "r");
  if (!infp) {
    write_user(user,
               "New users from that site or domain are not currently banned.\n");
    return;
  }
  outfp = fopen("tempfile", "w");
  if (!outfp) {
    vwrite_user(user, "%s: Cannot open tempfile.\n", syserror);
    write_syslog(SYSLOG | ERRLOG, 0,
                 "ERROR: Cannot open tempfile to write in unban_new().\n");
    fclose(infp);
    return;
  }
  found = cnt = 0;
  for (f = fscanf(infp, "%s", ubsite); f == 1; f = fscanf(infp, "%s", ubsite)) {
    if (!strcmp(word[2], ubsite)) {
      ++found;
      continue;
    }
    ++cnt;
    fprintf(outfp, "%s\n", ubsite);
  }
  fclose(infp);
  fclose(outfp);
  if (!found) {
    write_user(user,
               "New users from that site or domain are not currently banned.\n");
    remove("tempfile");
    return;
  }
  rename("tempfile", filename);
  if (!cnt) {
    remove(filename);
  }
  write_user(user, "New users from site ban removed.\n");
  write_syslog(SYSLOG, 1, "%s UNBANNED new users from site %s.\n", user->name,
               word[2]);
}


/*
 * Force a user to become invisible
 */
void
make_invis(UR_OBJECT user)
{
  UR_OBJECT user2;

  if (word_count < 2) {
    write_user(user, "Usage: makeinvis <user>\n");
    return;
  }
  user2 = get_user_name(user, word[1]);
  if (!user2) {
    write_user(user, notloggedon);
    return;
  }
  if (user == user2) {
    write_user(user, "There is an easier way to make yourself invisible!\n");
    return;
  }
  if (!user2->vis) {
    vwrite_user(user, "%s~RS is already invisible.\n", user2->recap);
    return;
  }
  if (user2->level > user->level) {
    vwrite_user(user, "%s~RS cannot be forced invisible.\n", user2->recap);
    return;
  }
  user2->vis = 0;
  vwrite_user(user, "You force %s~RS to become invisible.\n", user2->recap);
  write_user(user2, "You have been forced to become invisible.\n");
  vwrite_room_except(user2->room, user2,
                     "You see %s~RS mysteriously disappear into the shadows!\n",
                     user2->recap);
}


/*
 * Force a user to become visible
 */
void
make_vis(UR_OBJECT user)
{
  UR_OBJECT user2;

  if (word_count < 2) {
    write_user(user, "Usage: makevis <user>\n");
    return;
  }
  user2 = get_user_name(user, word[1]);
  if (!user2) {
    write_user(user, notloggedon);
    return;
  }
  if (user == user2) {
    write_user(user, "There is an easier way to make yourself visible!\n");
    return;
  }
  if (user2->vis) {
    vwrite_user(user, "%s~RS is already visible.\n", user2->recap);
    return;
  }
  if (user2->level > user->level) {
    vwrite_user(user, "%s~RS cannot be forced visible.\n", user2->recap);
    return;
  }
  user2->vis = 1;
  vwrite_user(user, "You force %s~RS to become visible.\n", user2->recap);
  write_user(user2, "You have been forced to become visible.\n");
  vwrite_room_except(user2->room, user2,
                     "You see %s~RS mysteriously emerge from the shadows!\n",
                     user2->recap);
}


/*
 * Clone a user in another room
 */
void
create_clone(UR_OBJECT user)
{
  UR_OBJECT u;
  RM_OBJECT rm;
  const char *name;
  int cnt;

  /* Check room */
  if (word_count < 2) {
    rm = user->room;
  } else {
    rm = get_room(word[1]);
    if (!rm) {
      write_user(user, nosuchroom);
      return;
    }
  }
  /* If room is private then nocando */
  if (!has_room_access(user, rm)) {
    write_user(user,
               "That room is currently private, you cannot create a clone there.\n");
    return;
  }
  /* Count clones and see if user already has a copy there , no point having 2 in the same room */
  cnt = 0;
  for (u = user_first; u; u = u->next) {
    if (u->type == CLONE_TYPE && u->owner == user) {
      if (u->room == rm) {
        vwrite_user(user, "You already have a clone in the %s.\n", rm->name);
        return;
      }
      if (++cnt == amsys->max_clones) {
        write_user(user,
                   "You already have the maximum number of clones allowed.\n");
        return;
      }
    }
  }
  /* Create clone */
  u = create_user();
  if (!u) {
    vwrite_user(user, "%s: Unable to create copy.\n", syserror);
    write_syslog(SYSLOG | ERRLOG, 0,
                 "ERROR: Unable to create user copy in clone().\n");
    return;
  }
  u->type = CLONE_TYPE;
  u->socket = user->socket;
  u->room = rm;
  u->owner = user;
  u->vis = 1;
  strcpy(u->name, user->name);
  strcpy(u->recap, user->recap);
  strcpy(u->bw_recap, colour_com_strip(u->recap));
  strcpy(u->desc, "~BR~OL(CLONE)");
  if (rm == user->room) {
    write_user(user,
               "~FB~OLYou wave your hands, mix some chemicals and a clone is created here.\n");
  } else {
    vwrite_user(user,
                "~FB~OLYou wave your hands, mix some chemicals, and a clone is created in the %s.\n",
                rm->name);
  }
  name = user->vis ? user->bw_recap : invisname;
  vwrite_room_except(user->room, user, "~FB~OL%s waves their hands...\n",
                     name);
  vwrite_room_except(rm, user,
                     "~FB~OLA clone of %s appears in a swirling magical mist!\n",
                     user->bw_recap);
}


/*
 * Destroy all clones belonging to given user
 */
void
destroy_user_clones(UR_OBJECT user)
{
  UR_OBJECT u, next;

  for (u = user_first; u; u = next) {
    next = u->next;
    if (u->type == CLONE_TYPE && u->owner == user) {
      vwrite_room(u->room,
                  "The clone of %s~RS is engulfed in magical blue flames and vanishes.\n",
                  u->recap);
      destruct_user(u);
    }
  }
}


/*
 * Destroy user clone
*/
void
destroy_clone(UR_OBJECT user)
{
  UR_OBJECT u, u2;
  RM_OBJECT rm;
  const char *name;

  /* Check room and user */
  if (word_count < 2) {
    rm = user->room;
  } else {
    rm = get_room(word[1]);
    if (!rm) {
      write_user(user, nosuchroom);
      return;
    }
  }
  if (word_count > 2) {
    u2 = get_user_name(user, word[2]);
    if (!u2) {
      write_user(user, notloggedon);
      return;
    }
    if (u2->level >= user->level) {
      write_user(user,
                 "You cannot destroy the clone of a user of an equal or higher level.\n");
      return;
    }
  } else {
    u2 = user;
  }
  for (u = user_first; u; u = u->next) {
    if (u->type == CLONE_TYPE && u->room == rm && u->owner == u2) {
      break;
    }
  }
  if (!u) {
    if (u2 == user) {
      vwrite_user(user, "You do not have a clone in the %s.\n", rm->name);
    } else {
      vwrite_user(user, "%s~RS does not have a clone the %s.\n", u2->recap,
                  rm->name);
    }
    return;
  }
  destruct_user(u);
  reset_access(rm);
  write_user(user,
             "~FM~OLYou whisper a sharp spell and the clone is destroyed.\n");
  name = user->vis ? user->bw_recap : invisname;
  vwrite_room_except(user->room, user, "~FM~OL%s whispers a sharp spell...\n",
                     name);
  vwrite_room(rm, "~FM~OLThe clone of %s shimmers and vanishes.\n",
              u2->bw_recap);
  if (u2 != user) {
    vwrite_user(u2, "~OLSYSTEM: ~FR%s has destroyed your clone in the %s.\n",
                user->bw_recap, rm->name);
  }
  destructed = 0;
}


/*
 * Show users own clones
 */
void
myclones(UR_OBJECT user)
{
  UR_OBJECT u;
  int cnt;

  cnt = 0;
  for (u = user_first; u; u = u->next) {
    if (u->type != CLONE_TYPE || u->owner != user) {
      continue;
    }
    if (!cnt++) {
      write_user(user, "\n~BB*** Rooms you have clones in ***\n\n");
    }
    vwrite_user(user, "  %s\n", u->room->name);
  }
  if (!cnt) {
    write_user(user, "You have no clones.\n");
  } else {
    vwrite_user(user, "\nTotal of ~OL%d~RS clone%s.\n\n", cnt, PLTEXT_S(cnt));
  }
}


/*
 * Show all clones on the system
 */
void
allclones(UR_OBJECT user)
{
  UR_OBJECT u;
  int cnt;

  cnt = 0;
  for (u = user_first; u; u = u->next) {
    if (u->type != CLONE_TYPE) {
      continue;
    }
    if (!cnt++) {
      vwrite_user(user, "\n~BB*** Current clones %s ***\n\n", long_date(1));
    }
    vwrite_user(user, "%-15s : %s\n", u->name, u->room->name);
  }
  if (!cnt) {
    write_user(user, "There are no clones on the system.\n");
  } else {
    vwrite_user(user, "\nTotal of ~OL%d~RS clone%s.\n\n", cnt, PLTEXT_S(cnt));
  }
}


/*
 * User swaps places with his own clone. All we do is swap the rooms the objects are in
 */
void
clone_switch(UR_OBJECT user)
{
  UR_OBJECT u;
  RM_OBJECT rm;

  /*
     if no room was given then check to see how many clones user has.  If 1, then
     move the user to that clone, else give an error
   */
  if (word_count < 2) {
    UR_OBJECT tu;

    u = NULL;
    for (tu = user_first; tu; tu = tu->next) {
      if (tu->type == CLONE_TYPE && tu->owner == user) {
        if (u) {
          write_user(user,
                     "You have more than one clone active.  Please specify a room to switch to.\n");
          return;
        }
        u = tu;
      }
    }
    if (!u) {
      write_user(user, "You do not currently have any active clones.\n");
      return;
    }
    rm = u->room;
  } else {
    /* if a room name was given then try to switch to a clone there */
    rm = get_room(word[1]);
    if (!rm) {
      write_user(user, nosuchroom);
      return;
    }
    for (u = user_first; u; u = u->next) {
      if (u->type == CLONE_TYPE && u->room == rm && u->owner == user) {
        break;
      }
    }
    if (!u) {
      write_user(user, "You do not have a clone in that room.\n");
      return;
    }
  }
  write_user(user, "\n~FB~OLYou experience a strange sensation...\n");
  u->room = user->room;
  user->room = rm;
  vwrite_room_except(user->room, user, "The clone of %s comes alive!\n",
                     u->name);
  vwrite_room_except(u->room, u, "%s~RS turns into a clone!\n", u->recap);
  look(user);
}


/*
 * Auto promote a user if they need to be and auto_promote is turned on
 */
void
check_autopromote(UR_OBJECT user, int attrib)
{
  int cnt = 0, i;

  /* user cannot get credit for that more than once */
  if (user->accreq & BIT(attrib)) {
    return;
  }
  user->accreq |= BIT(attrib);
  if (!amsys->auto_promote || user->level != NEW) {
    return;
  }
  /* check it out for stage
     1=accreq, 2=desc, 3=gender */
  for (i = 1; i < 4; ++i) {
    if (user->accreq & BIT(i)) {
      ++cnt;
    }
  }
  switch (cnt) {
  case 1:
  case 2:
    vwrite_user(user,
                "\n~OL~FY*** You have completed step %d of 3 for auto-promotion ***\n\n",
                cnt);
    return;
  case 3:
    user->accreq = -1;
    user->level = (enum lvl_value) (user->level + 1);
    user->real_level = user->level;
    user->unarrest = user->level;
    user_list_level(user->name, user->level);
    strcpy(user->date, (long_date(1)));
    vwrite_user(user,
                "\n\07~OL~FY*** You have been auto-promoted to level %s ***\n\n",
                user_level[user->level].name);
    sprintf(text, "Was auto-promoted to level %s.\n",
            user_level[user->level].name);
    add_history(user->name, 1, "%s", text);
    write_syslog(SYSLOG, 1, "%s was AUTO-PROMOTED to level %s.\n", user->name,
                 user_level[user->level].name);
    vwrite_level(WIZ, 1, NORECORD, NULL,
                 "~OL[ AUTO-PROMOTE ]~RS %s to level %s.\n", user->name,
                 user_level[user->level].name);
    return;
  }
}


/*
 * do a promotion of a user that lasts only for the current session
 */
void
temporary_promote(UR_OBJECT user)
{
  UR_OBJECT u;
  enum lvl_value lvl;

  if (word_count < 2) {
    write_user(user, "Usage: tpromote <user> [<level>]\n");
    return;
  }
  u = get_user_name(user, word[1]);
  if (!u) {
    write_user(user, notloggedon);
    return;
  }
  if (u == user) {
    write_user(user,
               "You cannot promote yourself, temporarily or otherwise.\n");
    return;
  }
  /* determine what level to promote them to */
  if (u->level >= user->level) {
    write_user(user,
               "You cannot temporarily promote anyone of the same or greater level than you.\n");
    return;
  }
  if (word_count == 3) {
    lvl = get_level(word[2]);
    if (lvl == NUM_LEVELS) {
      write_user(user, "Usage: tpromote <user> [<level>]\n");
      return;
    }
    if (lvl <= u->level) {
      vwrite_user(user,
                  "You must specify a level higher than %s currently is.\n",
                  u->name);
      return;
    }
  } else {
    lvl = (enum lvl_value) (u->level + 1);
  }
  if (lvl == GOD) {
    vwrite_user(user, "You cannot temporarily promote anyone to level %s.\n",
                user_level[lvl].name);
    return;
  }
  if (lvl >= user->level) {
    write_user(user,
               "You cannot temporarily promote anyone to a higher level than your own.\n");
    return;
  }
  /* if they have already been temp promoted this session then restore normal level first */
  if (u->level > u->real_level) {
    u->level = u->real_level;
  }
  u->real_level = u->level;
  u->level = lvl;
  vwrite_user(user, "You temporarily promote %s to %s.\n", u->name,
              user_level[u->level].name);
  vwrite_room_except(u->room, u,
                     "~OL~FG%s~RS~OL~FG starts to glow as their power increases...\n",
                     u->bw_recap);
  vwrite_user(u, "~OL~FGYou have been promoted (temporarily) to level %s.\n",
              user_level[u->level].name);
  write_syslog(SYSLOG, 1, "%s TEMPORARILY promote %s to %s.\n", user->name,
               u->name, user_level[u->level].name);
  sprintf(text, "Was temporarily to level %s.\n", user_level[u->level].name);
  add_history(u->name, 1, "%s", text);
}


/*
 * Promote a user
 */
void
promote(UR_OBJECT user)
{
  UR_OBJECT u;
  int on;
  enum lvl_value lvl;

  if (word_count < 2) {
    write_user(user, "Usage: promote <user> [<level>]\n");
    return;
  }
  if (word_count > 3) {
    write_user(user, "Usage: promote <user> [<level>]\n");
    return;
  }
  u = retrieve_user(user, word[1]);
  if (!u) {
    return;
  }
  on = retrieve_user_type == 1;
  /* FIXME: lose .tpromote if .promote fails */
  /* first, gotta reset the user level if they have been temp promoted */
  if (u->real_level < u->level) {
    u->level = u->real_level;
  }
  /* cannot promote jailed users */
  if (u->level == JAILED) {
    vwrite_user(user, "You cannot promote a user of level %s.\n",
                user_level[JAILED].name);
    done_retrieve(u);
    return;
  }
  if (word_count < 3) {
    lvl = (enum lvl_value) (u->level + 1);
  } else {
    strtoupper(word[2]);
    lvl = get_level(word[2]);
    if (lvl == NUM_LEVELS) {
      vwrite_user(user, "You must select a level between %s and %s.\n",
                  user_level[USER].name, user_level[GOD].name);
      done_retrieve(u);
      return;
    }
    if (lvl <= u->level) {
      write_user(user,
                 "You cannot promote a user to a level less than or equal to what they are now.\n");
      done_retrieve(u);
      return;
    }
  }
  if (user->level < lvl) {
    write_user(user,
               "You cannot promote a user to a level higher than your own.\n");
    done_retrieve(u);
    return;
  }
  /* do it */
  u->level = lvl;
  u->unarrest = u->level;
  u->real_level = u->level;
  user_list_level(u->name, u->level);
  strcpy(u->date, (long_date(1)));
  u->accreq = -1;
  u->real_level = u->level;
  strcpy(u->date, long_date(1));
  add_user_date_node(u->name, (long_date(1)));
  sprintf(text, "~FG~OLYou have been promoted to level: ~RS~OL%s.\n",
          user_level[u->level].name);
  if (!on) {
    send_mail(user, u->name, text, 0);
  } else {
    write_user(u, text);
  }
  vwrite_level(u->level, 1, NORECORD, u,
               "~FG~OL%s is promoted to level: ~RS~OL%s.\n", u->bw_recap,
               user_level[u->level].name);
  write_syslog(SYSLOG, 1, "%s PROMOTED %s to level %s.\n", user->name,
               u->name, user_level[u->level].name);
  sprintf(text, "Was ~FGpromoted~RS by %s to level %s.\n", user->name,
          user_level[u->level].name);
  add_history(u->name, 1, "%s", text);
  if (!on) {
    u->socket = -2;
    strcpy(u->site, u->last_site);
  }
  save_user_details(u, on);
  done_retrieve(u);
}


/*
 * Demote a user
 */
void
demote(UR_OBJECT user)
{
  UR_OBJECT u;
  int on;
  enum lvl_value lvl;

  if (word_count < 2) {
    write_user(user, "Usage: demote <user> [<level>]\n");
    return;
  }
  if (word_count > 3) {
    write_user(user, "Usage: demote <user> [<level>]\n");
    return;
  }
  u = retrieve_user(user, word[1]);
  if (!u) {
    return;
  }
  on = retrieve_user_type == 1;
  /* FIXME: lose .tpromote if .demote fails */
  /* first, gotta reset the user level if they have been temp promoted */
  if (u->real_level < u->level) {
    u->level = u->real_level;
  }
  /* cannot demote new or jailed users */
  if (u->level <= NEW) {
    vwrite_user(user, "You cannot demote a user of level %s or %s.\n",
                user_level[NEW].name, user_level[JAILED].name);
    done_retrieve(u);
    return;
  }
  if (word_count < 3) {
    lvl = (enum lvl_value) (u->level - 1);
  } else {
    strtoupper(word[2]);
    lvl = get_level(word[2]);
    if (lvl == NUM_LEVELS) {
      vwrite_user(user, "You must select a level between %s and %s.\n",
                  user_level[NEW].name, user_level[ARCH].name);
      done_retrieve(u);
      return;
    }
    /* now runs checks if level option was used */
    if (lvl >= u->level) {
      write_user(user,
                 "You cannot demote a user to a level higher than or equal to what they are now.\n");
      done_retrieve(u);
      return;
    }
    if (lvl == JAILED) {
      vwrite_user(user, "You cannot demote a user to the level %s.\n",
                  user_level[JAILED].name);
      done_retrieve(u);
      return;
    }
  }
  if (u->level >= user->level) {
    write_user(user,
               "You cannot demote a user of an equal or higher level than yourself.\n");
    done_retrieve(u);
    return;
  }
  /* do it */
  if (u->level == WIZ) {
    clean_retire_list(u->name);
  }
  u->level = lvl;
  u->unarrest = u->level;
  u->real_level = u->level;
  user_list_level(u->name, u->level);
  u->vis = 1;
  strcpy(u->date, long_date(1));
  add_user_date_node(u->name, (long_date(1)));
  sprintf(text, "~FR~OLYou have been demoted to level: ~RS~OL%s.\n",
          user_level[u->level].name);
  if (!on) {
    send_mail(user, u->name, text, 0);
  } else {
    write_user(u, text);
  }
  vwrite_level(u->level, 1, NORECORD, u,
               "~FR~OL%s is demoted to level: ~RS~OL%s.\n", u->bw_recap,
               user_level[u->level].name);
  write_syslog(SYSLOG, 1, "%s DEMOTED %s to level %s.\n", user->name, u->name,
               user_level[u->level].name);
  sprintf(text, "Was ~FRdemoted~RS by %s to level %s.\n", user->name,
          user_level[u->level].name);
  add_history(u->name, 1, "%s", text);
  if (!on) {
    u->socket = -2;
    strcpy(u->site, u->last_site);
  }
  save_user_details(u, on);
  done_retrieve(u);
}


/*
 * Muzzle an annoying user so he cant speak, emote, echo, write, smail
 * or bcast. Muzzles have levels from WIZ to GOD so for instance a wiz
 * cannot remove a muzzle set by a god
 */
void
muzzle(UR_OBJECT user)
{
  UR_OBJECT u;
  int on;

  if (word_count < 2) {
    write_user(user, "Usage: muzzle <user>\n");
    return;
  }
  u = retrieve_user(user, word[1]);
  if (!u) {
    return;
  }
  on = retrieve_user_type == 1;
  /* error checks */
  if (u == user) {
    write_user(user,
               "Trying to muzzle yourself is the ninth sign of madness.\n");
    return;
  }
  if (u->level >= user->level) {
    write_user(user,
               "You cannot muzzle a user of equal or higher level than yourself.\n");
    done_retrieve(u);
    return;
  }
  if (u->muzzled >= user->level) {
    vwrite_user(user, "%s~RS is already muzzled.\n", u->recap);
    done_retrieve(u);
    return;
  }
  /* do the muzzle */
  u->muzzled = user->level;
  vwrite_user(user, "~FR~OL%s now has a muzzle of level: ~RS~OL%s.\n",
              u->bw_recap, user_level[user->level].name);
  write_syslog(SYSLOG, 1, "%s muzzled %s (level %d).\n", user->name, u->name,
               user->level);
  add_history(u->name, 1, "Level %d (%s) ~FRmuzzle~RS put on by %s.\n",
              user->level, user_level[user->level].name, user->name);
  sprintf(text, "~FR~OLYou have been muzzled!\n");
  if (!on) {
    send_mail(user, u->name, text, 0);
  } else {
    write_user(u, text);
  }
  /* finish up */
  if (!on) {
    strcpy(u->site, u->last_site);
    u->socket = -2;
  }
  save_user_details(u, on);
  done_retrieve(u);
}


/*
 * Unmuzzle the bastard now he has apologised and grovelled enough via email
 */
void
unmuzzle(UR_OBJECT user)
{
  UR_OBJECT u;
  int on;

  if (word_count < 2) {
    write_user(user, "Usage: unmuzzle <user>\n");
    return;
  }
  u = retrieve_user(user, word[1]);
  if (!u) {
    return;
  }
  on = retrieve_user_type == 1;
  /* error checks */
  if (u == user) {
    write_user(user,
               "Trying to unmuzzle yourself is the tenth sign of madness.\n");
    done_retrieve(u);
    return;
  }
  /* FIXME: Use sentinel other JAILED */
  if (u->muzzled == JAILED) {
    vwrite_user(user, "%s~RS is not muzzled.\n", u->recap);
    done_retrieve(u);
    return;
  }
  if (u->muzzled > user->level) {
    vwrite_user(user,
                "%s~RS's muzzle is set to level %s, you do not have the power to remove it.\n",
                u->recap, user_level[u->muzzled].name);
    done_retrieve(u);
    return;
  }
  /* do the unmuzzle */
  u->muzzled = JAILED;          /* FIXME: Use sentinel other JAILED */
  vwrite_user(user, "~FG~OLYou remove %s~RS's muzzle.\n", u->recap);
  write_syslog(SYSLOG, 1, "%s unmuzzled %s.\n", user->name, u->name);
  add_history(u->name, 0, "~FGUnmuzzled~RS by %s, level %d (%s).\n",
              user->name, user->level, user_level[user->level].name);
  sprintf(text, "~FG~OLYou have been unmuzzled!\n");
  if (!on) {
    send_mail(user, u->name, text, 0);
  } else {
    write_user(u, text);
  }
  /* finish up */
  if (!on) {
    strcpy(u->site, u->last_site);
    u->socket = -2;
  }
  save_user_details(u, on);
  done_retrieve(u);
}


/*
 * Put annoying user in jail
 */
void
arrest(UR_OBJECT user)
{
  UR_OBJECT u;
  RM_OBJECT rm;
  int on;

  if (word_count < 2) {
    write_user(user, "Usage: arrest <user>\n");
    return;
  }
  u = retrieve_user(user, word[1]);
  if (!u) {
    return;
  }
  on = retrieve_user_type == 1;
  /* error checks */
  if (u == user) {
    write_user(user, "You cannot arrest yourself.\n");
    return;
  }
  if (u->level >= user->level) {
    write_user(user,
               "You cannot arrest anyone of the same or higher level than yourself.\n");
    done_retrieve(u);
    return;
  }
  if (u->level == JAILED) {
    vwrite_user(user, "%s~RS has already been arrested.\n", u->recap);
    done_retrieve(u);
    return;
  }
  /* do it */
  u->vis = 1;
  u->unarrest = u->level;
  u->arrestby = user->level;
  u->level = JAILED;
  u->real_level = u->level;
  user_list_level(u->name, u->level);
  strcpy(u->date, (long_date(1)));
  sprintf(text, "~FR~OLYou have been placed under arrest.\n");
  if (!on) {
    send_mail(user, u->name, text, 0);
    vwrite_user(user, "%s has been placed under arrest.\n", u->name);
  } else {
    write_user(u, text);
    vwrite_user(user, "%s has been placed under arrest.\n", u->name);
    write_room(NULL, "The Hand of Justice reaches through the air...\n");
    rm = get_room_full(amsys->default_jail);
    if (!rm) {
      vwrite_user(user,
                  "Cannot find the jail, so %s~RS is arrested but still in the %s.\n",
                  u->recap, u->room->name);
    } else {
      move_user(u, rm, 2);
    }
    vwrite_room_except_both(NULL, user, u,
                            "%s~RS has been placed under arrest...\n",
                            u->recap);
  }
  write_syslog(SYSLOG, 1, "%s ARRESTED %s (at level %s)\n", user->name,
               u->name, user_level[u->arrestby].name);
  add_history(u->name, 1, "Was ~FRarrested~RS by %s (at level ~OL%s~RS).\n",
              user->name, user_level[u->arrestby].name);
  if (!on) {
    u->socket = -2;
    strcpy(u->site, u->last_site);
  }
  save_user_details(u, on);
  done_retrieve(u);
}


/*
 * Unarrest a user who is currently under arrest and in jail
 */
void
unarrest(UR_OBJECT user)
{
  UR_OBJECT u;
  RM_OBJECT rm;
  int on;

  if (word_count < 2) {
    write_user(user, "Usage: unarrest <user>\n");
    return;
  }
  u = retrieve_user(user, word[1]);
  if (!u) {
    return;
  }
  on = retrieve_user_type == 1;
  /* error checks */
  if (u == user) {
    write_user(user, "You cannot unarrest yourself.\n");
    return;
  }
  if (u->level != JAILED) {
    vwrite_user(user, "%s~RS is not under arrest!\n", u->recap);
    done_retrieve(u);
    return;
  }
  if (user->level < u->arrestby) {
    vwrite_user(user, "%s~RS can only be unarrested by a %s or higher.\n",
                u->recap, user_level[u->arrestby].name);
    done_retrieve(u);
    return;
  }
  /* do it */
  u->level = u->unarrest;
  u->real_level = u->level;
  u->arrestby = JAILED;         /* FIXME: Use sentinel other JAILED */
  user_list_level(u->name, u->level);
  strcpy(u->date, (long_date(1)));
  sprintf(text, "~FG~OLYou have been unarrested...  Now try to behave!\n");
  if (!on) {
    send_mail(user, u->name, text, 0);
    vwrite_user(user, "%s has been unarrested.\n", u->name);
  } else {
    write_user(u, text);
    vwrite_user(user, "%s has been unarrested.\n", u->name);
    write_room(NULL, "The Hand of Justice reaches through the air...\n");
    rm = get_room_full(amsys->default_warp);
    if (!rm) {
      vwrite_user(user,
                  "Cannot find a room for ex-cons, so %s~RS is still in the %s!\n",
                  u->recap, u->room->name);
    } else {
      move_user(u, rm, 2);
    }
  }
  write_syslog(SYSLOG, 1, "%s UNARRESTED %s\n", user->name, u->name);
  add_history(u->name, 1, "Was ~FGunarrested~RS by %s.\n", user->name);
  if (!on) {
    u->socket = -2;
    strcpy(u->site, u->last_site);
  }
  save_user_details(u, on);
  done_retrieve(u);
}


/*
 * Change users password. Only GODs and above can change another users
 * password and they do this by specifying the user at the end. When this is
 * done the old password given can be anything, the wiz doesnt have to know it
 * in advance
 */
void
change_pass(UR_OBJECT user)
{
  UR_OBJECT u;
  const char *name;
  int i;

  if (word_count < 3) {
    if (user->level < GOD) {
      write_user(user, "Usage: passwd <old password> <new password>\n");
    } else {
      write_user(user,
                 "Usage: passwd <old password> <new password> [<user>]\n");
    }
    return;
  }
  i = strlen(word[2]);
  if (i < PASS_MIN) {
    write_user(user, "New password too short.\n");
    return;
  }
  /* Via use of crypt() */
  if (i > 8) {
    write_user(user,
               "WARNING: Only the first eight characters of password will be used!\n");
  }
  /* Change own password */
  if (word_count == 3) {
    if (strcmp(user->pass, crypt(word[1], user->pass))) {
      write_user(user, "Old password incorrect.\n");
      return;
    }
    if (!strcmp(word[1], word[2])) {
      write_user(user, "Old and new passwords are the same.\n");
      return;
    }
    strcpy(user->pass, crypt(word[2], crypt_salt));
    save_user_details(user, 0);
    cls(user);
    write_user(user, "Password changed.\n");
    add_history(user->name, 1, "Changed passwords.\n");
    return;
  }
  /* Change someone elses */
  if (user->level < GOD) {
    write_user(user,
               "You are not a high enough level to use the user option.\n");
    return;
  }
  *word[3] = toupper(*word[3]);
  if (!strcmp(word[3], user->name)) {
    /*
       security feature  - prevents someone coming to a wizes terminal and
       changing his password since he wont have to know the old one
     */
    write_user(user,
               "You cannot change your own password using the <user> option.\n");
    return;
  }
  u = get_user(word[3]);
  if (u) {
#ifdef NETLINKS
    if (u->type == REMOTE_TYPE) {
      write_user(user,
                 "You cannot change the password of a user logged on remotely.\n");
      return;
    }
#endif
    if (u->level >= user->level) {
      write_user(user,
                 "You cannot change the password of a user of equal or higher level than yourself.\n");
      return;
    }
    strcpy(u->pass, crypt(word[2], crypt_salt));
    cls(user);
    vwrite_user(user, "%s~RS's password has been changed.\n", u->recap);
    name = user->vis ? user->name : invisname;
    vwrite_user(u, "~FR~OLYour password has been changed by %s!\n", name);
    write_syslog(SYSLOG, 1, "%s changed %s's password.\n", user->name,
                 u->name);
    sprintf(text, "Forced password change by %s.\n", user->name);
    add_history(u->name, 0, "%s", text);
    return;
  }
  u = create_user();
  if (!u) {
    vwrite_user(user, "%s: unable to create temporary user object.\n",
                syserror);
    write_syslog(SYSLOG | ERRLOG, 0,
                 "ERROR: Unable to create temporary user object in change_pass().\n");
    return;
  }
  strcpy(u->name, word[3]);
  if (!load_user_details(u)) {
    write_user(user, nosuchuser);
    destruct_user(u);
    destructed = 0;
    return;
  }
  if (u->level >= user->level) {
    write_user(user,
               "You cannot change the password of a user of equal or higher level than yourself.\n");
    destruct_user(u);
    destructed = 0;
    return;
  }
  strcpy(u->pass, crypt(word[2], crypt_salt));
  save_user_details(u, 0);
  destruct_user(u);
  destructed = 0;
  cls(user);
  vwrite_user(user, "%s's password changed to \"%s\".\n", word[3], word[2]);
  sprintf(text, "Forced password change by %s.\n", user->name);
  add_history(word[3], 0, "%s", text);
  write_syslog(SYSLOG, 1, "%s changed %s's password.\n", user->name, word[3]);
}


/*
 * Kill a user - bump them off the talker
 */
void
kill_user(UR_OBJECT user)
{
  struct kill_mesgs_struct
  {
    const char *victim_msg;
    const char *room_msg;
  };
  static const struct kill_mesgs_struct kill_mesgs[] = {
    {"You are killed\n", "%s is killed\n"},
    {"You have been totally splatted\n", "A hammer splats %s\n"},
    {"The Hedgehog Of Doom runs over you with a car.\n",
     "The Hedgehog Of Doom runs over %s with a car.\n"},
    {"The Inquisitor deletes the worthless, prunes away the wastrels... ie, YOU!", "The Inquisitor prunes away %s.\n"},
    {NULL, NULL},
  };
  UR_OBJECT victim;
  const char *name;
  int msg;

  if (word_count < 2) {
    write_user(user, "Usage: kill <user>\n");
    return;
  }
  victim = get_user_name(user, word[1]);
  if (!victim) {
    write_user(user, notloggedon);
    return;
  }
  if (user == victim) {
    write_user(user,
               "Trying to commit suicide this way is the sixth sign of madness.\n");
    return;
  }
  if (victim->level >= user->level) {
    write_user(user,
               "You cannot kill a user of equal or higher level than yourself.\n");
    vwrite_user(victim, "%s~RS tried to kill you!\n", user->recap);
    return;
  }
  write_syslog(SYSLOG, 1, "%s KILLED %s.\n", user->name, victim->name);
  write_user(user, "~FM~OLYou chant an evil incantation...\n");
  name = user->vis ? user->bw_recap : invisname;
  vwrite_room_except(user->room, user,
                     "~FM~OL%s chants an evil incantation...\n", name);
  /*
     display random kill message.  if you only want one message to be displayed
     then only have one message listed in kill_mesgs[].
   */
  for (msg = 0; kill_mesgs[msg].victim_msg; ++msg) {;
  }
  if (msg) {
    msg = rand() % msg;
    write_user(victim, kill_mesgs[msg].victim_msg);
    vwrite_room_except(victim->room, victim, kill_mesgs[msg].room_msg,
                       victim->bw_recap);
  }
  sprintf(text, "~FRKilled~RS by %s.\n", user->name);
  add_history(victim->name, 1, "%s", text);
  disconnect_user(victim);
  write_monitor(user, NULL, 0);
  write_room(NULL,
             "~FM~OLYou hear insane laughter from beyond the grave...\n");
}


/*
 * Allow a user to delete their own account
 */
void
suicide(UR_OBJECT user)
{
  if (word_count < 2) {
    write_user(user, "Usage: suicide <your password>\n");
    return;
  }
  if (strcmp(user->pass, crypt(word[1], user->pass))) {
    write_user(user, "Password incorrect.\n");
    return;
  }
  write_user(user,
             "\n\07~FR~OL~LI*** WARNING - This will delete your account! ***\n\nAre you sure about this (y|n)? ");
  user->misc_op = 6;
  no_prompt = 1;
}


/*
 * Delete a user
 */
void
delete_user(UR_OBJECT user, int this_user)
{
  char name[USER_NAME_LEN + 1];
  UR_OBJECT u;

  if (this_user) {
    /*
     * User structure gets destructed in disconnect_user(), need to keep a
     * copy of the name
     */
    strcpy(name, user->name);
    write_user(user, "\n~FR~LI~OLACCOUNT DELETED!\n");
    vwrite_room_except(user->room, user, "~OL~LI%s commits suicide!\n",
                       user->name);
    write_syslog(SYSLOG, 1, "%s SUICIDED.\n", name);
    disconnect_user(user);
    clean_files(name);
    rem_user_node(name);
    return;
  }
  if (word_count < 2) {
    write_user(user, "Usage: nuke <user>\n");
    return;
  }
  *word[1] = toupper(*word[1]);
  if (!strcmp(word[1], user->name)) {
    write_user(user,
               "Trying to delete yourself is the eleventh sign of madness.\n");
    return;
  }
  if (get_user(word[1])) {
    /* Safety measure just in case. Will have to .kill them first */
    write_user(user,
               "You cannot delete a user who is currently logged on.\n");
    return;
  }
  u = create_user();
  if (!u) {
    vwrite_user(user, "%s: unable to create temporary user object.\n",
                syserror);
    write_syslog(SYSLOG | ERRLOG, 0,
                 "ERROR: Unable to create temporary user object in delete_user().\n");
    return;
  }
  strcpy(u->name, word[1]);
  if (!load_user_details(u)) {
    write_user(user, nosuchuser);
    destruct_user(u);
    destructed = 0;
    return;
  }
  if (u->level >= user->level) {
    write_user(user,
               "You cannot delete a user of an equal or higher level than yourself.\n");
    destruct_user(u);
    destructed = 0;
    return;
  }
  clean_files(u->name);
  rem_user_node(u->name);
  vwrite_user(user, "\07~FR~OL~LIUser %s deleted!\n", u->name);
  write_syslog(SYSLOG, 1, "%s DELETED %s.\n", user->name, u->name);
  destruct_user(u);
  destructed = 0;
}


/*
 * Purge users that have not been on for the expire length of time
 */
int
purge(int type, char *purge_site, int purge_days)
{
  UR_OBJECT u;
  UD_OBJECT entry, next;

  /* write to syslog and update purge time where needed */
  switch (type) {
  case 0:
    write_syslog(SYSLOG, 1, "PURGE: Executed automatically on default.\n");
    break;
  case 1:
    write_syslog(SYSLOG, 1, "PURGE: Executed manually on default.\n");
    break;
  case 2:
    write_syslog(SYSLOG, 1, "PURGE: Executed manually on site matching.\n");
    write_syslog(SYSLOG, 0, "PURGE: Site given was \"%s\".\n", purge_site);
    break;
  case 3:
    write_syslog(SYSLOG, 1, "PURGE: Executed manually on days given.\n");
    write_syslog(SYSLOG, 0, "PURGE: Days given were \"%d\".\n", purge_days);
    break;
  }
  amsys->purge_count = amsys->purge_skip = amsys->users_purged = 0;
  for (entry = first_user_entry; entry; entry = next) {
    next = entry->next;
    /* do not purge any logged on users */
    if (user_logged_on(entry->name)) {
      ++amsys->purge_skip;
      continue;
    }
    /* if user is not on, then free to check for purging */
    u = create_user();
    if (!u) {
      write_syslog(SYSLOG | ERRLOG, 0,
                   "ERROR: Unable to create temporary user object in purge().\n");
      continue;
    }
    strcpy(u->name, entry->name);
    if (!load_user_details(u)) {
      rem_user_node(u->name);   /* get rid of name from userlist */
      clean_files(u->name);     /* just incase there are any odd files around */
      clean_retire_list(u->name);       /* just incase the user is retired */
      destruct_user(u);
      destructed = 0;
      continue;
    }
    ++amsys->purge_count;
    if (!u->expire) {
      destruct_user(u);
      destructed = 0;
      continue;
    }
    switch (type) {
    case 0:
      /* automatic */
    case 1:
      /* manual default */
      purge_days = u->level == NEW ? NEWBIE_EXPIRES : USER_EXPIRES;
      if (u->last_login + 86400 * purge_days < time(0)) {
        rem_user_node(u->name);
        clean_files(u->name);
        write_syslog(SYSLOG, 0, "PURGE: removed user \"%s\"\n", u->name);
        ++amsys->users_purged;
        destruct_user(u);
        destructed = 0;
        continue;
      }
      break;
    case 2:
      /* purge on site */
      if (pattern_match(u->last_site, purge_site)) {
        rem_user_node(u->name);
        clean_files(u->name);
        write_syslog(SYSLOG, 0, "PURGE: removed user \"%s\"\n", u->name);
        ++amsys->users_purged;
        destruct_user(u);
        destructed = 0;
        continue;
      }
      break;
    case 3:
      /* given amount of days */
      if (u->last_login + 86400 * purge_days < time(0)) {
        rem_user_node(u->name);
        clean_files(u->name);
        write_syslog(SYSLOG, 0, "PURGE: removed user \"%s\"\n", u->name);
        ++amsys->users_purged;
        destruct_user(u);
        destructed = 0;
        continue;
      }
      break;
    }
    /* user not purged */
    destruct_user(u);
    destructed = 0;
  }
  write_syslog(SYSLOG, 0,
               "PURGE: Checked %d user%s (%d skipped), %d %s purged.\n\n",
               amsys->purge_count, PLTEXT_S(amsys->purge_count),
               amsys->purge_skip, amsys->users_purged,
               PLTEXT_WAS(amsys->users_purged));
  return 1;
}


/*
 * allows the user to call the purge function
 */
void
purge_users(UR_OBJECT user)
{
  int exp = 0;

  if (word_count < 2) {
    write_user(user, "Usage: purge [-d] [-s <site>] [-t <days>]\n");
    return;
  }
  if (!strcmp(word[1], "-d")) {
    write_user(user,
               "~OL~FR***~RS Purging users with default settings ~OL~FR***\n");
    purge(1, NULL, 0);
  } else if (!strcmp(word[1], "-s")) {
    if (word_count < 3) {
      write_user(user, "Usage: purge [-d] [-s <site>] [-t <days>]\n");
      return;
    }
    /* check for variations of wild card */
    if (!strcmp("*", word[2])) {
      write_user(user, "You cannot purge users from the site \"*\".\n");
      return;
    }
    if (strstr(word[2], "**")) {
      write_user(user, "You cannot have \"**\" in your site to purge.\n");
      return;
    }
    if (strstr(word[2], "?*")) {
      write_user(user, "You cannot have \"?*\" in your site to purge.\n");
      return;
    }
    if (strstr(word[2], "*?")) {
      write_user(user, "You cannot have \"*?\" in your site to purge.\n");
      return;
    }
    vwrite_user(user,
                "~OL~FR***~RS Purging users with site \"%s\" ~OL~FR***\n",
                word[2]);
    purge(2, word[2], 0);
  } else if (!strcmp(word[1], "-t")) {
    if (word_count < 3) {
      write_user(user, "Usage: purge [-d] [-s <site>] [-t <days>]\n");
      return;
    }
    exp = atoi(word[2]);
    if (exp <= USER_EXPIRES) {
      write_user(user,
                 "You cannot purge users who last logged in less than the default time.\n");
      vwrite_user(user, "The current default is: %d days\n", USER_EXPIRES);
      return;
    }
    if (exp < 0 || exp > 999) {
      write_user(user, "You must enter the amount days from 0-999.\n");
      return;
    }
    vwrite_user(user,
                "~OL~FR***~RS Purging users who last logged in over %d days ago ~OL~FR***\n",
                exp);
    purge(3, NULL, exp);
  } else {
    write_user(user, "Usage: purge [-d] [-s <site>] [-t <days>]\n");
    return;
  }
  /* finished purging - give result */
  vwrite_user(user,
              "Checked ~OL%d~RS user%s (~OL%d~RS skipped), ~OL%d~RS %s purged.  User count is now ~OL%d~RS.\n",
              amsys->purge_count, PLTEXT_S(amsys->purge_count),
              amsys->purge_skip, amsys->users_purged,
              PLTEXT_WAS(amsys->users_purged), amsys->user_count);
}


/*
 * Set a user to either expire after a set time, or never expire
 */
void
user_expires(UR_OBJECT user)
{
  UR_OBJECT u;

  if (word_count < 2) {
    write_user(user, "Usage: expire <user>\n");
    return;
  }
  u = retrieve_user(user, word[1]);
  if (!u) {
    return;
  }
  /* process */
  if (!u->expire) {
    u->expire = 1;
    vwrite_user(user,
                "You have set it so %s will expire when a purge is run.\n",
                u->name);
    sprintf(text, "%s enables expiration with purge.\n", user->name);
    add_history(u->name, 0, "%s", text);
    write_syslog(SYSLOG, 1, "%s enabled expiration on %s.\n", user->name,
                 u->name);
  } else {
    u->expire = 0;
    vwrite_user(user,
                "You have set it so %s will no longer expire when a purge is run.\n",
                u->name);
    sprintf(text, "%s disables expiration with purge.\n", user->name);
    add_history(u->name, 0, "%s", text);
    write_syslog(SYSLOG, 1, "%s disabled expiration on %s.\n", user->name,
                 u->name);
  }
  save_user_details(u, retrieve_user_type == 1);
  done_retrieve(u);
}


/*
 * Create an account for a user if new users from their site have been
 * banned and they want to log on - you know they aint a trouble maker, etc
 */
void
create_account(UR_OBJECT user)
{
  UR_OBJECT u;
  int i;

  if (word_count < 3) {
    write_user(user, "Usage: create <name> <passwd>\n");
    return;
  }
  if (find_user_listed(word[1])) {
    write_user(user,
               "You cannot create with the name of an existing user!\n");
    return;
  }
  for (i = 0; word[1][i]; ++i) {
    if (!isalpha(word[1][i])) {
      break;
    }
  }
  if (word[1][i]) {
    write_user(user,
               "You cannot have anything but letters in the name - account not created.\n\n");
    return;
  }
  if (i < USER_NAME_MIN) {
    write_user(user, "Name was too short--account not created.\n");
    return;
  }
  if (i > USER_NAME_LEN) {
    write_user(user, "Name was too long--account not created.\n");
    return;
  }
  if (contains_swearing(word[1])) {
    write_user(user,
               "You cannot use a name like that--account not created.\n\n");
    return;
  }
  i = strlen(word[2]);
  if (i < PASS_MIN) {
    write_user(user, "Password was too short--account not created.\n");
    return;
  }
  i = strlen(word[2]);
  /* Via use of crypt() */
  if (i > 8) {
    write_user(user,
               "WARNING: Only the first eight characters of password will be used!\n");
  }
  u = create_user();
  if (!u) {
    vwrite_user(user, "%s: unable to create temporary user session.\n",
                syserror);
    write_syslog(SYSLOG | ERRLOG, 0,
                 "ERROR: Unable to create temporary user session in create_account().\n");
    return;
  }
  strtolower(word[1]);
  *word[1] = toupper(*word[1]);
  strcpy(u->name, word[1]);
  if (!load_user_details(u)) {
    strcpy(u->pass, crypt(word[2], crypt_salt));
    strcpy(u->recap, u->name);
    strcpy(u->desc, "is a newbie");
    strcpy(u->in_phrase, "wanders in.");
    strcpy(u->out_phrase, "wanders out");
    strcpy(u->last_site, "created_account");
    strcpy(u->site, u->last_site);
    strcpy(u->logout_room, "<none>");
    *u->verify_code = '\0';
    *u->email = '\0';
    *u->homepage = '\0';
    *u->icq = '\0';
    u->prompt = amsys->prompt_def;
    u->charmode_echo = 0;
    u->room = room_first;
    u->level = NEW;
    u->unarrest = NEW;
    save_user_details(u, 0);
    add_user_node(u->name, u->level);
    add_user_date_node(u->name, (long_date(1)));
    sprintf(text, "Was manually created by %s.\n", user->name);
    add_history(u->name, 1, "%s", text);
    vwrite_user(user,
                "You have created an account with the name \"~FC%s~RS\" and password \"~FG%s~RS\".\n",
                u->name, word[2]);
    write_syslog(SYSLOG, 1, "%s created a new account with the name \"%s\"\n",
                 user->name, u->name);
    destruct_user(u);
    destructed = 0;
    return;
  }
  write_user(user,
             "You cannot create an account with the name of an existing user!\n");
  destruct_user(u);
  destructed = 0;
}


/*
 * Force a save of all the users who are logged on
 */
void
force_save(UR_OBJECT user)
{
  UR_OBJECT u;
  int cnt;

  cnt = 0;
  for (u = user_first; u; u = u->next) {
#ifdef NETLINKS
    if (u->type == REMOTE_TYPE) {
      continue;
    }
#endif
    if (u->type == CLONE_TYPE || u->login) {
      continue;
    }
    ++cnt;
    save_user_details(u, 1);
  }
  write_syslog(SYSLOG, 1, "Manually saved %d users.\n", cnt);
  vwrite_user(user, "You have manually saved %d users.\n", cnt);
}


/*
 * View the system log
 */
void
viewlog(UR_OBJECT user)
{
  static const char emp[] = "This log file is empty.\n\n";
  char logfile[80];
  char dstr[32];
  FILE *fp;
  int lines, cnt, cnt2, type;
  enum lvl_value lvl;
  int c;

  if (word_count < 2) {
    write_user(user,
               "Usage: viewlog sys|net|req|err|<level> [<lines from end>]\n");
    return;
  }
  *logfile = '\0';
  strtoupper(word[1]);
  lvl = get_level(word[1]);
  strftime(dstr, 32, "%Y%m%d", localtime(&amsys->boot_time));
  if (word_count == 2) {
    if (!strcmp(word[1], "SYS")) {
      sprintf(logfile, "%s/%s.%s", LOGFILES, MAINSYSLOG, dstr);
      write_user(user, "\n~BB~FG*** System log ***\n\n");
    } else if (!strcmp(word[1], "NET")) {
      sprintf(logfile, "%s/%s.%s", LOGFILES, NETSYSLOG, dstr);
      write_user(user, "\n~BB~FG*** Netlink log ***\n\n");
    } else if (!strcmp(word[1], "REQ")) {
      sprintf(logfile, "%s/%s.%s", LOGFILES, REQSYSLOG, dstr);
      write_user(user, "\n~BB~FG*** Account Request log ***\n\n");
    } else if (!strcmp(word[1], "ERR")) {
      sprintf(logfile, "%s/%s.%s", LOGFILES, ERRSYSLOG, dstr);
      write_user(user, "\n~BB~FG*** Error log ***\n\n");
    } else if (lvl != NUM_LEVELS) {
      vwrite_user(user, "\n~BB~FG*** User list for level \"%s\" ***\n\n",
                  user_level[lvl].name);
      if (!amsys->level_count[lvl]) {
        write_user(user, emp);
        return;
      }
      user->user_page_lev = lvl;
      switch (more_users(user)) {
      case 0:
        write_user(user, emp);
        return;
      case 1:
        user->misc_op = 16;
        break;
      }
      return;
    } else {
      write_user(user,
                 "Usage: viewlog sys|net|req|err|<level> [<lines from end>]\n");
      return;
    }
    switch (more(user, user->socket, logfile)) {
    case 0:
      write_user(user, emp);
      return;
    case 1:
      user->misc_op = 2;
      break;
    }
    return;
  }
  lines = atoi(word[2]);
  if (lines < 1) {
    write_user(user,
               "Usage: viewlog sys|net|req|err|<level> [<lines from the end>]\n");
    return;
  }
  type = 0;
  /* find out which log */
  if (!strcmp(word[1], "SYS")) {
    sprintf(logfile, "%s/%s.%s", LOGFILES, MAINSYSLOG, dstr);
    type = SYSLOG;
  }
  if (!strcmp(word[1], "NET")) {
    sprintf(logfile, "%s/%s.%s", LOGFILES, NETSYSLOG, dstr);
    type = NETLOG;
  }
  if (!strcmp(word[1], "REQ")) {
    sprintf(logfile, "%s/%s.%s", LOGFILES, REQSYSLOG, dstr);
    type = REQLOG;
  }
  if (!strcmp(word[1], "ERR")) {
    sprintf(logfile, "%s/%s.%s", LOGFILES, ERRSYSLOG, dstr);
    type = ERRLOG;
  }
  if (lvl != NUM_LEVELS) {
    if (!amsys->level_count[lvl]) {
      write_user(user, emp);
      return;
    }
    if (lines > amsys->level_count[lvl]) {
      vwrite_user(user, "There %s only %d line%s in the log.\n",
                  PLTEXT_IS(amsys->level_count[lvl]), amsys->level_count[lvl],
                  PLTEXT_S(amsys->level_count[lvl]));
      return;
    }
    if (lines == amsys->level_count[lvl]) {
      vwrite_user(user, "\n~BB~FG*** User list for level \"%s\" ***\n\n",
                  user_level[lvl].name);
    } else {
      user->user_page_pos = amsys->level_count[lvl] - lines;
      vwrite_user(user,
                  "\n~BB~FG*** User list for level \"%s\" (last %d line%s) ***\n\n",
                  user_level[lvl].name, lines, PLTEXT_S(lines));
    }
    user->user_page_lev = lvl;
    switch (more_users(user)) {
    case 0:
      write_user(user, emp);
      return;
    case 1:
      user->misc_op = 16;
      break;
    }
    return;
  }
  /* count total lines */
  fp = fopen(logfile, "r");
  if (!fp) {
    write_user(user, emp);
    return;
  }
  cnt = 0;
  for (c = getc(fp); c != EOF; c = getc(fp)) {
    if (c == '\n') {
      ++cnt;
    }
  }
  if (cnt < lines) {
    vwrite_user(user, "There %s only %d line%s in the log.\n", PLTEXT_IS(cnt),
                cnt, PLTEXT_S(cnt));
    fclose(fp);
    return;
  }
  if (cnt == lines) {
    switch (type) {
    case SYSLOG:
      write_user(user, "\n~BB~FG*** System log ***\n\n");
      break;
    case NETLOG:
      write_user(user, "\n~BB~FG*** Netlink log ***\n\n");
      break;
    case REQLOG:
      write_user(user, "\n~BB~FG*** Account Request log ***\n\n");
      break;
    case ERRLOG:
      write_user(user, "\n~BB~FG*** Error log ***\n\n");
      break;
    }
    fclose(fp);
    switch (more(user, user->socket, logfile)) {
    case 0:
      break;
    case 1:
      user->misc_op = 2;
      break;
    }
#if !!0
    more(user, user->socket, logfile);
#endif
    return;
  }
  /* Find line to start on */
  fseek(fp, 0, 0);
  cnt2 = 0;
  for (c = getc(fp); c != EOF; c = getc(fp)) {
    if (c != '\n') {
      continue;
    }
    if (++cnt2 == cnt - lines) {
      break;
    }
  }
  if (c == EOF) {
    fclose(fp);
    vwrite_user(user, "%s: Line count error.\n", syserror);
    write_syslog(SYSLOG | ERRLOG, 0,
                 "ERROR: Line count error in viewlog().\n");
    return;
  }
  user->filepos = ftell(fp);
  fclose(fp);
  switch (type) {
  case SYSLOG:
    vwrite_user(user, "\n~BB~FG*** System log (last %d line%s) ***\n\n",
                lines, PLTEXT_S(lines));
    break;
  case NETLOG:
    vwrite_user(user, "\n~BB~FG*** Netlink log (last %d line%s) ***\n\n",
                lines, PLTEXT_S(lines));
    break;
  case REQLOG:
    vwrite_user(user,
                "\n~BB~FG*** Account Request log (last %d line%s) ***\n\n",
                lines, PLTEXT_S(lines));
    break;
  case ERRLOG:
    vwrite_user(user, "\n~BB~FG*** Error log (last %d line%s) ***\n\n", lines,
                PLTEXT_S(lines));
    break;
  }
  if (more(user, user->socket, logfile) != 1) {
    user->filepos = 0;
  } else {
    user->misc_op = 2;
  }
}


/*
 * Display all the people logged on from the same site as user
 */
void
samesite(UR_OBJECT user, int stage)
{
  UR_OBJECT u, u_loop;
  UD_OBJECT entry;
  int found, cnt, same, on;

  on = 0;
  if (!stage) {
    if (word_count < 2) {
      write_user(user, "Usage: samesite user|site [all]\n");
      return;
    }
    if (word_count == 3 && !strcasecmp(word[2], "all")) {
      user->samesite_all_store = 1;
    } else {
      user->samesite_all_store = 0;
    }
    if (!strcasecmp(word[1], "user")) {
      write_user(user, "Enter the name of the user to be checked against: ");
      user->misc_op = 12;
      return;
    }
    if (!strcasecmp(word[1], "site")) {
      write_user(user,
                 "~OL~FRNOTE:~RS Wildcards \"*\" and \"?\" can be used.\n");
      write_user(user, "Enter the site to be checked against: ");
      user->misc_op = 13;
      return;
    }
    write_user(user, "Usage: samesite user|site [all]\n");
    return;
  }
  /* check for users of same site - user supplied */
  if (stage == 1) {
    /* check just those logged on */
    if (!user->samesite_all_store) {
      found = cnt = same = 0;
      u = get_user(user->samesite_check_store);
      if (!u) {
        write_user(user, notloggedon);
        return;
      }
      for (u_loop = user_first; u_loop; u_loop = u_loop->next) {
        ++cnt;
        if (u_loop == u) {
          continue;
        }
        if (!strcmp(u->site, u_loop->site)) {
          ++same;
          if (!found++) {
            vwrite_user(user,
                        "\n~BB~FG*** Users logged on from the same site as ~OL%s~RS~BB~FG ***\n\n",
                        u->name);
          }
          sprintf(text, "    %s %s\n", u_loop->name, u_loop->desc);
          if (u_loop->type == REMOTE_TYPE) {
            text[2] = '@';
          }
          if (!u_loop->vis) {
            text[3] = '*';
          }
          write_user(user, text);
        }
      }
      if (!found) {
        vwrite_user(user,
                    "No users currently logged on have the same site as %s.\n",
                    u->name);
      } else {
        vwrite_user(user,
                    "\nChecked ~FM~OL%d~RS users, ~FM~OL%d~RS had the site as ~FG~OL%s~RS ~FG(%s)\n\n",
                    cnt, same, u->name, u->site);
      }
      return;
    }
    /* check all the users..  First, load the name given */
    u = get_user(user->samesite_check_store);
    on = !!u;
    if (!on) {
      u = create_user();
      if (!u) {
        vwrite_user(user, "%s: unable to create temporary user session.\n",
                    syserror);
        write_syslog(SYSLOG | ERRLOG, 0,
                     "ERROR: Unable to create temporary user session in samesite() - stage 1 of all.\n");
        return;
      }
      strcpy(u->name, user->samesite_check_store);
      if (!load_user_details(u)) {
        destruct_user(u);
        destructed = 0;
        vwrite_user(user, "Sorry, unable to load user file for %s.\n",
                    user->samesite_check_store);
        write_syslog(SYSLOG | ERRLOG, 0,
                     "ERROR: Unable to load user details in samesite() - stage 1 of all.\n");
        return;
      }
    }
    /* read userlist and check against all users */
    found = cnt = same = 0;
    for (entry = first_user_entry; entry; entry = entry->next) {
      *entry->name = toupper(*entry->name);     /* just incase */
      /* create a user object if user not already logged on */
      u_loop = create_user();
      if (!u_loop) {
        write_syslog(SYSLOG | ERRLOG, 0,
                     "ERROR: Unable to create temporary user session in samesite().\n");
        continue;
      }
      strcpy(u_loop->name, entry->name);
      if (!load_user_details(u_loop)) {
        destruct_user(u_loop);
        destructed = 0;
        continue;
      }
      ++cnt;
      if ((on && !strcmp(u->site, u_loop->last_site))
          || (!on && !strcmp(u->last_site, u_loop->last_site))) {
        ++same;
        if (!found++) {
          vwrite_user(user,
                      "\n~BB~FG*** All users from the same site as ~OL%s~RS~BB~FG ***\n\n",
                      u->name);
        }
        vwrite_user(user, "    %s %s\n", u_loop->name, u_loop->desc);
        destruct_user(u_loop);
        destructed = 0;
        continue;
      }
      destruct_user(u_loop);
      destructed = 0;
    }
    if (!found) {
      vwrite_user(user, "No users have the same site as %s.\n", u->name);
    } else {
      if (!on) {
        vwrite_user(user,
                    "\nChecked ~FM~OL%d~RS users, ~FM~OL%d~RS had the site as ~FG~OL%s~RS ~FG(%s)\n\n",
                    cnt, same, u->name, u->last_site);
      } else {
        vwrite_user(user,
                    "\nChecked ~FM~OL%d~RS users, ~FM~OL%d~RS had the site as ~FG~OL%s~RS ~FG(%s)\n\n",
                    cnt, same, u->name, u->site);
      }
    }
    if (!on) {
      destruct_user(u);
      destructed = 0;
    }
    return;
  }
  /* check for users of same site - site supplied */
  if (stage == 2) {
    /* check any wildcards are correct */
    if (strstr(user->samesite_check_store, "**")) {
      write_user(user, "You cannot have ** in your site to check.\n");
      return;
    }
    if (strstr(user->samesite_check_store, "?*")) {
      write_user(user, "You cannot have ?* in your site to check.\n");
      return;
    }
    if (strstr(user->samesite_check_store, "*?")) {
      write_user(user, "You cannot have *? in your site to check.\n");
      return;
    }
    /* check just those logged on */
    if (!user->samesite_all_store) {
      found = cnt = same = 0;
      for (u = user_first; u; u = u->next) {
        ++cnt;
        if (!pattern_match(u->site, user->samesite_check_store)) {
          continue;
        }
        ++same;
        if (!found++) {
          vwrite_user(user,
                      "\n~BB~FG*** Users logged on from the same site as ~OL%s~RS~BB~FG ***\n\n",
                      user->samesite_check_store);
        }
        sprintf(text, "    %s %s\n", u->name, u->desc);
        if (u->type == REMOTE_TYPE) {
          text[2] = '@';
        }
        if (!u->vis) {
          text[3] = '*';
        }
        write_user(user, text);
      }
      if (!found) {
        vwrite_user(user,
                    "No users currently logged on have that same site as %s.\n",
                    user->samesite_check_store);
      } else {
        vwrite_user(user,
                    "\nChecked ~FM~OL%d~RS users, ~FM~OL%d~RS had the site as ~FG~OL%s\n\n",
                    cnt, same, user->samesite_check_store);
      }
      return;
    }
    /* check all the users.. */
    /* open userlist to check against all users */
    found = cnt = same = 0;
    for (entry = first_user_entry; entry; entry = entry->next) {
      *entry->name = toupper(*entry->name);
      /* create a user object if user not already logged on */
      u_loop = create_user();
      if (!u_loop) {
        write_syslog(SYSLOG | ERRLOG, 0,
                     "ERROR: Unable to create temporary user session in samesite().\n");
        continue;
      }
      strcpy(u_loop->name, entry->name);
      if (!load_user_details(u_loop)) {
        destruct_user(u_loop);
        destructed = 0;
        continue;
      }
      ++cnt;
      if ((pattern_match(u_loop->last_site, user->samesite_check_store))) {
        ++same;
        if (!found++) {
          vwrite_user(user,
                      "\n~BB~FG*** All users that have the site ~OL%s~RS~BB~FG ***\n\n",
                      user->samesite_check_store);
        }
        vwrite_user(user, "    %s %s\n", u_loop->name, u_loop->desc);
      }
      destruct_user(u_loop);
      destructed = 0;
    }
    if (!found) {
      vwrite_user(user, "No users have the same site as %s.\n",
                  user->samesite_check_store);
    } else {
      if (!on) {
        vwrite_user(user,
                    "\nChecked ~FM~OL%d~RS users, ~FM~OL%d~RS had the site as ~FG~OL%s~RS\n\n",
                    cnt, same, user->samesite_check_store);
      } else {
        vwrite_user(user,
                    "\nChecked ~FM~OL%d~RS users, ~FM~OL%d~RS had the site as ~FG~OL%s~RS\n\n",
                    cnt, same, user->samesite_check_store);
      }
    }
    return;
  }
}


/*
 * Site a user
 */
void
site(UR_OBJECT user)
{
  UR_OBJECT u;

  if (word_count < 2) {
    write_user(user, "Usage: site <user>\n");
    return;
  }
  u = retrieve_user(user, word[1]);
  if (!u) {
    return;
  }
#ifdef NETLINKS
  /* if the user is remotely connected */
  if (u->type == REMOTE_TYPE) {
    vwrite_user(user, "%s~RS is remotely connected from %s.\n", u->recap,
                u->site);
    done_retrieve(u);
    return;
  }
#endif
  if (retrieve_user_type == 1) {
    vwrite_user(user, "%s~RS is logged in from ~OL~FC%s~RS (%s:%s).\n",
                u->recap, u->site, u->ipsite, u->site_port);
  } else {
    vwrite_user(user, "%s~RS was last logged in from ~OL~FC%s~RS.\n",
                u->recap, u->last_site);
  }
  done_retrieve(u);
}


/*
 * allows a user to add to another users history list
 */
void
manual_history(UR_OBJECT user, char *inpstr)
{
  if (word_count < 3) {
    write_user(user, "Usage: addhistory <user> <text>\n");
    return;
  }
  *word[1] = toupper(*word[1]);
  if (!strcmp(user->name, word[1])) {
    write_user(user, "You cannot add to your own history list.\n");
    return;
  }
  if (!find_user_listed(word[1])) {
    write_user(user, nosuchuser);
    return;
  }
  inpstr = remove_first(inpstr);
  sprintf(text, "%-*s : %s\n", USER_NAME_LEN, user->name, inpstr);
  add_history(word[1], 1, "%s", text);
  vwrite_user(user, "You have added to %s's history list.\n", word[1]);
}


/*
 * shows the history file of a given user
 */
void
user_history(UR_OBJECT user)
{
  char filename[80], name[USER_NAME_LEN + 1];
  UR_OBJECT u;

  if (word_count < 2) {
    write_user(user, "Usage: history <user>\n");
    return;
  }
  u = retrieve_user(user, word[1]);
  if (!u) {
    return;
  }
  strcpy(name, u->name);
  done_retrieve(u);
  /* show file */
  sprintf(filename, "%s/%s/%s.H", USERFILES, USERHISTORYS, name);
  vwrite_user(user,
              "~FG*** The history of user ~OL%s~RS~FG is as follows ***\n\n",
              name);
  switch (more(user, user->socket, filename)) {
  case 0:
    sprintf(text, "%s has no previously recorded history.\n\n", name);
    write_user(user, text);
    break;
  case 1:
    user->misc_op = 2;
    break;
  }
}


/*
 * Turn on and off each individual system log, or globally on and off
 */
void
logging(UR_OBJECT user)
{
  char temp[ARR_SIZE];
  int cnt;

  if (word_count < 2) {
    write_user(user, "Usage: logging -l|-s|-r|-n|-e|-on|-off\n");
    return;
  }
  strtolower(word[1]);
  /* deal with listing the log status first */
  if (!strcmp(word[1], "-l")) {
    write_user(user,
               "\n+----------------------------------------------------------------------------+\n");
    write_user(user,
               "| ~OL~FCSystem log status~RS                                                          |\n");
    write_user(user,
               "+----------------------------------------------------------------------------+\n");
    sprintf(temp,
            "General system log : ~OL%s~RS   Account request logs : ~OL%s~RS   Netlinks log : ~OL%s~RS",
            amsys->logging & SYSLOG ? "ON " : "OFF",
            amsys->logging & REQLOG ? "ON " : "OFF",
            amsys->logging & NETLOG ? "ON " : "OFF");
    cnt = 74 + teslen(temp, 74);
    vwrite_user(user, "| %-*.*s |\n", cnt, cnt, temp);
    sprintf(temp, "         Error log : ~OL%s~RS",
            amsys->logging & ERRLOG ? "ON " : "OFF");
    cnt = 74 + teslen(temp, 74);
    vwrite_user(user, "| %-*.*s |\n", cnt, cnt, temp);
    write_user(user,
               "+----------------------------------------------------------------------------+\n\n");
    return;
  }
  /* (un)set syslog bit */
  if (!strcmp(word[1], "-s")) {
    /* if already on */
    if (amsys->logging & SYSLOG) {
      write_syslog(SYSLOG, 1, "%s switched general system logging OFF.\n",
                   user->name);
    }
    amsys->logging ^= SYSLOG;
    /* if now on */
    if (amsys->logging & SYSLOG) {
      write_syslog(SYSLOG, 1, "%s switched general system logging ON.\n",
                   user->name);
    }
    vwrite_user(user, "You have now turned the general system logging %s.\n",
                amsys->logging & SYSLOG ? "~OL~FGON~RS" : "~OL~FROFF~RS");
    return;
  }
  /* (un)set reqlog bit */
  if (!strcmp(word[1], "-r")) {
    /* if already on */
    if (amsys->logging & REQLOG) {
      write_syslog(REQLOG, 1, "%s switched account request logging OFF.\n",
                   user->name);
    }
    amsys->logging ^= REQLOG;
    /* if now on */
    if (amsys->logging & REQLOG) {
      write_syslog(REQLOG, 1, "%s switched account request logging ON.\n",
                   user->name);
    }
    vwrite_user(user, "You have now turned the account request logging %s.\n",
                amsys->logging & REQLOG ? "~OL~FGON~RS" : "~OL~FROFF~RS");
    return;
  }
  /* (un)set netlog bit */
  if (!strcmp(word[1], "-n")) {
#ifdef NETLINKS
    /* if already on */
    if (amsys->logging & NETLOG) {
      write_syslog(NETLOG, 1, "%s switched netlink logging OFF.\n",
                   user->name);
    }
    amsys->logging ^= NETLOG;
    /* if now on */
    if (amsys->logging & NETLOG) {
      write_syslog(NETLOG, 1, "%s switched netlink logging ON.\n",
                   user->name);
    }
    vwrite_user(user, "You have now turned the netlink logging %s.\n",
                amsys->logging & NETLOG ? "~OL~FGON~RS" : "~OL~FROFF~RS");
#else
    write_user(user, "Netlinks are not currently active.\n");
#endif
    return;
  }
  /* (un)set errlog bit */
  if (!strcmp(word[1], "-e")) {
    /* if already on */
    if (amsys->logging & ERRLOG) {
      write_syslog(ERRLOG, 1, "%s switched error logging OFF.\n", user->name);
    }
    amsys->logging ^= ERRLOG;
    /* if on already */
    if (amsys->logging & ERRLOG) {
      write_syslog(REQLOG, 1, "%s switched error logging ON.\n", user->name);
    }
    vwrite_user(user, "You have now turned the error logging %s.\n",
                amsys->logging & ERRLOG ? "~OL~FGON~RS" : "~OL~FROFF~RS");
    return;
  }
  /* set all bit */
  if (!strcmp(word[1], "-on")) {
    if (!(amsys->logging & SYSLOG)) {
      amsys->logging |= SYSLOG;
      write_syslog(SYSLOG, 1, "%s switched general system logging ON.\n",
                   user->name);
    }
    if (!(amsys->logging & REQLOG)) {
      amsys->logging |= REQLOG;
      write_syslog(REQLOG, 1, "%s switched acount request logging ON.\n",
                   user->name);
    }
    if (!(amsys->logging & NETLOG)) {
      amsys->logging |= NETLOG;
      write_syslog(NETLOG, 1, "%s switched netlink logging ON.\n",
                   user->name);
    }
    if (!(amsys->logging & ERRLOG)) {
      amsys->logging |= ERRLOG;
      write_syslog(ERRLOG, 1, "%s switched error logging ON.\n", user->name);
    }
    write_user(user, "You have now turned all logging ~OL~FGON~RS.\n");
    return;
  }
  /* unset all bit */
  if (!strcmp(word[1], "-off")) {
    if (amsys->logging & SYSLOG) {
      write_syslog(SYSLOG, 1, "%s switched general system logging OFF.\n",
                   user->name);
      amsys->logging &= ~SYSLOG;
    }
    if (amsys->logging & REQLOG) {
      write_syslog(REQLOG, 1, "%s switched acount request logging OFF.\n",
                   user->name);
      amsys->logging &= ~REQLOG;
    }
    if (amsys->logging & NETLOG) {
      write_syslog(NETLOG, 1, "%s switched netlink logging OFF.\n",
                   user->name);
      amsys->logging &= ~NETLOG;
    }
    if (amsys->logging & ERRLOG) {
      write_syslog(ERRLOG, 1, "%s switched error logging OFF.\n", user->name);
      amsys->logging &= ~ERRLOG;
    }
    write_user(user, "You have now turned all logging ~OL~FROFF~RS.\n");
    return;
  }
  write_user(user, "Usage: logging -l|-s|-r|-n|-e|-on|-off\n");
}


/*
 * Set minlogin level
 */
void
minlogin(UR_OBJECT user)
{
  static const char usage[] = "Usage: minlogin -a|-n|<level>\n";
  const char *levstr;
  const char *name;
  UR_OBJECT u, next;
  int cnt;
  enum lvl_value lvl;

  if (word_count < 2) {
    write_user(user, usage);
    if (amsys->stop_logins) {
      write_user(user, "Currently set to ALL\n");
    } else if (amsys->minlogin_level == NUM_LEVELS) {
      write_user(user, "Currently set to NONE\n");
    } else {
      vwrite_user(user, "Currently set to %s\n",
                  user_level[amsys->minlogin_level].name);
    }
    return;
  }
  if (!strcasecmp(word[1], "-a")) {
    amsys->stop_logins = 1;
    write_user(user, "You have now stopped all logins on the user port.\n");
    return;
  }
  if (!strcasecmp(word[1], "-n")) {
    amsys->stop_logins = 0;
    amsys->minlogin_level = NUM_LEVELS;
    write_user(user, "You have now removed the minlogin level.\n");
    return;
  }
  lvl = get_level(word[1]);
  if (lvl == NUM_LEVELS) {
    write_user(user, usage);
    if (amsys->stop_logins) {
      write_user(user, "Currently set to ALL\n");
    } else if (amsys->minlogin_level == NUM_LEVELS) {
      write_user(user, "Currently set to NONE\n");
    } else {
      vwrite_user(user, "Currently set to %s\n",
                  user_level[amsys->minlogin_level].name);
    }
    return;
  } else {
    levstr = user_level[lvl].name;
  }
  if (lvl > user->level) {
    write_user(user,
               "You cannot set minlogin to a higher level than your own.\n");
    return;
  }
  if (amsys->minlogin_level == lvl) {
    write_user(user, "It is already set to that level.\n");
    return;
  }
  amsys->minlogin_level = lvl;
  vwrite_user(user, "Minlogin level set to: ~OL%s.\n", levstr);
  name = user->vis ? user->name : invisname;
  vwrite_room_except(NULL, user, "%s has set the minlogin level to: ~OL%s.\n",
                     name, levstr);
  write_syslog(SYSLOG, 1, "%s set the minlogin level to %s.\n", user->name,
               levstr);
  /* Now boot off anyone below that level */
  cnt = 0;
  if (amsys->boot_off_min) {
    for (u = user_first; u; u = next) {
      next = u->next;
      if (!u->login && u->type != CLONE_TYPE && u->level < lvl) {
        write_user(u,
                   "\n~FY~OLYour level is now below the minlogin level, disconnecting you...\n");
        disconnect_user(u);
        ++cnt;
      }
    }
  }
  if (cnt) {
    vwrite_user(user, "Total of ~OL%d~RS users were disconnected.\n", cnt);
  }
  destructed = 0;
}


/*
 * Show talker system parameters etc
 */
void
system_details(UR_OBJECT user)
{
  static const char *const ca[] =
    { "NONE", "SHUTDOWN", "REBOOT", "SEAMLESS" };
  static const char *const rip[] = { "OFF", "AUTO", "MANUAL", "IDENTD" };
  char bstr[32], foo[ARR_SIZE];
  UR_OBJECT u;
  UD_OBJECT d;
  RM_OBJECT rm;
  CMD_OBJECT cmd;
  size_t l;
  int ucount, dcount, rmcount, cmdcount, lcount;
  int tsize;
  int uccount, rmpcount;
#ifdef NETLINKS
  NL_OBJECT nl;
  int nlcount, nlupcount, nlicount, nlocount, rmnlicount;
#endif
  enum lvl_value lvl;
  int days, hours, mins, secs;

  if (word_count < 2 || !strcasecmp("-a", word[1])) {
    write_user(user,
               "\n+----------------------------------------------------------------------------+\n");
    sprintf(text, "System Details for %s (Amnuts version %s)", TALKER_NAME,
            AMNUTSVER);
    vwrite_user(user, "| ~OL~FC%-74.74s~RS |\n", text);
    write_user(user,
               "|----------------------------------------------------------------------------|\n");
    /* Get some values */
    strftime(bstr, 32, "%a %Y-%m-%d %H:%M:%S", localtime(&amsys->boot_time));
    secs = (int) (time(0) - amsys->boot_time);
    days = secs / 86400;
    hours = (secs % 86400) / 3600;
    mins = (secs % 3600) / 60;
    secs = secs % 60;
    /* Show header parameters */
#ifdef NETLINKS
#ifdef IDENTD
    if (amsys->ident_state) {
#ifdef WIZPORT
      write_user(user,
                 "| talker pid      identd pid      main port      wiz port      netlinks port |\n");
      vwrite_user(user,
                  "|   %-5u           %-5u           %-5.5s         %-5.5s            %-5.5s     |\n",
                  getpid(), amsys->ident_pid, amsys->mport_port,
                  amsys->wport_port, amsys->nlink_port);
#else
      write_user(user,
                 "| talker pid      identd pid      main port      netlinks port               |\n");
      vwrite_user(user,
                  "|   %-5u           %-5u           %-5.5s         %-5.5s                      |\n",
                  getpid(), amsys->ident_pid, amsys->mport_port,
                  amsys->nlink_port);
#endif
      write_user(user,
                 "+----------------------------------------------------------------------------+\n");
    } else
#endif
    {
#ifdef WIZPORT
      write_user(user,
                 "| talker pid           main port            wiz port           netlinks port |\n");
      vwrite_user(user,
                  "|   %-5u               %-5.5s                %-5.5s                 %-5.5s     |\n",
                  getpid(), amsys->mport_port, amsys->wport_port,
                  amsys->nlink_port);
#else
      write_user(user,
                 "| talker pid           main port            netlinks port                    |\n");
      vwrite_user(user,
                  "|   %-5u               %-5.5s                %-5.5s                           |\n",
                  getpid(), amsys->mport_port, amsys->nlink_port);
#endif
      write_user(user,
                 "+----------------------------------------------------------------------------+\n");
    }
#else
#ifdef IDENTD
    if (amsys->ident_state) {
#ifdef WIZPORT
      write_user(user,
                 "| talker pid            identd pid             main port            wiz port |\n");
      vwrite_user(user,
                  "|   %-5u                  %-5u                 %-5.5s                %-5.5s  |\n",
                  getpid(), amsys->ident_pid, amsys->mport_port,
                  amsys->wport_port);
#else
      write_user(user,
                 "| talker pid            identd pid             main port                     |\n");
      vwrite_user(user,
                  "|   %-5u                  %-5u                 %-5.5s                       |\n",
                  getpid(), amsys->ident_pid, amsys->mport_port);
#endif
      write_user(user,
                 "+----------------------------------------------------------------------------+\n");
    } else
#endif
    {
#ifdef WIZPORT
      write_user(user,
                 "| talker pid                       main port                        wiz port |\n");
      vwrite_user(user,
                  "|   %-5u                            %-5.5s                           %-5.5s   |\n",
                  getpid(), amsys->mport_port, amsys->wport_port);
#else
      write_user(user,
                 "| talker pid                       main port                                 |\n");
      vwrite_user(user,
                  "|   %-5u                            %-5.5s                                   |\n",
                  getpid(), amsys->mport_port);
#endif
      write_user(user,
                 "+----------------------------------------------------------------------------+\n");
    }
#endif
    vwrite_user(user, "| %-17.17s : %-54.54s |\n", "talker booted", bstr);
    sprintf(text, "%d day%s, %d hour%s, %d minute%s, %d second%s", days,
            PLTEXT_S(days), hours, PLTEXT_S(hours), mins, PLTEXT_S(mins),
            secs, PLTEXT_S(secs));
    vwrite_user(user, "| %-17.17s : %-54.54s |\n", "uptime", text);
    vwrite_user(user,
                "| %-17.17s : %-6.6s                %-17.17s : %-10.10s   |\n",
                "system logging", offon[(amsys->logging) ? 1 : 0],
                "flood protection", offon[amsys->flood_protect]);
    vwrite_user(user,
                "| %-17.17s : %-6.6s                %-17.17s : %-10.10s   |\n",
                "ignoring sigterms", noyes[amsys->ignore_sigterm],
                "crash action", ca[amsys->crash_action]);
    sprintf(text, "every %d sec%s", amsys->heartbeat,
            PLTEXT_S(amsys->heartbeat));
    vwrite_user(user,
                "| %-17.17s : %-20.20s  %-17.17s : %-10.10s   |\n",
                "heartbeat", text, "resolving IP", rip[amsys->resolve_ip]);
    vwrite_user(user, "| %-17.17s : %-54.54s |\n",
                "swear ban", minmax[amsys->ban_swearing]);
    if (word_count < 2) {
		write_user(user,
				   "+----------------------------------------------------------------------------+\n");
		write_user(user,
				   "| For other options, see: system -m, -n, -r, -u, -a                          |\n");
		write_user(user,
				   "+----------------------------------------------------------------------------+\n\n");
		return;
    }
  }
  /* user option */
  if (!strcasecmp("-u", word[1]) || !strcasecmp("-a", word[1])) {
    uccount = 0;
    for (u = user_first; u; u = u->next) {
      if (u->type == CLONE_TYPE) {
        ++uccount;
      }
    }
	write_user(user,
		   "+----------------------------------------------------------------------------+\n");
    write_user(user,
               "| ~OL~FCSystem Details - Users~RS                                                     |\n");
    write_user(user,
               "|----------------------------------------------------------------------------|\n");
    for (lvl = JAILED; lvl < NUM_LEVELS; lvl = (enum lvl_value) (lvl + 1)) {
      vwrite_user(user,
                  "| users at level %-8.8s : %-5d                                            |\n",
                  user_level[lvl].name, amsys->level_count[lvl]);
    }
    write_user(user,
               "+----------------------------------------------------------------------------+\n");
    vwrite_user(user,
                "| %-24.24s: %-5d       %-24.24s: %-5d      |\n",
                "online now", amsys->num_of_users, "max allowed online",
                amsys->max_users);
    vwrite_user(user, "| %-24.24s: %-5d       %-24.24s: %-5d      |\n",
                "new this boot", amsys->logons_new, "returning this boot",
                amsys->logons_old);
    vwrite_user(user, "| %-24.24s: %-5d       %-24.24s: %-5d      |\n",
                "clones now on", uccount, "max allowed clones",
                amsys->max_clones);
    sprintf(text, "%d sec%s", amsys->login_idle_time,
            PLTEXT_S(amsys->login_idle_time));
    sprintf(foo, "%d sec%s", amsys->user_idle_time,
            PLTEXT_S(amsys->user_idle_time));
    vwrite_user(user, "| %-24.24s: %-10.10s  %-24.24s: %-10.10s |\n",
                "login idle time out", text, "user idle time out", foo);
    vwrite_user(user, "| %-24.24s: %-10.10s  %-24.24s: %-10.10s |\n",
                "time out maxlevel",
                user_level[amsys->time_out_maxlevel].name, "time out afks",
                noyes[amsys->time_out_afks]);
    vwrite_user(user, "| %-24.24s: %-10.10s  %-24.24s: %-10.10s |\n",
                "current minlogin level",
                amsys->minlogin_level ==
                NUM_LEVELS ? "NONE" : user_level[amsys->minlogin_level].name,
                "min login disconnect", noyes[amsys->boot_off_min]);
    vwrite_user(user, "| %-24.24s: %-10.10s  %-24.24s: %-10.10s |\n",
                "newbie prompt default", offon[amsys->prompt_def],
                "newbie colour default", offon[amsys->colour_def]);
    vwrite_user(user, "| %-24.24s: %-10.10s  %-24.24s: %-10.10s |\n",
                "newbie charecho default", offon[amsys->charecho_def],
                "echoing password default", offon[amsys->passwordecho_def]);
    vwrite_user(user, "| %-24.24s: %-10.10s  %-24.24s: %-10.10s |\n",
                "name recaps allowed", noyes[amsys->allow_recaps],
                "smail auto-forwarding", offon[amsys->forwarding]);
    strftime(text, ARR_SIZE * 2, "%a %Y-%m-%d %H:%M:%S",
             localtime(&amsys->auto_purge_date));
    vwrite_user(user, "| %-24.24s: %-4s  %-15.15s: %-25s |\n", "autopurge on",
                noyes[amsys->auto_purge_date != -1], "next autopurge", text);
    sprintf(text, "%d day%s", USER_EXPIRES, PLTEXT_S(USER_EXPIRES));
    sprintf(foo, "%d day%s", NEWBIE_EXPIRES, PLTEXT_S(NEWBIE_EXPIRES));
    vwrite_user(user, "| %-24.24s: %-10.10s  %-24.24s: %-10.10s |\n",
                "purge length (newbies)", foo, "purge length (users)", text);
#ifdef WIZPORT
    vwrite_user(user,
                "| %-24.24s: %-10.10s                                       |\n",
                "wizport min login level",
                user_level[amsys->wizport_level].name);
#endif
    if (!strcasecmp("-u", word[1])) {
		write_user(user,
				   "+----------------------------------------------------------------------------+\n\n");
		return;
    }
  }
  /* Netlinks Option */
  if (!strcasecmp("-n", word[1]) || !strcasecmp("-a", word[1])) {
	write_user(user,
			   "+----------------------------------------------------------------------------+\n");
    write_user(user,
               "| ~OL~FCSystem Details - Netlinks~RS                                                  |\n");
    write_user(user,
               "|----------------------------------------------------------------------------|\n");
#ifdef NETLINKS
    rmnlicount = 0;
    for (rm = room_first; rm; rm = rm->next) {
      if (rm->inlink) {
        ++rmnlicount;
      }
    }
    nlcount = nlupcount = nlicount = nlocount = 0;
    for (nl = nl_first; nl; nl = nl->next) {
      ++nlcount;
      if (nl->type != UNCONNECTED && nl->stage == UP) {
        ++nlupcount;
      }
      if (nl->type == INCOMING) {
        ++nlicount;
      }
      if (nl->type == OUTGOING) {
        ++nlocount;
      }
    }
    vwrite_user(user, "| %-21.21s: %5d %45s |\n",
    			"total netlinks", nlcount, " ");
    vwrite_user(user, "| %-21.21s: %5d secs     %-21.21s: %5d secs    |\n",
    			"idle time out", amsys->net_idle_time,
    			"keepalive interval", amsys->keepalive_interval);
    vwrite_user(user, "| %-21.21s: %5d          %-21.21s: %5d         |\n",
                "accepting connects", rmnlicount,
                "live connects", nlupcount);
    vwrite_user(user, "| %-21.21s: %5d          %-21.21s: %5d         |\n",
                "incoming connections", nlicount,
                "outgoing connections", nlocount);
    vwrite_user(user, "| %-21.21s: %-13.13s  %-21.21s: %-13.13s |\n",
                "remote user maxlevel", user_level[amsys->rem_user_maxlevel].name,
                "remote user deflevel", user_level[amsys->rem_user_deflevel].name);
    vwrite_user(user, "| %-21.21s: %5d bytes    %-21.21s: %5d bytes   |\n",
                "netlink structure", (int) (sizeof *nl),
                "total memory", nlcount * (int) (sizeof *nl));
#else
    write_user(user,
               "| Netlinks are not currently compiled into the talker.                       |\n");
#endif
	if (!strcasecmp("-n", word[1])) {
		write_user(user,
				   "+----------------------------------------------------------------------------+\n\n");
		return;
	}
  }
  /* Room Option */
  if (!strcasecmp("-r", word[1]) || !strcasecmp("-a", word[1])) {
    rmcount = rmpcount = 0;
    for (rm = room_first; rm; rm = rm->next) {
      ++rmcount;
      if (is_personal_room(rm)) {
        ++rmpcount;
      }
    }
	write_user(user,
			   "+----------------------------------------------------------------------------+\n");
    write_user(user,
               "| ~OL~FCSystem Details - Rooms~RS                                                     |\n");
    write_user(user,
               "|----------------------------------------------------------------------------|\n");
    vwrite_user(user,
                "| %-16.16s: %-15.15s      %-20.20s: %5d         |\n",
                "gatecrash level", user_level[amsys->gatecrash_level].name,
                "min private count", amsys->min_private_users);
    sprintf(text, "%d day%s", amsys->mesg_life, PLTEXT_S(amsys->mesg_life));
    vwrite_user(user,
                "| %-16.16s: %-15.15s      %-20.20s: %.2d:%.2d         |\n",
                "message life", text, "message check time",
                amsys->mesg_check_hour, amsys->mesg_check_min);
    vwrite_user(user,
                "| %-16.16s: %5d                %-20.20s: %5d         |\n",
                "personal rooms", rmpcount, "total rooms", rmcount);
    vwrite_user(user, "| %-16.16s: %7d bytes        %-20.20s: %7d bytes |\n",
                "room structure", (int) (sizeof *rm), "total memory",
                rmcount * (int) (sizeof *rm));
    if (!strcasecmp("-r", word[1])) {
		write_user(user,
				   "+----------------------------------------------------------------------------+\n\n");
		return;
    }
  }
  /* Memory Option */
  if (!strcasecmp("-m", word[1]) || !strcasecmp("-a", word[1])) {
    ucount = 0;
    for (u = user_first; u; u = u->next) {
      ++ucount;
    }
    dcount = 0;
    for (d = first_user_entry; d; d = d->next) {
      ++dcount;
    }
    rmcount = 0;
    for (rm = room_first; rm; rm = rm->next) {
      ++rmcount;
    }
    cmdcount = 0;
    for (cmd = first_command; cmd; cmd = cmd->next) {
      ++cmdcount;
    }
    lcount = 0;
    for (l = 0; l < LASTLOGON_NUM; ++l) {
      ++lcount;
    }
    tsize =
      ucount * (sizeof *u) + dcount * (sizeof *d) + rmcount * (sizeof *rm) +
      cmdcount * (sizeof *cmd) + (sizeof *amsys) +
      lcount * (sizeof *last_login_info);
#ifdef NETLINKS
    nlcount = 0;
    for (nl = nl_first; nl; nl = nl->next) {
      ++nlcount;
    }
    tsize += nlcount * (sizeof *nl);
#endif
	write_user(user,
			   "+----------------------------------------------------------------------------+\n");
    write_user(user,
               "| ~OL~FCSystem Details - Memory Object Allocation~RS                                  |\n");
    write_user(user,
               "|----------------------------------------------------------------------------|\n");
    vwrite_user(user,
                "| %-16.16s: %8d * %8d bytes = %8d total bytes         |\n",
                "users", ucount, (int) (sizeof *u),
                ucount * (int) (sizeof *u));
    vwrite_user(user,
                "| %-16.16s: %8d * %8d bytes = %8d total bytes         |\n",
                "all users", dcount, (int) (sizeof *d),
                dcount * (int) (sizeof *d));
    vwrite_user(user,
                "| %-16.16s: %8d * %8d bytes = %8d total bytes         |\n",
                "rooms", rmcount, (int) (sizeof *rm),
                rmcount * (int) (sizeof *rm));
    vwrite_user(user,
                "| %-16.16s: %8d * %8d bytes = %8d total bytes         |\n",
                "commands", cmdcount, (int) (sizeof *cmd),
                cmdcount * (int) (sizeof *cmd));
    vwrite_user(user,
                "| %-16.16s: %8d * %8d bytes = %8d total bytes         |\n",
                "system", 1, (int) (sizeof *amsys),
                1 * (int) (sizeof *amsys));
    vwrite_user(user,
                "| %-16.16s: %8d * %8d bytes = %8d total bytes         |\n",
                "last logins", lcount, (int) (sizeof *last_login_info),
                lcount * (int) (sizeof *last_login_info));
#ifdef NETLINKS
    vwrite_user(user,
                "| %-16.16s: %8d * %8d bytes = %8d total bytes         |\n",
                "netlinks", nlcount, (int) (sizeof *nl),
                nlcount * (int) (sizeof *nl));
#endif
    write_user(user,
               "+----------------------------------------------------------------------------+\n");
    vwrite_user(user,
                "| %-16.16s: %12.3f Mb             %8d total bytes         |\n",
                "total", tsize / 1048576.0, tsize);
    if (!strcasecmp("-m", word[1])) {
		write_user(user,
				   "+----------------------------------------------------------------------------+\n\n");
		return;
    }
  }
  if (!strcasecmp("-a", word[1])) {
	write_user(user,
			   "+----------------------------------------------------------------------------+\n\n");
  } else {
	  write_user(user, "Usage: system [-m|-n|-r|-u|-a]\n");
  }
}


/*
 * Free a hung socket
 */
void
clearline(UR_OBJECT user)
{
  UR_OBJECT u;
  int sock;

  if (word_count < 2 || !is_number(word[1])) {
    write_user(user, "Usage: clearline <line>\n");
    return;
  }
  sock = atoi(word[1]);
  /* Find line amongst users */
  for (u = user_first; u; u = u->next) {
    if (u->type != CLONE_TYPE && u->socket == sock) {
      break;
    }
  }
  if (!u) {
    write_user(user, "That line is not currently active.\n");
    return;
  }
  if (!u->login) {
    write_user(user, "You cannot clear the line of a logged in user.\n");
    return;
  }
  write_user(u, "\n\nThis line is being cleared.\n\n");
  disconnect_user(u);
  write_syslog(SYSLOG, 1, "%s cleared line %d.\n", user->name, sock);
  vwrite_user(user, "Line %d cleared.\n", sock);
  destructed = 0;
  no_prompt = 0;
}


/*
 * Switch swearing ban on and off
 */
void
toggle_swearban(UR_OBJECT user)
{
  switch (amsys->ban_swearing) {
  case SBOFF:
    write_user(user, "Swearing ban now set to ~FGminimum ban~RS.\n");
    amsys->ban_swearing = SBMIN;
    write_syslog(SYSLOG, 1, "%s set swearing ban to MIN.\n", user->name);
    break;
  case SBMIN:
    write_user(user, "Swearing ban now set to ~FRmaximum ban~RS.\n");
    amsys->ban_swearing = SBMAX;
    write_syslog(SYSLOG, 1, "%s set swearing ban to MAX.\n", user->name);
    break;
  case SBMAX:
    write_user(user, "Swearing ban now set to ~FYoff~RS.\n");
    amsys->ban_swearing = SBOFF;
    write_syslog(SYSLOG, 1, "%s set swearing ban to OFF.\n", user->name);
    break;
  }
}


/*
 * Retire a user--i.e., remove from the wizlist but do not alter level
 */
void
retire_user(UR_OBJECT user)
{
  UR_OBJECT u;
  int on;

  if (word_count < 2) {
    write_user(user, "Usage: retire <user>\n");
    return;
  }
  if (is_retired(word[1])) {
    vwrite_user(user, "%s has already been retired from the wizlist.\n",
                word[1]);
    return;
  }
  u = retrieve_user(user, word[1]);
  if (!u) {
    return;
  }
  on = retrieve_user_type == 1;
  if (u == user) {
    write_user(user, "You cannot retire yourself.\n");
    return;
  }
  if (u->level < WIZ) {
    write_user(user, "You cannot retire anyone under the level WIZ\n");
    return;
  }
  u->retired = 1;
  add_retire_list(u->name);
  vwrite_user(user, "You retire %s from the wizlist.\n", u->name);
  write_syslog(SYSLOG, 1, "%s RETIRED %s\n", user->name, u->name);
  sprintf(text, "Was ~FRretired~RS by %s.\n", user->name);
  add_history(u->name, 1, "%s", text);
  sprintf(text,
          "You have been retired from the wizlist but still retain your level.\n");
  if (!on) {
    send_mail(user, u->name, text, 0);
  } else {
    write_user(u, text);
  }
  if (!on) {
    strcpy(u->site, u->last_site);
    u->socket = -2;
  }
  save_user_details(u, on);
  done_retrieve(u);
}


/*
 * Unretire a user--i.e., put them back on show on the wizlist
 */
void
unretire_user(UR_OBJECT user)
{
  UR_OBJECT u;
  int on;

  if (word_count < 2) {
    write_user(user, "Usage: unretire <user>\n");
    return;
  }
  if (!is_retired(word[1])) {
    vwrite_user(user, "%s has not been retired from the wizlist.\n", word[1]);
    return;
  }
  u = retrieve_user(user, word[1]);
  if (!u) {
    return;
  }
  on = retrieve_user_type == 1;
  if (u == user) {
    write_user(user, "You cannot unretire yourself.\n");
    return;
  }
  if (u->level < WIZ) {
    write_user(user, "You cannot retire anyone under the level WIZ.\n");
    return;
  }
  u->retired = 0;
  clean_retire_list(u->name);
  vwrite_user(user, "You unretire %s and put them back on the wizlist.\n",
              u->name);
  write_syslog(SYSLOG, 1, "%s UNRETIRED %s\n", user->name, u->name);
  sprintf(text, "Was ~FGunretired~RS by %s.\n", user->name);
  add_history(u->name, 1, "%s", text);
  sprintf(text, "You have been unretired and put back on the wizlist.\n");
  if (!on) {
    send_mail(user, u->name, text, 0);
  } else {
    write_user(u, text);
  }
  if (!on) {
    strcpy(u->site, u->last_site);
    u->socket = -2;
  }
  save_user_details(u, on);
  done_retrieve(u);
}


/*
 * checks a name to see if it is in the retire list
 */
int
is_retired(const char *name)
{
  UD_OBJECT entry;

  for (entry = first_user_entry; entry; entry = entry->next) {
    if (!strcasecmp(name, entry->name)) {
      break;
    }
  }
  return entry && entry->retired;
}


/*
 * adds a name to the retire list
 */
void
add_retire_list(const char *name)
{
  UD_OBJECT entry;

  for (entry = first_user_entry; entry; entry = entry->next) {
    if (!strcasecmp(name, entry->name)) {
      break;
    }
  }
  if (entry) {
    entry->retired = 1;
  }
}


/*
 * removes a user from the retired list
 */
void
clean_retire_list(const char *name)
{
  UD_OBJECT entry;

  for (entry = first_user_entry; entry; entry = entry->next) {
    if (!strcasecmp(name, entry->name)) {
      break;
    }
  }
  if (entry) {
    entry->retired = 0;
  }
}


/*
 * read all the user files to check if a user exists
 */
void
recount_users(UR_OBJECT user, char *inpstr)
{
  char filename[80], *s;
  DIR *dirp;
  FILE *fp;
  struct dirent *dp;
  UD_OBJECT entry, next;
  UR_OBJECT u;
  int incorrect, correct, added, removed;

  if (!user->misc_op) {
    user->misc_op = 17;
    write_user(user,
               "~OL~FRWARNING:~RS This process may take some time if you have a lot of user accounts.\n");
    write_user(user,
               "         This should only be done if there are no, or minimal, users currently\n         logged on.\n");
    write_user(user, "\nDo you wish to continue (y|n)? ");
    return;
  }
  user->misc_op = 0;
  if (tolower(*inpstr) != 'y') {
    return;
  }
  write_user(user,
             "\n+----------------------------------------------------------------------------+\n");
  incorrect = correct = added = removed = 0;
  write_user(user, "~OLRecounting all of the users...~RS\n");
  /* First process the files to see if there are any to add to the directory listing */
  write_user(user, "Processing users to add...");
  u = create_user();
  if (!u) {
    write_user(user, "ERROR: Cannot create user object.\n");
    write_syslog(SYSLOG | ERRLOG, 1,
                 "ERROR: Cannot create user object in recount_users().\n");
    return;
  }
  /* open the directory file up */
  dirp = opendir(USERFILES);
  if (!dirp) {
    write_user(user, "ERROR: Failed to open userfile directory.\n");
    write_syslog(SYSLOG | ERRLOG, 1,
                 "ERROR: Directory open failure in recount_users().\n");
    return;
  }
  /* count up how many files in the directory - this include . and .. */
  for (dp = readdir(dirp); dp; dp = readdir(dirp)) {
    s = strchr(dp->d_name, '.');
    if (!s || strcmp(s, ".D")) {
      continue;
    }
    *u->name = '\0';
    strncat(u->name, dp->d_name, (size_t) (s - dp->d_name));
    for (entry = first_user_entry; entry; entry = next) {
      next = entry->next;
      if (!strcmp(u->name, entry->name)) {
        break;
      }
    }
    if (!entry) {
      if (load_user_details(u)) {
        add_user_node(u->name, u->level);
        write_syslog(SYSLOG, 0,
                     "Added new user node for existing user \"%s\"\n",
                     u->name);
        ++added;
        reset_user(u);
      }
      /* FIXME: Probably ought to warn about this case */
    } else {
      ++correct;
    }
  }
  closedir(dirp);
  destruct_user(u);
  /*
   * Now process any nodes to remove the directory listing.  This may
   * not be optimal to do one loop to add and then one to remove, but
   * it is the best way I can think of doing it right now at 4:27am!
   */
  write_user(user, "\nProcessing users to remove...");
  for (entry = first_user_entry; entry; entry = next) {
    next = entry->next;
    sprintf(filename, "%s/%s.D", USERFILES, entry->name);
    fp = fopen(filename, "r");
    if (!fp) {
      ++removed;
      --correct;
      write_syslog(SYSLOG, 0,
                   "Removed user node for \"%s\" - user file does not exist.\n",
                   entry->name);
      rem_user_node(entry->name);
    } else {
      fclose(fp);
    }
  }
  write_user(user,
             "\n+----------------------------------------------------------------------------+\n");
  vwrite_user(user,
              "Checked ~OL%d~RS user%s.  ~OL%d~RS node%s %s added, and ~OL%d~RS node%s %s removed.\n",
              added + removed + correct, PLTEXT_S(added + removed + correct),
              added, PLTEXT_S(added), PLTEXT_WAS(added), removed,
              PLTEXT_S(removed), PLTEXT_WAS(removed));
  if (incorrect) {
    write_user(user, "See the system log for further details.\n");
  }
  write_user(user,
             "+----------------------------------------------------------------------------+\n");
}


/*
 * This command allows you to do a search for any user names that match
 * a particular pattern
 */
void
grep_users(UR_OBJECT user)
{
  int found, x;
  char name[USER_NAME_LEN + 1], pat[ARR_SIZE];
  UD_OBJECT entry;

  if (word_count < 2) {
    write_user(user, "Usage: grepu <pattern>\n");
    return;
  }
  if (strstr(word[1], "**")) {
    write_user(user, "You cannot have ** in your pattern.\n");
    return;
  }
  if (strstr(word[1], "?*")) {
    write_user(user, "You cannot have ?* in your pattern.\n");
    return;
  }
  if (strstr(word[1], "*?")) {
    write_user(user, "You cannot have *? in your pattern.\n");
    return;
  }
  start_pager(user);
  write_user(user,
             "\n+----------------------------------------------------------------------------+\n");
  sprintf(text, "| ~FC~OLUser grep for pattern:~RS ~OL%-51s~RS |\n", word[1]);
  write_user(user, text);
  write_user(user,
             "+----------------------------------------------------------------------------+\n");
  x = 0;
  found = 0;
  *pat = '\0';
  strcpy(pat, word[1]);
  strtolower(pat);
  for (entry = first_user_entry; entry; entry = entry->next) {
    strcpy(name, entry->name);
    *name = tolower(*name);
    if (pattern_match(name, pat)) {
      if (!x) {
        vwrite_user(user, "| %-*s  ~FC%-20s~RS   ", USER_NAME_LEN,
                    entry->name, user_level[entry->level].name);
      } else {
        vwrite_user(user, "   %-*s  ~FC%-20s~RS |\n", USER_NAME_LEN,
                    entry->name, user_level[entry->level].name);
      }
      x = !x;
      ++found;
    }
  }
  if (x) {
    write_user(user, "                                      |\n");
  }
  if (!found) {
    write_user(user,
               "|                                                                            |\n");
    write_user(user,
               "| ~OL~FRNo users have that pattern~RS                                                 |\n");
    write_user(user,
               "|                                                                            |\n");
    write_user(user,
               "+----------------------------------------------------------------------------+\n");
    stop_pager(user);
    return;
  }
  write_user(user,
             "+----------------------------------------------------------------------------+\n");
  write_user(user,
             align_string(0, 78, 1, "|",
                          "  ~OL%d~RS user%s had the pattern ~OL%s~RS ",
                          found, PLTEXT_S(found), word[1]));
  write_user(user,
             "+----------------------------------------------------------------------------+\n\n");
  stop_pager(user);
}


/*
 * Allows a user to alter the minimum level which can use the command given
 */
void
set_command_level(UR_OBJECT user)
{
  CMD_OBJECT cmd;
  size_t len;
  enum lvl_value lvl;

  if (word_count < 3) {
    write_user(user, "Usage: setcmdlev <command name> <level>|norm\n");
    return;
  }
  /* FIXME: command search order is different than command_table/exec_com()
   * because it uses the alpha sorted command list instead! */
  len = strlen(word[1]);
  for (cmd = first_command; cmd; cmd = cmd->next) {
    if (!strncmp(word[1], cmd->name, len)) {
      break;
    }
  }
  if (!cmd) {
    vwrite_user(user, "The command \"~OL%s~RS\" could not be found.\n",
                word[1]);
    return;
  }
  /* levels and "norm" are checked in upper case */
  strtoupper(word[2]);
  if (!strcmp(word[2], "NORM")) {
    /* FIXME: Permissions are weak setting level via "norm" */
    if (cmd->level == (enum lvl_value) command_table[cmd->id].level) {
      write_user(user, "That command is already at its normal level.\n");
      return;
    }
    cmd->level = (enum lvl_value) command_table[cmd->id].level;
    write_syslog(SYSLOG, 1,
                 "%s has returned level to normal for cmd \"%s\"\n",
                 user->name, cmd->name);
    write_monitor(user, NULL, 0);
    vwrite_room(NULL,
                "~OL~FR--==<~RS The level for command ~OL%s~RS has been returned to %s ~OL~FR>==--\n",
                cmd->name, user_level[cmd->level].name);
    return;
  }
  lvl = get_level(word[2]);
  if (lvl == NUM_LEVELS) {
    write_user(user, "Usage: setcmdlev <command> <level>|norm\n");
    return;
  }
  if (lvl > user->level) {
    write_user(user,
               "You cannot set a command level to one greater than your own.\n");
    return;
  }
  if (user->level < (enum lvl_value) command_table[cmd->id].level) {
    write_user(user,
               "You are not a high enough level to alter that command level.\n");
    return;
  }
  cmd->level = lvl;
  write_syslog(SYSLOG, 1, "%s has set the level for cmd \"%s\" to %d (%s)\n",
               user->name, cmd->name, cmd->level,
               user_level[cmd->level].name);
  write_monitor(user, NULL, 0);
  vwrite_room(NULL,
              "~OL~FR--==<~RS The level for command ~OL%s~RS has been set to %s ~OL~FR>==--\n",
              cmd->name, user_level[cmd->level].name);
}


/*
 * stop a user from using a certain command
 */
void
user_xcom(UR_OBJECT user)
{
  CMD_OBJECT cmd;
  UR_OBJECT u;
  size_t i, x;

  if (word_count < 2) {
    write_user(user, "Usage: xcom <user> [<command>]\n");
    return;
  }
  u = get_user_name(user, word[1]);
  if (!u) {
    write_user(user, notloggedon);
    return;
  }
  if (u == user) {
    write_user(user, "You cannot ban any commands of your own.\n");
    return;
  }
  /* if no command is given, then just view banned commands */
  if (word_count < 3) {
    write_user(user,
               "+----------------------------------------------------------------------------+\n");
    vwrite_user(user, "~OL~FCBanned commands for user \"%s~RS\"\n", u->recap);
    write_user(user,
               "+----------------------------------------------------------------------------+\n");
    x = 0;
    for (i = 0; i < MAX_XCOMS; ++i) {
      if (u->xcoms[i] == -1) {
        continue;
      }
      for (cmd = first_command; cmd; cmd = cmd->next) {
        if (cmd->id == u->xcoms[i]) {
          break;
        }
      }
      if (!cmd) {
        /* XXX: Maybe emit some sort of error? */
        continue;
      }
      vwrite_user(user, "~OL%s~RS (level %d)\n", cmd->name, cmd->level);
      ++x;
    }
    if (!x) {
      write_user(user, "User has no banned commands.\n");
    }
    write_user(user,
               "+----------------------------------------------------------------------------+\n\n");
    return;
  }
  if (u->level >= user->level) {
    write_user(user,
               "You cannot ban the commands of a user with the same or higher level as yourself.\n");
    return;
  }
  /* FIXME: command search order is different than command_table/exec_com()
   * because it uses the alpha sorted command list instead! */
  i = strlen(word[2]);
  for (cmd = first_command; cmd; cmd = cmd->next) {
    if (!strncmp(word[2], cmd->name, i)) {
      break;
    }
  }
  if (!cmd) {
    write_user(user, "That command does not exist.\n");
    return;
  }
  if (u->level < cmd->level) {
    vwrite_user(user,
                "%s is not of a high enough level to use that command anyway.\n",
                u->name);
    return;
  }
  /* check to see is the user has previously been given the command */
  if (has_gcom(u, cmd->id)) {
    write_user(user,
               "You cannot ban a command that a user has been specifically given.\n");
    return;
  }
  if (has_xcom(u, cmd->id)) {
    /* user already has the command banned, so unban it */
    if (!set_xgcom(user, u, cmd->id, 1, 0)) {
      /* XXX: Maybe emit some sort of error? */
      return;
    }
    vwrite_user(user, "You have unbanned the \"%s\" command for %s\n",
                word[2], u->name);
    vwrite_user(u,
                "The command \"%s\" has been unbanned and you can use it again.\n",
                word[2]);
    sprintf(text, "%s ~FGUNXCOM~RS'd the command \"%s\"\n", user->name,
            word[2]);
    add_history(u->name, 1, "%s", text);
    write_syslog(SYSLOG, 1, "%s UNXCOM'd the command \"%s\" for %s\n",
                 user->name, word[2], u->name);
  } else {
    /* user does not have the command banned, so ban it */
    if (!set_xgcom(user, u, cmd->id, 1, 1)) {
      /* XXX: Maybe emit some sort of error? */
      return;
    }
    vwrite_user(user, "You have banned the \"%s\" command for %s\n", word[2],
                u->name);
    vwrite_user(u, "You have been banned from using the command \"%s\".\n",
                word[2]);
    sprintf(text, "%s ~FRXCOM~RS'd the command \"%s\"\n", user->name,
            word[2]);
    add_history(u->name, 1, "%s", text);
    write_syslog(SYSLOG, 1, "%s XCOM'd the command \"%s\" for %s\n",
                 user->name, word[2], u->name);
  }
}


/*
 * stop a user from using a certain command
 */
void
user_gcom(UR_OBJECT user)
{
  CMD_OBJECT cmd;
  UR_OBJECT u;
  size_t i, x;

  if (word_count < 2) {
    write_user(user, "Usage: gcom <user> [<command>]\n");
    return;
  }
  u = get_user_name(user, word[1]);
  if (!u) {
    write_user(user, notloggedon);
    return;
  }
  if (u == user) {
    write_user(user, "You cannot give yourself any commands.\n");
    return;
  }
  /* if no command is given, then just view given commands */
  if (word_count < 3) {
    write_user(user,
               "+----------------------------------------------------------------------------+\n");
    vwrite_user(user, "~OL~FCGiven commands for user~RS \"%s~RS\"\n",
                u->recap);
    write_user(user,
               "+----------------------------------------------------------------------------+\n");
    x = 0;
    for (i = 0; i < MAX_GCOMS; ++i) {
      if (u->gcoms[i] == -1) {
        continue;
      }
      for (cmd = first_command; cmd; cmd = cmd->next) {
        if (cmd->id == u->gcoms[i]) {
          break;
        }
      }
      if (!cmd) {
        /* XXX: Maybe emit some sort of error? */
        continue;
      }
      vwrite_user(user, "~OL%s~RS (level %d)\n", cmd->name, cmd->level);
      ++x;
    }
    if (!x) {
      write_user(user, "User has no given commands.\n");
    }
    write_user(user,
               "+----------------------------------------------------------------------------+\n\n");
    return;
  }
  if (u->level >= user->level) {
    write_user(user,
               "You cannot give commands to a user with the same or higher level as yourself.\n");
    return;
  }
  /* FIXME: command search order is different than command_table/exec_com()
   * because it uses the alpha sorted command list instead! */
  i = strlen(word[2]);
  for (cmd = first_command; cmd; cmd = cmd->next) {
    if (!strncmp(word[2], cmd->name, i)) {
      break;
    }
  }
  if (!cmd) {
    write_user(user, "That command does not exist.\n");
    return;
  }
  if (u->level >= cmd->level) {
    vwrite_user(user, "%s can already use that command.\n", u->name);
    return;
  }
  if (user->level < cmd->level) {
    write_user(user,
               "You cannot use that command, so you cannot give it to others.\n");
    return;
  }
  /* check to see if the user has previously been banned from using the command */
  if (has_xcom(u, cmd->id)) {
    write_user(user,
               "You cannot give a command to a user that already has it banned.\n");
    return;
  }
  if (has_gcom(u, cmd->id)) {
    /* user already has the command given, so ungive it */
    if (!set_xgcom(user, u, cmd->id, 0, 0)) {
      /* XXX: Maybe emit some sort of error? */
      return;
    }
    vwrite_user(user, "You have removed the given command \"%s\" for %s~RS\n",
                word[2], u->recap);
    vwrite_user(u,
                "Access to the given command \"%s\" has now been taken away from you.\n",
                word[2]);
    sprintf(text, "%s ~FRUNGCOM~RS'd the command \"%s\"\n", user->name,
            word[2]);
    add_history(u->name, 1, "%s", text);
    write_syslog(SYSLOG, 1, "%s UNGCOM'd the command \"%s\" for %s\n",
                 user->name, word[2], u->name);
  } else {
    /* user does not have the command given, so give it */
    if (!set_xgcom(user, u, cmd->id, 0, 1)) {
      /* XXX: Maybe emit some sort of error? */
      return;
    }
    vwrite_user(user, "You have given the \"%s\" command for %s\n", word[2],
                u->name);
    vwrite_user(u, "You have been given access to the command \"%s\".\n",
                word[2]);
    sprintf(text, "%s ~FGGCOM~RS'd the command \"%s\"\n", user->name,
            word[2]);
    add_history(u->name, 1, "%s", text);
    write_syslog(SYSLOG, 1, "%s GCOM'd the command \"%s\" for %s\n",
                 user->name, word[2], u->name);
  }
}


/*
 * set command bans or unbans
 * banned=0 or 1 - 0 is unban and 1 is ban
 * set=0 or 1 - 0 is unset and 1 is set
 */
int
set_xgcom(UR_OBJECT user, UR_OBJECT u, int id, int banned, int set)
{
  char filename[80];
  FILE *fp;
  int *xgcom, key, value;
  size_t cnt, i, max_xgcom;

  if (banned) {
    xgcom = u->xcoms;
    max_xgcom = MAX_XCOMS;
  } else {
    xgcom = u->gcoms;
    max_xgcom = MAX_GCOMS;
  }
  if (set) {
    key = -1;
    value = id;
  } else {
    key = id;
    value = -1;
  }
  for (i = 0; i < max_xgcom; ++i) {
    if (xgcom[i] == key) {
      break;
    }
  }
  if (i >= max_xgcom) {
    if (banned) {
      if (set) {
        vwrite_user(user,
                    "%s has had the maximum amount of commands banned.\n",
                    u->name);
      } else {
        write_user(user, "ERROR: Could not unban that command.\n");
      }
    } else {
      if (set) {
        vwrite_user(user,
                    "%s has had the maximum amount of commands given.\n",
                    u->name);
      } else {
        write_user(user, "ERROR: Could not ungive that command.\n");
      }
    }
    return 0;
  }
  xgcom[i] = value;
  /* write out the commands to a file */
  sprintf(filename, "%s/%s/%s.C", USERFILES, USERCOMMANDS, u->name);
  fp = fopen(filename, "w");
  if (!fp) {
    write_user(user, "ERROR: Unable to open the command list file.\n");
    write_syslog(SYSLOG | ERRLOG, 1,
                 "Unable to open %s's command list in set_xgcom().\n",
                 u->name);
    return 0;
  }
  cnt = 0;
  for (i = 0; i < MAX_XCOMS; ++i) {
    if (u->xcoms[i] == -1) {
      continue;
    }
    fprintf(fp, "0 %d\n", u->xcoms[i]);
    ++cnt;
  }
  for (i = 0; i < MAX_GCOMS; ++i) {
    if (u->gcoms[i] == -1) {
      continue;
    }
    fprintf(fp, "1 %d\n", u->gcoms[i]);
    ++cnt;
  }
  fclose(fp);
  if (!cnt) {
    remove(filename);
  }
  return 1;
}


/*
 * read any banned commands that a user may have
 */
int
get_xgcoms(UR_OBJECT user)
{
  char filename[80];
  FILE *fp;
  int f;
  int tmp;
  int type;
  size_t xi, gi;

  sprintf(filename, "%s/%s/%s.C", USERFILES, USERCOMMANDS, user->name);
  fp = fopen(filename, "r");
  if (!fp) {
    return 0;
  }
  xi = 0;
  gi = 0;
  for (f = fscanf(fp, "%d %d", &type, &tmp); f == 2;
       f = fscanf(fp, "%d %d", &type, &tmp)) {
    if (!type) {
      user->xcoms[xi++] = tmp;
    } else {
      user->gcoms[gi++] = tmp;
    }
  }
  fclose(fp);
  return 1;
}


/*
 * Check to see if a command has been given to a user
 */
int
has_gcom(UR_OBJECT user, int cmd_id)
{
  size_t i;

  for (i = 0; i < MAX_GCOMS; ++i) {
    if (user->gcoms[i] == cmd_id) {
      break;
    }
  }
  return i < MAX_GCOMS;
}


/*
 * Check to see if a command has been taken from a user
 */
int
has_xcom(UR_OBJECT user, int cmd_id)
{
  size_t i;

  for (i = 0; i < MAX_XCOMS; ++i) {
    if (user->xcoms[i] == cmd_id) {
      break;
    }
  }
  return i < MAX_XCOMS;
}


/*
 * bring a user to the same room
 */
void
bring(UR_OBJECT user)
{
  UR_OBJECT u;
  RM_OBJECT rm;

  if (word_count < 2) {
    write_user(user, "Usage: bring <user>\n");
    return;
  }
  u = get_user_name(user, word[1]);
  if (!u) {
    write_user(user, notloggedon);
    return;
  }
  rm = user->room;
  if (user == u) {
    write_user(user,
               "You ~OLreally~RS want to bring yourself?!  What would others think?!\n");
    return;
  }
  if (rm == u->room) {
    vwrite_user(user, "%s~RS is already here!\n", u->recap);
    return;
  }
  if (u->level >= user->level && user->level != GOD) {
    write_user(user,
               "You cannot move a user of equal or higher level that yourself.\n");
    return;
  }
  write_user(user, "You chant a mystic spell...\n");
  if (user->vis) {
    vwrite_room_except(user->room, user, "%s~RS chants a mystic spell...\n",
                       user->recap);
  } else {
    write_monitor(user, user->room, 0);
    vwrite_room_except(user->room, user, "%s chants a mystic spell...\n",
                       invisname);
  }
  move_user(u, rm, 2);
  prompt(u);
}


/*
 * Force a user to do something
 * adapted from Ogham: "Oh God Here's Another MUD" (c) Neil Robertson
 */
void
force(UR_OBJECT user, char *inpstr)
{
  UR_OBJECT u;
  int w;

  if (word_count < 3) {
    write_user(user, "Usage: force <user> <action>\n");
    return;
  }
  *word[1] = toupper(*word[1]);
  u = get_user_name(user, word[1]);
  if (!u) {
    write_user(user, notloggedon);
    return;
  }
  if (u == user) {
    write_user(user, "There is an easier way to do something yourself.\n");
    return;
  }
  if (u->level >= user->level) {
    write_user(user,
               "You cannot force a user of the same or higher level as yourself.\n");
    return;
  }
  inpstr = remove_first(inpstr);
  write_syslog(SYSLOG, 0, "%s FORCED %s to: \"%s\"\n", user->name, u->name,
               inpstr);
  /* shift words down to pass to exec_com */
  word_count -= 2;
  for (w = 0; w < word_count; ++w) {
    strcpy(word[w], word[w + 2]);
  }
  *word[w++] = '\0';
  *word[w++] = '\0';
  vwrite_user(u, "%s~RS forces you to: \"%s\"\n", user->recap, inpstr);
  vwrite_user(user, "You force %s~RS to: \"%s\"\n", u->recap, inpstr);
  if (!exec_com(u, inpstr, COUNT)) {
    vwrite_user(user, "Unable to execute the command for %s~RS.\n", u->recap);
  }
  prompt(u);
}


/*
 * Allows you to dump certain things to files as a record
 */
void
dump_to_file(UR_OBJECT user)
{
  char filename[80], bstr[32];
  FILE *fp;
  UR_OBJECT u;
  UD_OBJECT d;
  RM_OBJECT rm;
  CMD_OBJECT cmd;
  size_t l;
  int ucount, dcount, rmcount, cmdcount, lcount;
  int tsize;
  int drcount;
#ifdef NETLINKS
  NL_OBJECT nl;
  int nlcount;
#endif
  enum lvl_value lvl;
  int days, hours, mins, secs;
  int i, j;

  if (word_count < 2) {
    write_user(user, "Usage: dump -u|-r <rank>|-c|-m|-s\n");
    return;
  }
  strtolower(word[1]);
  /* see if -r switch was used : dump all users of given level */
  if (!strcmp("-r", word[1])) {
    if (word_count < 3) {
      write_user(user, "Usage: dump -r <rank>\n");
      return;
    }
    strtoupper(word[2]);
    lvl = get_level(word[2]);
    if (lvl == NUM_LEVELS) {
      write_user(user, "Usage: dump -r <rank>\n");
      return;
    }
    sprintf(filename, "%s/%s.dump", DUMPFILES, user_level[lvl].name);
    fp = fopen(filename, "w");
    if (!fp) {
      write_user(user,
                 "There was an error trying to open the file to dump to.\n");
      write_syslog(SYSLOG, 0,
                   "Unable to open dump file %s in dump_to_file().\n",
                   filename);
      return;
    }
    fprintf(fp,
            "------------------------------------------------------------------------------\n");
    fprintf(fp, "Users of level %s %s\n", user_level[lvl].name, long_date(1));
    fprintf(fp,
            "------------------------------------------------------------------------------\n");
    drcount = 0;
    for (d = first_user_entry; d; d = d->next) {
      if (d->level != lvl) {
        continue;
      }
      fprintf(fp, "%s\n", d->name);
      ++drcount;
    }
    fprintf(fp,
            "------------------------------------------------------------------------------\n");
    fprintf(fp, "Total users at %s : %d\n", user_level[lvl].name, drcount);
    fprintf(fp,
            "------------------------------------------------------------------------------\n\n");
    fclose(fp);
    sprintf(text,
            "Dumped rank ~OL%s~RS to file.  ~OL%d~RS user%s recorded.\n",
            user_level[lvl].name, drcount, PLTEXT_S(drcount));
    write_user(user, text);
    return;
  }
  /* check to see if -u switch was used : dump all users */
  if (!strcmp("-u", word[1])) {
    sprintf(filename, "%s/users.dump", DUMPFILES);
    fp = fopen(filename, "w");
    if (!fp) {
      write_user(user,
                 "There was an error trying to open the file to dump to.\n");
      write_syslog(SYSLOG, 0,
                   "Unable to open dump file %s in dump_to_file().\n",
                   filename);
      return;
    }
    fprintf(fp,
            "------------------------------------------------------------------------------\n");
    fprintf(fp, "All users %s\n", long_date(1));
    fprintf(fp,
            "------------------------------------------------------------------------------\n");
    dcount = 0;
    for (d = first_user_entry; d; d = d->next) {
      fprintf(fp, "%s\n", d->name);
      ++dcount;
    }
    fprintf(fp,
            "------------------------------------------------------------------------------\n");
    fprintf(fp, "Total users : %d\n", dcount);
    fprintf(fp,
            "------------------------------------------------------------------------------\n\n");
    fclose(fp);
    sprintf(text, "Dumped all users to file.  ~OL%d~RS user%s recorded.\n",
            dcount, PLTEXT_S(dcount));
    write_user(user, text);
    return;
  }
  /* check to see if -c switch was used : dump last few commands used */
  if (!strcmp("-c", word[1])) {
    sprintf(filename, "%s/commands.dump", DUMPFILES);
    fp = fopen(filename, "w");
    if (!fp) {
      write_user(user,
                 "There was an error trying to open the file to dump to.\n");
      write_syslog(SYSLOG, 0,
                   "Unable to open dump file %s in dump_to_file().\n",
                   filename);
      return;
    }
    fprintf(fp,
            "------------------------------------------------------------------------------\n");
    fprintf(fp, "The last 16 commands %s\n", long_date(1));
    fprintf(fp,
            "------------------------------------------------------------------------------\n");
    j = amsys->last_cmd_cnt - 16;
    for (i = j > 0 ? j : 0; i < amsys->last_cmd_cnt; ++i) {
      fprintf(fp, "%s\n", cmd_history[i & 15]);
    }
    fprintf(fp,
            "------------------------------------------------------------------------------\n\n");
    fclose(fp);
    write_user(user, "Dumped the last 16 commands that have been used.\n");
    return;
  }
  /* check to see if -m was used : dump memory currently being used */
  if (!strcmp("-m", word[1])) {
    sprintf(filename, "%s/memory.dump", DUMPFILES);
    fp = fopen(filename, "w");
    if (!fp) {
      write_user(user,
                 "There was an error trying to open the file to dump to.\n");
      write_syslog(SYSLOG, 0,
                   "Unable to open dump file %s in dump_to_file().\n",
                   filename);
      return;
    }
    ucount = 0;
    for (u = user_first; u; u = u->next) {
      ++ucount;
    }
    dcount = 0;
    for (d = first_user_entry; d; d = d->next) {
      ++dcount;
    }
    rmcount = 0;
    for (rm = room_first; rm; rm = rm->next) {
      ++rmcount;
    }
    cmdcount = 0;
    for (cmd = first_command; cmd; cmd = cmd->next) {
      ++cmdcount;
    }
    lcount = 0;
    for (l = 0; l < LASTLOGON_NUM; ++l) {
      ++lcount;
    }
    tsize =
      ucount * (sizeof *u) + rmcount * (sizeof *rm) + dcount * (sizeof *d) +
      cmdcount * (sizeof *cmd) + (sizeof *amsys) +
      lcount * (sizeof *last_login_info);
#ifdef NETLINKS
    nlcount = 0;
    for (nl = nl_first; nl; nl = nl->next) {
      ++nlcount;
    }
    tsize += nlcount * (sizeof *nl);
#endif
    fprintf(fp,
            "------------------------------------------------------------------------------\n");
    fprintf(fp, "Memory Object Allocation %s\n", long_date(1));
    fprintf(fp,
            "------------------------------------------------------------------------------\n");
    fprintf(fp, "  %-16s: %8d * %8d bytes = %8d total bytes\n", "users",
            ucount, (int) (sizeof *u), ucount * (int) (sizeof *u));
    fprintf(fp, "  %-16s: %8d * %8d bytes = %8d total bytes\n", "all users",
            dcount, (int) (sizeof *d), dcount * (int) (sizeof *d));
    fprintf(fp, "  %-16s: %8d * %8d bytes = %8d total bytes\n", "rooms",
            rmcount, (int) (sizeof *rm), rmcount * (int) (sizeof *rm));
    fprintf(fp, "  %-16s: %8d * %8d bytes = %8d total bytes\n", "commands",
            cmdcount, (int) (sizeof *cmd), cmdcount * (int) (sizeof *cmd));
    fprintf(fp, "  %-16s: %8d * %8d bytes = %8d total bytes\n", "system", 1,
            (int) (sizeof *amsys), 1 * (int) (sizeof *amsys));
    fprintf(fp, "  %-16s: %8d * %8d bytes = %8d total bytes\n", "last logins",
            lcount, (int) (sizeof *last_login_info),
            lcount * (int) (sizeof *last_login_info));
#ifdef NETLINKS
    fprintf(fp, "  %-16s: %8d * %8d bytes = %8d total bytes\n",
            "netlinks", nlcount, (int) (sizeof *nl),
            nlcount * (int) (sizeof *nl));
#endif
    fprintf(fp,
            "------------------------------------------------------------------------------\n");
    fprintf(fp, "  %-16s: %12.3f Mb             %8d total bytes\n", "total",
            tsize / 1048576.0, tsize);
    fprintf(fp,
            "------------------------------------------------------------------------------\n");
    fclose(fp);
    write_user(user,
               "Dumped the memory currently being used by the talker.\n");
    return;
  }
  /* check to see if -s switch was used : show system details */
  if (!strcmp("-s", word[1])) {
    sprintf(filename, "%s/system.dump", DUMPFILES);
    fp = fopen(filename, "w");
    if (!fp) {
      write_user(user,
                 "There was an error trying to open the file to dump to.\n");
      write_syslog(SYSLOG, 0,
                   "Unable to open dump file %s in dump_to_file().\n",
                   filename);
      return;
    }
    strftime(bstr, 32, "%a %Y-%m-%d %H:%M:%S", localtime(&amsys->boot_time));
    secs = (int) (time(0) - amsys->boot_time);
    days = secs / 86400;
    hours = (secs % 86400) / 3600;
    mins = (secs % 3600) / 60;
    secs = secs % 60;
    fprintf(fp,
            "------------------------------------------------------------------------------\n");
    fprintf(fp, "System details %s\n", long_date(1));
    fprintf(fp,
            "------------------------------------------------------------------------------\n");
    fprintf(fp, "Node name   : %s\n", amsys->uts.nodename);
    fprintf(fp, "Running on  : %s %s %s %s\n", amsys->uts.machine,
            amsys->uts.sysname, amsys->uts.release, amsys->uts.version);
    fprintf(fp, "Talker PID  : %u\n", getpid());
    fprintf(fp, "Booted      : %s\n", bstr);
    fprintf(fp,
            "Uptime      : %d day%s, %d hour%s, %d minute%s, %d second%s\n",
            days, PLTEXT_S(days), hours, PLTEXT_S(hours), mins,
            PLTEXT_S(mins), secs, PLTEXT_S(secs));
#ifdef NETLINKS
    fprintf(fp, "Netlinks    : Compiled and running\n");
#else
    fprintf(fp, "Netlinks    : Not currently compiled or running\n");
#endif
    switch (amsys->resolve_ip) {
    default:
      fprintf(fp, "IP Resolve  : Off\n");
      break;
    case 1:
      fprintf(fp, "IP Resolve  : On via automatic library\n");
      break;
#ifdef MANDNS
    case 2:
      fprintf(fp, "IP Resolve  : On via manual program\n");
      break;
#endif
#ifdef IDENTD
    case 3:
      fprintf(fp, "IP Resolve  : On via ident daemon\n");
      break;
#endif
    }
    fprintf(fp,
            "------------------------------------------------------------------------------\n\n");
    fclose(fp);
    write_user(user, "Dumped the system details.\n");
    return;
  }
  write_user(user, "Usage: dump -u|-r <rank>|-c|-m|-s\n");
}


/*
 * Change a user name from their existing one to whatever
 */
void
change_user_name(UR_OBJECT user)
{
  char oldname[ARR_SIZE], newname[ARR_SIZE], oldfile[80], newfile[80];
  UR_OBJECT u, usr;
  UD_OBJECT uds;
  const char *name;
  int i, on = 0, self = 0, retired = 0;

  if (word_count < 3) {
    write_user(user, "Usage: cname <old user name> <new user name>\n");
    return;
  }
  /* do a bit of setting up */
  strcpy(oldname, colour_com_strip(word[1]));
  strcpy(newname, colour_com_strip(word[2]));
  strtolower(oldname);
  strtolower(newname);
  *oldname = toupper(*oldname);
  *newname = toupper(*newname);
  /* first check the given attributes */
  if (!find_user_listed(oldname)) {
    write_user(user, nosuchuser);
    return;
  }
  if (find_user_listed(newname)) {
    write_user(user,
               "You cannot change the name to that of an existing user.\n");
    return;
  }
  for (i = 0; newname[i]; ++i) {
    if (!isalpha(newname[i])) {
      break;
    }
  }
  if (newname[i]) {
    write_user(user,
               "You cannot have anything but letters in the new name.\n");
    return;
  }
  if (i < USER_NAME_MIN) {
    write_user(user, "The new name given was too short.\n");
    return;
  }
  if (i > USER_NAME_LEN) {
    write_user(user, "The new name given was too long.\n");
    return;
  }
  if (contains_swearing(newname)) {
    write_user(user, "You cannot use a name that contains swearing.\n");
    return;
  }
  /* See if user is on atm */
  u = get_user(oldname);
  on = !!u;
  if (!on) {
    /* User not logged on */
    u = create_user();
    if (!u) {
      vwrite_user(user, "%s: unable to create temporary user object.\n",
                  syserror);
      write_syslog(SYSLOG | ERRLOG, 0,
                   "ERROR: Unable to create temporary user object in change_user_name().\n");
      return;
    }
    strcpy(u->name, oldname);
    if (!load_user_details(u)) {
      write_user(user, nosuchuser);
      destruct_user(u);
      destructed = 0;
      return;
    }
  }
  if (u == user) {
    self = 1;
  }
  if ((u->level >= user->level) && !self) {
    write_user(user,
               "You cannot change the name of a user with the same or higher level to you.\n");
    if (!on) {
      destruct_user(u);
      destructed = 0;
    }
    return;
  }
  /* everything is ok, so go ahead with change */
  strcpy(u->name, newname);
  strcpy(u->recap, newname);
  strcpy(u->bw_recap, newname);
  /* if user was retired */
  if (is_retired(oldname)) {
    /* XXX: redundant; retire merged into user and user directory list */
    clean_retire_list(oldname);
    retired = 1;
  }
  /* online user list; for clones, etc. */
  for (usr = user_first; usr; usr = usr->next) {
    if (!strcmp(usr->name, oldname)) {
      strcpy(usr->name, newname);
      /* FIXME: Should this change more for clones? */
    }
  }
  /* all users list */
  for (uds = first_user_entry; uds; uds = uds->next) {
    if (!strcmp(uds->name, oldname)) {
      break;
    }
  }
  if (uds) {
    strcpy(uds->name, newname);
  }
  /* if user was retired */
  if (retired) {
    /* XXX: redundant; retire merged into user and user directory list */
    add_retire_list(newname);
  }
  /* change last_login info so that old name if offline */
  record_last_logout(oldname);
  record_last_login(newname);
  /* all memory occurences should be done.  now do files */
  sprintf(oldfile, "%s/%s.D", USERFILES, oldname);
  sprintf(newfile, "%s/%s.D", USERFILES, newname);
  rename(oldfile, newfile);
  sprintf(oldfile, "%s/%s/%s.M", USERFILES, USERMAILS, oldname);
  sprintf(newfile, "%s/%s/%s.M", USERFILES, USERMAILS, newname);
  rename(oldfile, newfile);
  sprintf(oldfile, "%s/%s/%s.P", USERFILES, USERPROFILES, oldname);
  sprintf(newfile, "%s/%s/%s.P", USERFILES, USERPROFILES, newname);
  rename(oldfile, newfile);
  sprintf(oldfile, "%s/%s/%s.H", USERFILES, USERHISTORYS, oldname);
  sprintf(newfile, "%s/%s/%s.H", USERFILES, USERHISTORYS, newname);
  rename(oldfile, newfile);
  sprintf(oldfile, "%s/%s/%s.C", USERFILES, USERCOMMANDS, oldname);
  sprintf(newfile, "%s/%s/%s.C", USERFILES, USERCOMMANDS, newname);
  rename(oldfile, newfile);
  sprintf(oldfile, "%s/%s/%s.MAC", USERFILES, USERMACROS, oldname);
  sprintf(newfile, "%s/%s/%s.MAC", USERFILES, USERMACROS, newname);
  rename(oldfile, newfile);
  sprintf(oldfile, "%s/%s/%s.R", USERFILES, USERROOMS, oldname);
  sprintf(newfile, "%s/%s/%s.R", USERFILES, USERROOMS, newname);
  rename(oldfile, newfile);
  sprintf(oldfile, "%s/%s/%s.B", USERFILES, USERROOMS, oldname);
  sprintf(newfile, "%s/%s/%s.B", USERFILES, USERROOMS, newname);
  rename(oldfile, newfile);
  sprintf(oldfile, "%s/%s/%s.K", USERFILES, USERROOMS, oldname);
  sprintf(newfile, "%s/%s/%s.K", USERFILES, USERROOMS, newname);
  rename(oldfile, newfile);
  sprintf(oldfile, "%s/%s/%s.REM", USERFILES, USERREMINDERS, oldname);
  sprintf(newfile, "%s/%s/%s.REM", USERFILES, USERREMINDERS, newname);
  rename(oldfile, newfile);
  sprintf(oldfile, "%s/%s/%s.U", USERFILES, USERFLAGGED, oldname);
  sprintf(newfile, "%s/%s/%s.U", USERFILES, USERFLAGGED, newname);
  rename(oldfile, newfile);
  /* give results of name change */
  sprintf(text, "Had name changed from ~OL%s~RS by %s~RS.\n", oldname,
          (self ? "self" : user->recap));
  add_history(newname, 1, "%s", text);
  write_syslog(SYSLOG, 1, "%s CHANGED NAME of %s to %s.\n",
               (self ? oldname : user->name), oldname, newname);
  name = user->vis ? user->recap : invisname;
  if (on) {
    if (!self) {
      vwrite_user(u,
                  "\n%s~RS ~FR~OLhas changed your name to \"~RS~OL%s~FR\".\n\n",
                  name, newname);
    }
    write_room_except(u->room,
                      "~OL~FMThere is a shimmering in the air and something pops into existence...\n",
                      u);
    save_user_details(u, 0);
  } else {
    u->socket = -2;
    strcpy(u->site, u->last_site);
    save_user_details(u, 0);
    sprintf(text,
            "~FR~OLYou have had your name changed from ~FY%s~FR to ~FY%s~FR.\nDo not forget to reset your recap if you want.\n",
            oldname, u->name);
    send_mail(user, u->name, text, 0);
    destruct_user(u);
    destructed = 0;
  }
  if (self) {
    vwrite_user(user,
                "You have changed your name from ~OL%s~RS to ~OL%s~RS.\n\n",
                oldname, newname);
  } else {
    vwrite_user(user,
                "You have changed the name of ~OL%s~RS to ~OL%s~RS.\n\n",
                oldname, newname);
  }
}


/*
 * Stop a user from using the go command and leaving the room they are currently in
 */
void
shackle(UR_OBJECT user)
{
  UR_OBJECT u;

  if (word_count < 2) {
    write_user(user, "Usage: shackle <user>\n");
    return;
  }
  u = get_user_name(user, word[1]);
  if (!u) {
    write_user(user, notloggedon);
    return;
  }
  if (user == u) {
    write_user(user, "You cannot shackle yourself!\n");
    return;
  }
#ifdef NETLINKS
  if (!u->room) {
    vwrite_user(user,
                "%s~RS is currently off site and cannot be shackled there.\n",
                u->recap);
    return;
  }
#endif
  if (u->level >= user->level) {
    write_user(user,
               "You cannot shackle someone of the same or higher level as yourself.\n");
    return;
  }
  if (u->lroom == 2) {
    vwrite_user(user, "%s~RS has already been shackled.\n", u->recap);
    return;
  }
  u->lroom = 2;
  vwrite_user(u, "\n~FR~OLYou have been shackled to the %s room.\n",
              u->room->name);
  vwrite_user(user, "~FR~OLYou shackled~RS %s~RS ~FR~OLto the %s room.\n",
              u->recap, u->room->name);
  sprintf(text, "~FRShackled~RS to the ~FB%s~RS room by ~FB~OL%s~RS.\n",
          u->room->name, user->name);
  add_history(u->name, 1, "%s", text);
  write_syslog(SYSLOG, 1, "%s SHACKLED %s to the room: %s\n", user->name,
               u->name, u->room->name);
}


/*
 * Allow a user to move between rooms again
 */
void
unshackle(UR_OBJECT user)
{
  UR_OBJECT u;

  if (word_count < 2) {
    write_user(user, "Usage: unshackle <user>\n");
    return;
  }
  u = get_user_name(user, word[1]);
  if (!u) {
    write_user(user, notloggedon);
    return;
  }
  if (user == u) {
    write_user(user, "You cannot unshackle yourself!\n");
    return;
  }
  if (u->lroom != 2) {
    vwrite_user(user, "%s~RS in not currently shackled.\n", u->recap);
    return;
  }
  u->lroom = 0;
  write_user(u, "\n~FG~OLYou have been unshackled.\n");
  write_user(u,
             "You can now use the ~FCset~RS command to alter the ~FBroom~RS attribute.\n");
  vwrite_user(user, "~FG~OLYou unshackled~RS %s~RS ~FG~OLfrom the %s room.\n",
              u->recap, u->room->name);
  sprintf(text, "~FGUnshackled~RS from the ~FB%s~RS room by ~FB~OL%s~RS.\n",
          u->room->name, user->name);
  add_history(u->name, 1, "%s", text);
  write_syslog(SYSLOG, 1, "%s UNSHACKLED %s from the room: %s\n", user->name,
               u->name, u->room->name);
}
