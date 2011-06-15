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

/***************************************************************************

        reboot.c
        Header file for PARIS By Arnaud Abelard [Arny].
        This reboot system is a "nuts compatible" converted verion of the
        phypors <phypor@benland.muc.edu> reboot system for EW-TOO systems
        by Arnaud Abelard

        With additions by Andrew Collington

***************************************************************************/

static char rebooter[USER_NAME_LEN + 1] = "";   /* use a global for nicer looking code */
static char mrname[ROOM_NAME_LEN + 1] = "";

/*
 * Save the system details
 */
int
build_sysinfo(UR_OBJECT user)
{
  struct talker_system_info tsi;
  FILE *f;

  memset(&tsi, 0, (sizeof tsi));
  strcpy(tsi.mr_name, room_first->name);
  tsi.ban_swearing = amsys->ban_swearing;
  tsi.mport_socket = amsys->mport_socket;
#ifdef WIZPORT
  tsi.wport_socket = amsys->wport_socket;
#endif
#ifdef NETLINKS
  tsi.nlink_socket = amsys->nlink_socket;
#endif
  tsi.boot_time = amsys->boot_time;
  tsi.num_of_logins = amsys->num_of_logins;
  tsi.new_user_logins = amsys->logons_new;
  tsi.old_user_logins = amsys->logons_old;
  if (user) {
    strcpy(tsi.rebooter, user->name);
  }
  f = fopen(TALKER_SYSINFO_FILE, "w");
  if (!f) {
    if (user) {
      vwrite_user(user, "Failed to open reboot system info file [%s]\n",
                  strerror(errno));
    } else {
      write_syslog(SYSLOG | ERRLOG, 0,
                   "ERROR: Failed to open reboot system info file [%s]\n",
                   strerror(errno));
    }
    return -1;
  }
  fwrite(&tsi, (sizeof tsi), 1, f);
  fclose(f);
  return 0;
}

/*
 * Save the user list
 */
int
build_loggedin_users_list(UR_OBJECT user)
{
  FILE *ulf, *rlf;
  UR_OBJECT u;
  int nb;

  /* build user list */
  ulf = fopen(USER_LIST_FILE, "w");
  if (!ulf) {
    if (user) {
      vwrite_user(user, "Failed to open reboot user list file [%s]\n",
                  strerror(errno));
    } else {
      write_syslog(SYSLOG | ERRLOG, 0,
                   "ERROR: Failed to open reboot user list file [%s]\n",
                   strerror(errno));
    }
    return -1;
  }
  rlf = fopen(ROOM_LIST_FILE, "w");
  if (!rlf) {
    fclose(ulf);
    if (user) {
      vwrite_user(user, "Failed to open reboot room list file [%s]\n",
                  strerror(errno));
    } else {
      write_syslog(SYSLOG | ERRLOG, 0,
                   "ERROR: Failed to open reboot room list file [%s]\n",
                   strerror(errno));
    }
    return -1;
  }
  nb = 0;
  for (u = user_first; u; u = u->next) {
    if (!u->login && u->room) {
      fprintf(ulf, "%s %s\n", u->name, u->room->name);
    }
    if (u->room) {
      fprintf(rlf, "%s\n", u->room->name);
      ++nb;
    }
  }
  fclose(ulf);
  fclose(rlf);
  if (!nb) {
    remove(ROOM_LIST_FILE);
  }
  return 0;
}


/*
 * Save the logged in user info
 */
int
build_loggedin_users_info(UR_OBJECT user)
{
  UR_OBJECT u;
  FILE *f;
  char filename[80];

  for (u = user_first; u; u = u->next) {
    if (u->login > 0) {
      write_user(u,
                 "\n\n   Reboot in progress, losing connection.\n Please relogin.\n\n\n");
      shutdown(u->socket, SHUT_WR);
      close(u->socket);
      continue;
    }
    /* save the flagged users info */
    if (build_flagged_user_info(u) < 0) {
      write_user(u,
                 "A reboot is in progress and your flagged users were possibly lost.\n");
    }
    /* save the pager info */
    if (u->pm_first && (build_pager_info(u) < 0)) {
      write_user(u, "A reboot is in progress and your pager was lost.\n");
    }
    /* save the review info */
    if (build_review_buffer_info(u) < 0) {
      write_user(u,
                 "A reboot is in progress and your tell/afk/edit buffer was possibly lost.\n");
    }
    /* save editor information */
    if (u->malloc_start) {
      sprintf(filename, "%s/%s.edit", REBOOTING_DIR, u->name);
      f = fopen(filename, "w");
      if (!f) {
        write_user(u,
                   "A reboot is in progress and your edit buffer was lost.\n");
      } else {
        fwrite(u->malloc_start,
               (size_t) (u->malloc_end - u->malloc_start + 1), 1, f);
        fclose(f);
      }
      memset(u->malloc_start, 0, MAX_LINES * 81);
      free(u->malloc_start);
      u->malloc_start = u->malloc_end = NULL;
    }
    /* finally, save user information */
    sprintf(text, "%s/%s", REBOOTING_DIR, u->name);
    f = fopen(text, "w");
    if (!f) {
      if (user) {
        vwrite_user(user, "Failed to open reboot user info file (%s) [%s]\n",
                    u->name, strerror(errno));
      } else {
        write_syslog(SYSLOG | ERRLOG, 0,
                     "ERROR: Failed to open reboot user info (%s) [%s]\n",
                     u->name, strerror(errno));
      }
      return -1;
    }
    fwrite(u, (sizeof *u), 1, f);
    fclose(f);
  }
  return 0;
}


