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
  write_level(WIZ, 1, NORECORD, "~FY<ArIdent Daemon Started>\n", NULL);
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
  write_user(user, "~OLRecounting all of the users...\n");
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