/*
 * Save user pager position
 */
int
build_pager_info(UR_OBJECT user)
{
  FILE *fp;
  PM_OBJECT t;

  sprintf(text, "%s/%s.pager", REBOOTING_DIR, user->name);
  fp = fopen(text, "w");
  if (!fp) {
    while (user->pm_first) {
      destruct_pag_mesg(user, user->pm_first);
    }
    user->pm_first = user->pm_last = user->pm_current = NULL;
    user->pm_count = 0;
    user->pm_currcount = 0;
    return -1;
  }
  fprintf(fp, "%d\n", user->pm_currcount);
  for (t = user->pm_first; t; t = t->next) {
    fprintf(fp, "%s", t->data);
  }
  while (user->pm_first) {
    destruct_pag_mesg(user, user->pm_first);
  }
  user->pm_first = user->pm_last = user->pm_current = NULL;
  user->pm_count = 0;
  user->pm_currcount = 0;
  fclose(fp);
  return 0;
}


/*
 * Save user flagged user list
 */
int
build_flagged_user_info(UR_OBJECT user)
{
  int bad;

  bad = save_flagged_users(user) ? 0 : -1;
  destruct_all_flagged_users(user);
  user->fu_first = user->fu_last = NULL;
  return bad;
}


/*
 * Save user review buffer list
 */
int
build_review_buffer_info(UR_OBJECT user)
{
  FILE *fp;
  RB_OBJECT rb;

  if (!user->rb_first) {
    return 0;
  }
  sprintf(text, "%s/%s.review", REBOOTING_DIR, user->name);
  fp = fopen(text, "w");
  if (!fp) {
    write_syslog(SYSLOG | ERRLOG, 1,
                 "ERROR: Cannot save flagged user list for %s\n", user->name);
    destruct_all_review_buffer(user);
    return -1;
  }
  for (rb = user->rb_first; rb; rb = rb->next) {
    fprintf(fp, "%s %d %s%s", rb->name, rb->flags, rb->buffer,
            (rb->buffer[strlen(rb->buffer) - 1] == '\n' ? "" : "\n"));
  }
  fclose(fp);
  destruct_all_review_buffer(user);
  return 0;
}


/*
 * Save the user rooms info
 */
int
build_room_info(UR_OBJECT user)
{
  UR_OBJECT u;
  FILE *f;

  for (u = user_first; u; u = u->next) {
    if (u->login) {
      continue;
    }
    if (!u->room) {
      continue;
    }
    sprintf(text, "%s/%s.room", REBOOTING_DIR, u->room->name);
    f = fopen(text, "w");
    if (!f) {
      if (user) {
        vwrite_user(user, "Failed to open reboot room info file (%s) [%s]\n",
                    u->name, strerror(errno));
      } else {
        write_syslog(SYSLOG | ERRLOG, 0,
                     "ERROR: Failed to open reboot room info (%s) [%s]\n",
                     u->name, strerror(errno));
      }
      return -1;
    }
    fwrite(u->room, (sizeof *u->room), 1, f);
    fclose(f);
  }
  return 0;
}


#ifdef IDENTD
/*
 * Save the ident daemon info
 */
int
build_ident_info(void)
{
  FILE *fp;

  sprintf(text, "%s/ident_socket", REBOOTING_DIR);
  fp = fopen(text, "w");
  if (!fp) {
    return -1;
  }
  fprintf(fp, "%d %d %d\n", amsys->ident_socket, amsys->ident_state,
          amsys->ident_pid);
  fclose(fp);
  return 0;
}
#endif


/*
 * Close the open sockets
 */
void
close_fds(void)
{
  int i, d = 0;
  UR_OBJECT u;

  /* FIXME: use sysconf(_SC_OPEN_MAX) */
  for (i = 4; i < (1 << 12); ++i) {     /* start at 4, so we dont kill stdout, etc */
    if (i == amsys->mport_socket) {
      continue;
    }
#ifdef WIZPORT
    if (i == amsys->wport_socket) {
      continue;
    }
#endif
#ifdef NETLINKS
    if (i == amsys->nlink_socket) {
      continue;
    }
#endif
#ifdef IDENTD
    if (i == amsys->ident_socket) {
      continue;
    }
#endif
    for (u = user_first; u; u = u->next) {
      if (u->socket == i) {
        break;
      }
    }
    if (u) {
      continue;
    }
    if (!close(i)) {
      ++d;
    }
  }
}


/*
 * Do the Reboot!
 */
void
do_sreboot(UR_OBJECT user)
{
  static char *args[] = { progname, confile, NULL };
  FILE *f;
  UR_OBJECT u, next;
  int cpid;

  for (u = user_first; u; u = next) {
    next = u->next;
    /* destroy the clones */
    if (u->type == CLONE_TYPE && u->owner) {
      vwrite_user(u->owner,
                  "\nUh-oh.. A seamless reboot happened and your clone in %s went bang.\n",
                  u->room->name);
      vwrite_room(u->room,
                  "The clone of %s~RS is engulfed in magical blue flames and vanishes.\n",
                  u->owner->recap);
      destruct_user(u);
      continue;
    }
    /* do a little fudging */
    u->invite_room = NULL;
    /* now save them */
    save_user_details(u, 1);
  }
  /* save the states */
  if (build_sysinfo(user) < 0) {
    if (user) {
      write_user(user, " Reboot failed (build_sysinfo) ...\n");
    } else {
      write_syslog(SYSLOG | ERRLOG, 1,
                   "ERROR: Reboot failed (build_sysinfo)\n");
    }
    return;
  }
  if (build_loggedin_users_list(user) < 0) {
    if (user) {
      write_user(user, " Reboot failed (build_loggedin_users_list) ...\n");
    } else {
      write_syslog(SYSLOG | ERRLOG, 1,
                   "ERROR: Reboot failed (build_loggedin_users_list)\n");
    }
    return;
  }
  if (build_room_info(user) < 0) {
    if (user) {
      write_user(user, " Reboot failed (build_room_info) ...\n");
    } else {
      write_syslog(SYSLOG | ERRLOG, 1,
                   "ERROR: Reboot failed (build_room_info)\n");
    }
    return;
  }
  if (build_loggedin_users_info(user) < 0) {
    if (user) {
      write_user(user, " Reboot failed (build_loggedin_users_info) ...\n");
    } else {
      write_syslog(SYSLOG | ERRLOG, 1,
                   "ERROR: Reboot failed (build_loggedin_users_info)\n");
    }
    return;
  }
#ifdef IDENTD
  if (build_ident_info() < 0) {
    if (user) {
      write_user(user, " Reboot failed (build_ident_info) ...\n");
    } else {
      write_syslog(SYSLOG | ERRLOG, 1,
                   "ERROR: Reboot failed (build_ident_info)\n");
    }
    return;
  }
#endif

  /*
   * here is where to add for any other syncing that needs to be done
   * when your talker shutsdown... notes, etc
   */

  close_fds();
  /* XXX: Why does this need to fork()?; Find a better reboot proof mechanism */
  cpid = fork();
  switch (cpid) {
  case -1:
    write_syslog(SYSLOG | ERRLOG, 0,
                 "ERROR: Failed to fork() in rebooting\n");
    if (user) {
      write_user(user, " Failed to fork!\n");
    }
    break;
  case 0:
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
          /*** run the background process..and die! ***/
    write_syslog(SYSLOG, 0, "BOOT: Exiting child process.\n");
    break;
  default:                      /* parents thread ... put the childs pid to file, for reboot matching */
    f = fopen(CHILDS_PID_FILE, "w");
    if (!f) {
      write_syslog(SYSLOG | ERRLOG, 0,
                   "ERROR: Failed to fopen [%s] because %s ... reboot fails.\n",
                   CHILDS_PID_FILE, strerror(errno));
    } else {
      fprintf(f, "%d\n", cpid);
      fclose(f);
    }
    exit(0);
  }
}


/******************************************************************************
After the reboot functions!
******************************************************************************/


/*
 * Create a user
 */
UR_OBJECT
create_user_template(UR_OBJECT user)
{
  UR_OBJECT u;

  u = (UR_OBJECT) malloc(sizeof *u);
  if (!u) {
    return u;
  }
  memcpy(u, user, (sizeof *u));
  if (!user_first) {
    user_first = u;
    u->prev = NULL;
  } else {
    user_last->next = u;
    u->prev = user_last;
  }
  u->next = NULL;
  user_last = u;
  return u;
}


/*
 * Create a room
 */
RM_OBJECT
create_room_template(RM_OBJECT room)
{
  RM_OBJECT rm, p, n;

  for (rm = room_first; rm != room_last; rm = rm->next) {
    if (!strcmp(rm->name, room->name)) {
      p = rm->prev;
      n = rm->next;
      memcpy(rm, room, (sizeof *rm));
      rm->prev = p;
      rm->next = n;
      return rm;
    }
  }
  rm = (RM_OBJECT) malloc(sizeof *rm);
  if (!rm) {
    return rm;
  }
  memcpy(rm, room, (sizeof *rm));
  if (!room_first) {
    room_first = rm;
  } else {
    room_last->next = rm;
  }
  rm->next = NULL;
  room_last = rm;
  return rm;
}


/*
 * get the talker system info
 */
int
retrieve_sysinfo(void)
{
  struct talker_system_info tsi;
  FILE *f;

  memset(&tsi, 0, (sizeof tsi));
  f = fopen(TALKER_SYSINFO_FILE, "r");
  if (!f) {
    write_syslog(SYSLOG | ERRLOG, 0,
                 "Failed to open reboot system info file for read [%s]\n",
                 strerror(errno));
    return -1;
  }
  fread(&tsi, (sizeof tsi), 1, f);
  fclose(f);
  strcpy(mrname, tsi.mr_name);
  amsys->ban_swearing = tsi.ban_swearing;
  amsys->mport_socket = tsi.mport_socket;
#ifdef WIZPORT
  amsys->wport_socket = tsi.wport_socket;
#endif
#ifdef NETLINKS
  amsys->nlink_socket = tsi.nlink_socket;
#endif
  amsys->num_of_logins = tsi.num_of_logins;
  amsys->logons_new = tsi.new_user_logins;
  amsys->logons_old = tsi.old_user_logins;
  amsys->boot_time = tsi.boot_time;
  *rebooter = '\0';
  strncat(rebooter, tsi.rebooter, USER_NAME_LEN - 1);
  return 0;
}


/*
 * get the stored room information
 */
void
retrieve_rooms(void)
{
  FILE *pf;
  FILE *f;
  int fc;
  char rmname[ROOM_NAME_LEN];
  struct room_struct rm;

  f = fopen(ROOM_LIST_FILE, "r");
  if (!f) {
    write_syslog(SYSLOG | ERRLOG, 0,
                 "ERROR: When rebooting, in retrieve_rooms, fopen failed.\n");
    return;
  }
  for (fc = fscanf(f, "%s", rmname); fc == 1; fc = fscanf(f, "%s", rmname)) {
    sprintf(text, "%s/%s.room", REBOOTING_DIR, rmname);
    pf = fopen(text, "r");
    if (!pf) {
      write_syslog(SYSLOG | ERRLOG, 0,
                   "ERROR: Failed to open reboot room info for (%s) [%s]\n",
                   rmname, strerror(errno));
      continue;
    }
    fread(&rm, (sizeof rm), 1, pf);
    fclose(pf);
    create_room_template(&rm);
  }
  fclose(f);
}


/*
 * get the stored user information
 */
void
retrieve_users(void)
{
  char name[USER_NAME_LEN], rmname[ROOM_NAME_LEN],
    filename[USER_NAME_LEN + 15], line[ARR_SIZE * 3 + 1], *s;
  struct user_struct spanky;
  FILE *f, *pf;
  UR_OBJECT u;
  RM_OBJECT room;
  PM_OBJECT t;
  int c, pager, fc;

  f = fopen(USER_LIST_FILE, "r");
  if (!f) {
    write_syslog(SYSLOG | ERRLOG, 0,
                 "ERROR: When rebooting, in retrieve_users, fopen failed.\n");
    return;
  }
  for (fc = fscanf(f, "%s %s", name, rmname); fc == 2;
       fc = fscanf(f, "%s %s", name, rmname)) {
    sprintf(text, "%s/%s", REBOOTING_DIR, name);
    pf = fopen(text, "r");
    if (!pf) {
      write_syslog(SYSLOG | ERRLOG, 0,
                   "ERROR: Failed to open reboot user info for (%s) [%s]\n",
                   name, strerror(errno));
      continue;
    }
    /* read in the user */
    fread(&spanky, (sizeof spanky), 1, pf);
    fclose(pf);
    u = create_user_template(&spanky);
    ++amsys->num_of_users;
    u = get_user(name);
    if (!u) {
      continue;
    }
    room = get_room_full(rmname);
    u->room = !room ? room_first : room;
    record_last_login(u->name);
    /* editor buffer */
    sprintf(filename, "%s/%s.edit", REBOOTING_DIR, u->name);
    pf = fopen(filename, "r");
    if (pf) {
      u->malloc_start = (char *) malloc(MAX_LINES * 81);
      if (u->malloc_start) {
        memset(u->malloc_start, 0, MAX_LINES * 81);
        u->malloc_end = u->malloc_start;
        *u->malloc_start = '\0';
        for (c = getc(pf); c != EOF; c = getc(pf)) {
          *u->malloc_end++ = c;
        }
      }
      fclose(pf);
    }
    /* pager buffer */
    sprintf(filename, "%s/%s.pager", REBOOTING_DIR, u->name);
    pf = fopen(filename, "r");
    if (pf) {
      /* get current page count */
      fscanf(pf, "%d\n", &u->pm_currcount);
      /* get data */
      for (s = fgets(line, (sizeof line - 1), pf); s;
           s = fgets(line, (sizeof line - 1), pf)) {
        add_pm(u, s);
      }
      fclose(pf);
      /* position */
      pager = u->pager < MAX_LINES || u->pager > 999 ? 23 : u->pager;
      pager *= u->pm_currcount;
      for (t = u->pm_first; t->next; t = t->next) {
        if (!pager--) {
          break;
        }
      }
      u->pm_current = t;
      /* give them some indication they are in the pager */
      --u->pm_currcount;
      write_apager(u);
    }
    /* flagged users */
    load_flagged_users(u);
    /* review buffer */
    sprintf(filename, "%s/%s.review", REBOOTING_DIR, u->name);
    pf = fopen(filename, "r");
    if (pf) {
      /* get data */
      for (s = fgets(line, (sizeof line - 1), pf); s;
           s = fgets(line, (sizeof line - 1), pf)) {
        if (wordfind(s)) {
          strcpy(text, remove_first(remove_first(s)));
          create_review_buffer_entry(u, word[0], text,
                                     (size_t) atoi(word[1]));
        }
      }
      fclose(pf);
    }
  }
  fclose(f);
}


#ifdef IDENTD
/*
 * get the stored ident info
 */
void
retrieve_ident(void)
{
  FILE *fp;

  sprintf(text, "%s/ident_socket", REBOOTING_DIR);
  fp = fopen(text, "r");
  if (!fp) {
    return;
  }
  fscanf(fp, "%d %d %d", &amsys->ident_socket, &amsys->ident_state,
         &amsys->ident_pid);
  fclose(fp);
}
#endif


/*
 * launch by the child, after the reboot, exec all the fncts of recoveries
 */
int
possibly_reboot(void)
{
  char r[16];
  FILE *f;
  UR_OBJECT u;

  write_syslog(SYSLOG, 0, "BOOT: Checking for a reboot proof...\n");
  f = fopen(CHILDS_PID_FILE, "r");
  if (!f) {
    write_syslog(SYSLOG, 0,
                 "BOOT: Cannot find child's pid\nBOOT: This is not a reboot.\n");
    return 0;                   /* no child pid file, punk */
  }
  memset(r, 0, 16);
  fgets(r, 15, f);
  fclose(f);
  write_syslog(SYSLOG, 0, "BOOT: Reading child's PID... me: %d child: %d\n",
               getpid(), atoi(r));
  if (getpid() != atoi(r)) {
    write_syslog(SYSLOG, 0,
                 "BOOT: I am not the child, this is not a reboot\n");
    return 0;                   /* outdated reboot stuffs */
  }
  write_syslog(SYSLOG, 0,
               "BOOT: I am a reboot child...\n[possible reboot] Rebooting ...\n\n");
  retrieve_sysinfo();
  retrieve_rooms();
  retrieve_users();
#ifdef IDENTD
  retrieve_ident();
#endif

  /* FIXME: Find a better way to do this; rm tries to remove CVS directory */
  sprintf(text, "rm -f %s/*", REBOOTING_DIR);
  system(text);
  time(&amsys->sreb_time);
  if (*rebooter) {
    u = get_user(rebooter);
    if (u) {
      write_level(WIZ, 1, NORECORD, "~OL*** Seamless Reboot complete!! ***\n",
                  NULL);
    } else {
      write_syslog(SYSLOG, 1,
                   "\nThe initiating user [%s] did not survive the reboot.\n",
                   rebooter);
    }
  }
  return 1;
}
