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
#include "rooms.h"

/***************************************************************************/

/*
 * check to see if the room given is a personal room
 */
int
is_personal_room(RM_OBJECT rm)
{
  return !!(rm->access & PERSONAL);
}

/*
 * check to see if the room given is a fixed room
 */
int
is_fixed_room(RM_OBJECT rm)
{
  return !!(rm->access & FIXED);
}

/*
 * check to see if the room given is a private room
 */
int
is_private_room(RM_OBJECT rm)
{
  return !!(rm->access & PRIVATE);
}


/*
 * find out if the room given is the personal room of the user
 */
int
is_my_room(UR_OBJECT user, RM_OBJECT rm)
{
  return !strcmp(user->name, rm->owner);
}


/*
 * check to see how many people are in a given room
 */
int
room_visitor_count(RM_OBJECT rm)
{
  UR_OBJECT u;
  int cnt;

  if (!rm) {
    return 0;
  }
  cnt = 0;
  for (u = user_first; u; u = u->next) {
    if (u->room != rm) {
      continue;
    }
    ++cnt;
  }
  return cnt;
}


/*
 * See if user has a room key
 */
int
has_room_key(const char *visitor, RM_OBJECT rm)
{
  UR_OBJECT u;
  FU_OBJECT fu;
  int haskey;

  /* get owner */
  u = retrieve_user(NULL, rm->owner);
  if (!u) {
    return 0;
  }
  /* check flags */
  for (fu = u->fu_first; fu; fu = fu->next) {
    if (!strcasecmp(fu->name, visitor)) {
      break;
    }
  }
  haskey = fu && (fu->flags & fufROOMKEY);
  done_retrieve(u);
  return haskey;
}


/*
 * Parse the user rooms
 */
void
parse_user_rooms(void)
{
  char dirname[80], name[USER_NAME_LEN + 1], *s;
  DIR *dirp;
  struct dirent *dp;
  RM_OBJECT rm;

  sprintf(dirname, "%s/%s", USERFILES, USERROOMS);
  dirp = opendir(dirname);
  if (!dirp) {
    fprintf(stderr,
            "Amnuts: Directory open failure in parse_user_rooms().\n");
    boot_exit(19);
  }
  /* parse the names of the files but do not include . and .. */
  for (dp = readdir(dirp); dp; dp = readdir(dirp)) {
    s = strchr(dp->d_name, '.');
    if (!s || strcmp(s, ".R")) {
      continue;
    }
    *name = '\0';
    strncat(name, dp->d_name, (size_t) (s - dp->d_name));
    rm = create_room();
    if (!personal_room_store(name, 0, rm)) {
      write_syslog(SYSLOG | ERRLOG, 1,
                   "ERROR: Could not read personal room attributes.  Using standard config.\n");
    }
  }
  closedir(dirp);
}


/*
 * lets a user enter their own room.  It creates the room if !exists
 */
void
personal_room(UR_OBJECT user)
{
  char rmname[ROOM_NAME_LEN + 1], filename[80];
  UR_OBJECT u;
  RM_OBJECT rm;

  if (!amsys->personal_rooms) {
    write_user(user, "Personal room functions are currently disabled.\n");
    return;
  }
  sprintf(rmname, "(%s)", user->name);
  strtolower(rmname);
  rm = get_room_full(rmname);
  /* if the user wants to delete their room */
  if (word_count >= 2) {
    if (strcmp(word[1], "-d")) {
      write_user(user, "Usage: myroom [-d]\n");
      return;
    }
    /* move to the user out of the room if they are in it */
    if (!rm) {
      write_user(user, "You do not have a personal room built.\n");
      return;
    }
    if (room_visitor_count(rm)) {
      write_user(user,
                 "You cannot destroy your room if any people are in it.\n");
      return;
    }
    write_user(user,
               "~OL~FRYou whistle a sharp spell and watch your room crumble into dust.\n");
    /* remove invites */
    for (u = user_first; u; u = u->next) {
      if (u->invite_room == rm) {
        u->invite_room = NULL;
      }
    }
    /* remove all the key flags */
    write_user(user, "~OL~FRAll keys to your room crumble to ashes.\n");
    all_unsetbit_flagged_user_entry(user, fufROOMKEY);
    /* destroy */
    destruct_room(rm);
    /* delete the files */
    sprintf(filename, "%s/%s/%s.R", USERFILES, USERROOMS, user->name);
    remove(filename);
    sprintf(filename, "%s/%s/%s.B", USERFILES, USERROOMS, user->name);
    remove(filename);
    sprintf(filename, "%s/%s/%s.K", USERFILES, USERROOMS, user->name);
    remove(filename);
    write_syslog(SYSLOG, 1, "%s destructed their personal room.\n",
                 user->name);
    return;
  }
  /* if the user is moving to their room */
  if (user->lroom == 2) {
    write_user(user, "You have been shackled and cannot move.\n");
    return;
  }
  /* if room does not exist then create it */
  if (!rm) {
    rm = create_room();
    if (!rm) {
      write_user(user,
                 "Sorry, but your room cannot be created at this time.\n");
      write_syslog(SYSLOG | ERRLOG, 0,
                   "ERROR: Cannot create room for in personal_room()\n");
      return;
    }
    write_user(user, "\nYour room does not exists. Building it now...\n\n");
    /* check to see if the room was just unloaded from memory first */
    if (!personal_room_store(user->name, 0, rm)) {
      write_syslog(SYSLOG, 1, "%s creates their own room.\n", user->name);
      if (!personal_room_store(user->name, 1, rm)) {
        write_syslog(SYSLOG | ERRLOG, 1,
                     "ERROR: Unable to save personal room status in personal_room()\n");
      }
    }
  }
  /* if room just created then should not go in his block */
  if (user->room == rm) {
    write_user(user, "You are already in your own room!\n");
    return;
  }
  move_user(user, rm, 1);
}


/*
 * allows a user to lock their room out to access from anyone
 */
void
personal_room_lock(UR_OBJECT user)
{
  if (!amsys->personal_rooms) {
    write_user(user, "Personal room functions are currently disabled.\n");
    return;
  }
  if (strcmp(user->room->owner, user->name)) {
    write_user(user,
               "You have to be in your personal room to lock and unlock it.\n");
    return;
  }
  user->room->access ^= PRIVATE;
  if (is_private_room(user->room)) {
    write_user(user,
               "You have now ~OL~FRlocked~RS your room to all the other users.\n");
  } else {
    write_user(user,
               "You have now ~OL~FGunlocked~RS your room to all the other users.\n");
  }
  if (!personal_room_store(user->name, 1, user->room))
    write_syslog(SYSLOG | ERRLOG, 1,
                 "ERROR: Unable to save personal room status in personal_room_lock()\n");
}


/*
 * let a user go into another user's personal room if it is unlocked
 */
void
personal_room_visit(UR_OBJECT user)
{
  char rmname[ROOM_NAME_LEN + 1];
  RM_OBJECT rm;

  if (word_count < 2) {
    write_user(user, "Usage: visit <user>\n");
    return;
  }
  if (!amsys->personal_rooms) {
    write_user(user, "Personal room functions are currently disabled.\n");
    return;
  }
  /* check if not same user */
  if (!strcasecmp(user->name, word[1])) {
    vwrite_user(user, "To go to your own room use the \"%s\" command.\n",
                command_table[MYROOM].name);
    return;
  }
  /* see if there is such a user */
  if (!find_user_listed(word[1])) {
    write_user(user, nosuchuser);
    return;
  }
  /* get room to go to */
  sprintf(rmname, "(%s)", word[1]);
  strtolower(rmname);
  rm = get_room_full(rmname);
  if (!rm) {
    write_user(user, nosuchroom);
    return;
  }
  /* can they go there? */
  if (!has_room_access(user, rm)) {
    write_user(user, "That room is currently private, you cannot enter.\n");
    return;
  }
  move_user(user, rm, 1);
}


/*
 * Enter a description for a personal room
 */
void
personal_room_decorate(UR_OBJECT user, char *inpstr)
{
  if (inpstr) {
    if (!amsys->personal_rooms) {
      write_user(user, "Personal room functions are currently disabled.\n");
      return;
    }
    if (strcmp(user->room->owner, user->name)) {
      write_user(user,
                 "You have to be in your personal room to decorate it.\n");
      return;
    }
    if (word_count < 2) {
      write_user(user, "\n~BB*** Decorating your personal room ***\n\n");
      user->misc_op = 19;
      editor(user, NULL);
      return;
    }
    strcat(inpstr, "\n");
  } else {
    inpstr = user->malloc_start;
  }
  *user->room->desc = '\0';
  strncat(user->room->desc, inpstr, ROOM_DESC_LEN);
  if (strlen(user->room->desc) < strlen(inpstr)) {
    vwrite_user(user, "The description is too long for the room \"%s\".\n",
                user->room->name);
  }
  write_user(user, "You have now redecorated your personal room.\n");
  if (!personal_room_store(user->name, 1, user->room)) {
    write_syslog(SYSLOG | ERRLOG, 1,
                 "ERROR: Unable to save personal room status in personal_room_decorate()\n");
  }
}


/*
 * allow a user to bump others from their personal room
 */
void
personal_room_bgone(UR_OBJECT user)
{
  RM_OBJECT rm;
  UR_OBJECT u;

  if (!amsys->personal_rooms) {
    write_user(user, "Personal room functions are currently disabled.\n");
    return;
  }
  if (word_count < 2) {
    write_user(user, "Usage: mybgone <user>/all\n");
    return;
  }
  if (strcmp(user->room->owner, user->name)) {
    write_user(user,
               "You have to be in your personal room to bounce people from it.\n");
    return;
  }
  /* get room to bounce people to */
  rm = get_room_full(amsys->default_warp);
  if (!rm) {
    write_user(user,
               "No one can be bounced from your personal room at this time.\n");
    return;
  }
  strtolower(word[1]);
  /* bounce everyone out - except GODS */
  if (!strcmp(word[1], "all")) {
    for (u = user_first; u; u = u->next) {
      if (u == user || u->room != user->room || u->level == GOD) {
        continue;
      }
      vwrite_user(user, "%s~RS is forced to leave the room.\n", u->recap);
      write_user(u, "You are being forced to leave the room.\n");
      move_user(u, rm, 0);
    }
    return;
  }
  /* send out just the one user */
  u = get_user_name(user, word[1]);
  if (!u) {
    write_user(user, notloggedon);
    return;
  }
  if (u->room != user->room) {
    vwrite_user(user, "%s~RS is not in your personal room.\n", u->recap);
    return;
  }
  if (u->level == GOD) {
    vwrite_user(user, "%s~RS cannot be forced to leave your personal room.\n",
                u->recap);
    return;
  }
  vwrite_user(user, "%s~RS is forced to leave the room.\n", u->recap);
  write_user(u, "You are being forced to leave the room.\n");
  move_user(u, rm, 0);
}


/*
 * save and load personal room information for the user of name given.
 * if store=0 then read info from file else store.
 */
int
personal_room_store(const char *name, int store, RM_OBJECT rm)
{
  FILE *fp;
  char filename[80], line[ARR_SIZE + 1];
  int c, i;

  if (!rm) {
    return 0;
  }
  /* load the info */
  if (!store) {
    strcpy(rm->owner, name);
    strtolower(rm->owner);
    *rm->owner = toupper(*rm->owner);
    sprintf(rm->name, "(%s)", rm->owner);
    strtolower(rm->name);
    rm->link[0] = room_first;
    sprintf(filename, "%s/%s/%s.R", USERFILES, USERROOMS, rm->owner);
    fp = fopen(filename, "r");
    if (!fp) {
      /* if cannot open the file then just put in default attributes */
      rm->access = PERSONAL;
      strcpy(rm->topic, "Welcome to my room!");
      strcpy(rm->show_name, rm->name);
      strcpy(rm->desc, default_personal_room_desc);
      return 0;
    }
    fscanf(fp, "%d\n", &rm->access);
    fgets(line, TOPIC_LEN + 1, fp);
    line[strlen(line) - 1] = '\0';
    if (!strcmp(line, "#UNSET")) {
      *rm->topic = '\0';
    } else {
      strcpy(rm->topic, line);
    }
    fgets(line, PERSONAL_ROOMNAME_LEN + 1, fp);
    line[strlen(line) - 1] = '\0';
    strcpy(rm->show_name, line);
    i = 0;
    for (c = getc(fp); c != EOF; c = getc(fp)) {
      if (i == ROOM_DESC_LEN) {
        break;
      }
      rm->desc[i++] = c;
    }
    if (c != EOF) {
      write_syslog(SYSLOG | ERRLOG, 0,
		   "ERROR: Description too long when reloading for room %s.\n",
		   rm->name);
    }
    rm->desc[i] = '\0';
    fclose(fp);
    return 1;
  }
  /* save info */
  sprintf(filename, "%s/%s/%s.R", USERFILES, USERROOMS, rm->owner);
  fp = fopen(filename, "w");
  if (!fp) {
    return 0;
  }
  fprintf(fp, "%d\n", rm->access);
  fprintf(fp, "%s\n", !*rm->topic ? "#UNSET" : rm->topic);
  fprintf(fp, "%s\n", rm->show_name);
  for (i = 0; rm->desc[i]; ++i) {
    putc(rm->desc[i], fp);
  }
  fclose(fp);
  return 1;
}


/*
 * this function allows admin to control personal rooms
 */
void
personal_room_admin(UR_OBJECT user)
{
  char rmname[ROOM_NAME_LEN + 1], filename[80];
  RM_OBJECT rm;
  UR_OBJECT u;
  int trsize, rmcount, locked, unlocked;

  if (word_count < 2) {
    write_user(user, "Usage: rmadmin -l / -m / -u <name> / -d <name>\n");
    return;
  }
  if (!amsys->personal_rooms) {
    write_user(user, "Personal room functions are currently disabled.\n");
    return;
  }
  strtolower(word[1]);
  /* just display the amount of memory used by personal rooms */
  if (!strcmp(word[1], "-m")) {
    write_user(user,
               "+----------------------------------------------------------------------------+\n");
    write_user(user,
               "| ~FC~OLPersonal Room Memory Usage~RS                                                 |\n");
    write_user(user,
               "+----------------------------------------------------------------------------+\n");
    rmcount = locked = unlocked = 0;
    for (rm = room_first; rm; rm = rm->next) {
      if (!is_personal_room(rm)) {
        continue;
      }
      ++rmcount;
      if (is_private_room(rm)) {
        ++locked;
      } else {
        ++unlocked;
      }
    }
    trsize = rmcount * (sizeof *rm);
    vwrite_user(user,
                "| %-15.15s: ~OL%2d~RS locked, ~OL%2d~RS unlocked                                    |\n",
                "status", locked, unlocked);
    vwrite_user(user,
                "| %-15.15s: ~OL%5d~RS * ~OL%8d~RS bytes = ~OL%8d~RS total bytes  (%02.3f Mb) |\n",
                "rooms", rmcount, (int) (sizeof *rm), trsize,
                trsize / 1048576.0);
    write_user(user,
               "+----------------------------------------------------------------------------+\n");
    return;
  }
  /* list all the personal rooms in memory together with status */
  if (!strcmp(word[1], "-l")) {
    write_user(user,
               "+----------------------------------------------------------------------------+\n");
    write_user(user,
               "| ~OL~FCPersonal Room Listings~RS                                                     |\n");
    write_user(user,
               "+----------------------------------------------------------------------------+\n");
    rmcount = 0;
    for (rm = room_first; rm; rm = rm->next) {
      if (!is_personal_room(rm)) {
        continue;
      }
      ++rmcount;
      vwrite_user(user,
                  "| Owner : ~OL%-*.*s~RS       Status : ~OL%s~RS   Msg Count : ~OL%2d~RS  People : ~OL%2d~RS |\n",
                  USER_NAME_LEN, USER_NAME_LEN, rm->owner,
                  is_private_room(rm) ? "~FRlocked  " : "~FGunlocked",
                  rm->mesg_cnt, room_visitor_count(rm));
    }
    if (!rmcount) {
      write_user(user,
                 "| ~FRNo personal rooms are currently in memory~RS                                  |\n");
    }
    write_user(user,
               "+----------------------------------------------------------------------------+\n");
    if (rmcount) {
      vwrite_user(user,
                  "| Total personal rooms : ~OL~FM%2d~RS                                                  |\n",
                  rmcount);
      write_user(user,
                 "+----------------------------------------------------------------------------+\n");
    }
    return;
  }
  /* unload a room from memory or delete it totally - all rooms files */
  if (!strcmp(word[1], "-u") || !strcmp(word[1], "-d")) {
    if (word_count < 3) {
      write_user(user, "Usage: rmadmin -l / -m / -u <name> / -d <name>\n");
      return;
    }
    sprintf(rmname, "(%s)", word[2]);
    strtolower(rmname);
    /* first do checks on the room */
    rm = get_room_full(rmname);
    if (!rm) {
      write_user(user, "That user does not have a personal room built.\n");
      return;
    }
    if (room_visitor_count(rm)) {
      write_user(user, "You cannot remove a room if people are in it.\n");
      return;
    }
    /* okay to remove the room */
    /* remove invites */
    for (u = user_first; u; u = u->next) {
      if (u->invite_room == rm) {
        u->invite_room = NULL;
      }
    }
    /* destroy */
    destruct_room(rm);
    strtolower(word[2]);
    *word[2] = toupper(*word[2]);
    /* delete all files */
    if (!strcmp(word[1], "-d")) {
      sprintf(filename, "%s/%s/%s.R", USERFILES, USERROOMS, word[2]);
      remove(filename);
      sprintf(filename, "%s/%s/%s.B", USERFILES, USERROOMS, word[2]);
      remove(filename);
      write_syslog(SYSLOG, 1, "%s deleted the personal room of %s.\n",
                   user->name, word[2]);
      vwrite_user(user,
                  "You have now ~OL~FRdeleted~RS the room belonging to %s.\n",
                  word[2]);
      /* remove all the key flags */
      u = retrieve_user(user, word[2]);
      if (u) {
        all_unsetbit_flagged_user_entry(u, fufROOMKEY);
        write_user(user, "All keys to that room have now been destroyed.\n");
        done_retrieve(u);
      }
    } else {
      /* just unload from memory */
      write_syslog(SYSLOG, 1,
                   "%s unloaded the personal room of %s from memory.\n",
                   user->name, word[2]);
      vwrite_user(user,
                  "You have now ~OL~FGunloaded~RS the room belonging to %s from memory.\n",
                  word[2]);
    }
    return;
  }
  /* wrong input given */
  write_user(user, "Usage: rmadmin -l | -m | -u <name> | -d <name>\n");
}


/*
 * this function allows users to give access to others even if their personal room
 * has been locked
 */
void
personal_room_key(UR_OBJECT user)
{
  RM_OBJECT rm;
  FU_OBJECT fu;

  if (!amsys->personal_rooms) {
    write_user(user, "Personal room functions are currently disabled.\n");
    return;
  }

  /* if no name was given then display keys given */
  if (word_count < 2) {
    char text2[ARR_SIZE];
    int found = 0, cnt = 0;

    *text2 = '\0';
    for (fu = user->fu_first; fu; fu = fu->next) {
      if (fu->flags & fufROOMKEY) {
        if (!found++) {
          write_user(user,
                     "+----------------------------------------------------------------------------+\n");
          write_user(user,
                     "| ~OL~FCYou have given the following people a key to your room~RS                     |\n");
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
      write_user(user,
                 "You have not given anyone a personal room key yet.\n");
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
    return;
  }

  {
    char rmname[ROOM_NAME_LEN + 1];

    /* see if user has a room created */
    sprintf(rmname, "(%s)", user->name);
    strtolower(rmname);
    rm = get_room_full(rmname);
    if (!rm) {
      write_user(user,
                 "Sorry, but you have not created a personal room yet.\n");
      return;
    }
  }

  /* actually add/remove a user */
  strtolower(word[1]);
  *word[1] = toupper(*word[1]);
  if (!strcmp(user->name, word[1])) {
    write_user(user, "You already have access to your own room!\n");
    return;
  }
  /*
   * check to see if the user is already listed before the adding part.
   * This is to ensure you can remove a user even if they have, for
   * instance, suicided.
   */
  if (has_room_key(word[1], rm)) {
    if (!personal_key_remove(user, word[1])) {
      write_user(user,
                 "There was an error taking the key away from that user.\n");
      return;
    }
    vwrite_user(user,
                "You take your personal room key away from ~FC~OL%s~RS.\n",
                word[1]);
    vwrite_user(get_user(word[1]), "%s takes back their personal room key.\n",
                user->name);
  } else {
    /* see if there is such a user */
    if (!find_user_listed(word[1])) {
      write_user(user, nosuchuser);
      return;
    }
    /* give them a key */
    if (!personal_key_add(user, word[1])) {
      write_user(user, "There was an error giving the key to that user.\n");
      return;
    }
    vwrite_user(user, "You give ~FC~OL%s~RS a key to your personal room.\n",
                word[1]);
    vwrite_user(get_user(word[1]), "%s gives you a key to their room.\n",
                user->name);
  }
}


/*
 * adds a name to the user's personal room key list
 */
int
personal_key_add(UR_OBJECT user, char *name)
{
  return setbit_flagged_user_entry(user, name, fufROOMKEY);
}


/*
 * remove a name from the user's personal room key list
 */
int
personal_key_remove(UR_OBJECT user, char *name)
{
  return unsetbit_flagged_user_entry(user, name, fufROOMKEY);
}


/*
 * allows a user to rename their room
 */
void
personal_room_rename(UR_OBJECT user, char *inpstr)
{
  if (!amsys->personal_rooms) {
    write_user(user, "Personal room functions are currently disabled.\n");
    return;
  }
  if (word_count < 2) {
    write_user(user, "Usage: myname <name you want room to have>\n");
    return;
  }
  if (strcmp(user->room->owner, user->name)) {
    write_user(user, "You have to be in your personal room to rename it.\n");
    return;
  }
  if (strlen(inpstr) > PERSONAL_ROOMNAME_LEN) {
    write_user(user, "You cannot have a room name that long.\n");
    return;
  }
  if (strlen(inpstr) - teslen(inpstr, 0) < 1) {
    write_user(user, "You must enter a room name.\n");
    return;
  }
  strcpy(user->room->show_name, inpstr);
  vwrite_user(user, "You have now renamed your room to: %s\n",
              user->room->show_name);
  if (!personal_room_store(user->name, 1, user->room)) {
    write_syslog(SYSLOG | ERRLOG, 1,
                 "ERROR: Unable to save personal room status in personal_room_rename()\n");
  }
}



/*
 * Called by go() and move()
 */
void
move_user(UR_OBJECT user, RM_OBJECT rm, int teleport)
{
  RM_OBJECT old_room;

  if (teleport != 2 && !has_room_access(user, rm)) {
    write_user(user, "That room is currently private, you cannot enter.\n");
    return;
  }
  /* Reset invite room if in it */
  if (user->invite_room == rm) {
    user->invite_room = NULL;
    *user->invite_by = '\0';
  }
  if (user->vis) {
    switch (teleport) {
    case 0:
      vwrite_room(rm, "%s~RS %s.\n", user->recap, user->in_phrase);
      vwrite_room_except(user->room, user, "%s~RS %s to the %s.\n",
                         user->recap, user->out_phrase, rm->name);
      break;
    case 1:
      vwrite_room(rm, "%s~RS ~FC~OLappears in an explosion of blue magic!\n",
                  user->recap);
      vwrite_room_except(user->room, user,
                         "%s~RS ~FC~OLchants a spell and vanishes into a magical blue vortex!\n",
                         user->recap);
      break;
    case 2:
      write_user(user,
                 "\n~FC~OLA giant hand grabs you and pulls you into a magical blue vortex!\n");
      vwrite_room(rm, "%s~RS ~FC~OLfalls out of a magical blue vortex!\n",
                  user->recap);
#ifdef NETLINKS
      if (!release_nl(user))
#endif
      {
        vwrite_room_except(user->room, user,
                           "~FC~OLA giant hand grabs~RS %s~RS ~FC~OLwho is pulled into a magical blue vortex!\n",
                           user->recap);
      }
      break;
    }

  } else if (user->level < GOD) {
    write_room(rm, invisenter);
    write_room_except(user->room, invisleave, user);
  }
  old_room = user->room;
  user->room = rm;
  reset_access(old_room);
  look(user);
}

/*
 * Reloads the description for one or all rooms--incase you have edited the
 * file and do not want to reboot the talker to to make the changes displayed
 */
void
reload_room_description(UR_OBJECT user)
{
  int c, i, error;
  RM_OBJECT rm;
  char filename[80];
  FILE *fp;

  if (word_count < 2) {
    write_user(user, "Usage: rloadrm -a/<room name>\n");
    return;
  }
  /* if reload all of the rooms */
  if (!strcmp(word[1], "-a")) {
    error = 0;
    for (rm = room_first; rm; rm = rm->next) {
      if (is_personal_room(rm)) {
        continue;
      }
      sprintf(filename, "%s/%s.R", DATAFILES, rm->name);
      fp = fopen(filename, "r");
      if (!fp) {
        vwrite_user(user,
                    "Sorry, cannot reload the description file for the room \"%s\".\n",
                    rm->name);
        write_syslog(SYSLOG | ERRLOG, 0,
                     "ERROR: Cannot reload the description file for room %s.\n",
                     rm->name);
        ++error;
        continue;
      }
      i = 0;
      for (c = getc(fp); c != EOF; c = getc(fp)) {
        if (i == ROOM_DESC_LEN) {
          break;
        }
        rm->desc[i++] = c;
      }
      if (c != EOF) {
	vwrite_user(user,
		    "The description is too long for the room \"%s\".\n",
		    rm->name);
	write_syslog(SYSLOG | ERRLOG, 0,
		     "ERROR: Description too long when reloading for room %s.\n",
		     rm->name);
      }
      rm->desc[i] = '\0';
      fclose(fp);
    }
    if (!error) {
      write_user(user, "You have now reloaded all room descriptions.\n");
    } else {
      write_user(user,
                 "You have now reloaded all room descriptions that you can.\n");
    }
    write_syslog(SYSLOG, 1, "%s reloaded all of the room descriptions.\n",
                 user->name);
    return;
  }
  /* if it is just one room to reload */
  rm = get_room(word[1]);
  if (!rm) {
    write_user(user, nosuchroom);
    return;
  }
  /* check first for personal room, and do not reload */
  if (is_personal_room(rm)) {
    write_user(user,
               "Sorry, but you cannot reload personal room descriptions.\n");
    return;
  }
  sprintf(filename, "%s/%s.R", DATAFILES, rm->name);
  fp = fopen(filename, "r");
  if (!fp) {
    vwrite_user(user,
                "Sorry, cannot reload the description file for the room \"%s\".\n",
                rm->name);
    write_syslog(SYSLOG | ERRLOG, 0,
                 "ERROR: Cannot reload the description file for room %s.\n",
                 rm->name);
    return;
  }
  i = 0;
  for (c = getc(fp); c != EOF; c = getc(fp)) {
    if (i == ROOM_DESC_LEN) {
      break;
    }
    rm->desc[i++] = c;
  }
  if (c != EOF) {
    vwrite_user(user, "The description is too long for the room \"%s\".\n",
		rm->name);
    write_syslog(SYSLOG | ERRLOG, 0,
		 "ERROR: Description too long when reloading for room %s.\n",
		 rm->name);
  }
  rm->desc[i] = '\0';
  fclose(fp);
  vwrite_user(user,
              "You have now reloaded the description for the room \"%s\".\n",
              rm->name);
  write_syslog(SYSLOG, 1, "%s reloaded the description for the room %s\n",
               user->name, rm->name);
}


/*
 * Set room access back to public if not enough users in room
 */
void
reset_access(RM_OBJECT rm)
{
  UR_OBJECT u;

  if (!rm || is_personal_room(rm) || is_fixed_room(rm)
      || !is_private_room(rm)) {
    return;
  }
  if (room_visitor_count(rm) < amsys->min_private_users) {
    /* Reset any invites into the room & clear review buffer */
    for (u = user_first; u; u = u->next) {
      if (u->invite_room == rm) {
        u->invite_room = NULL;
      }
    }
    clear_revbuff(rm);
    rm->access &= ~PRIVATE;
    write_room(rm, "Room access returned to ~FGPUBLIC.\n");
  }
}


/*
 * See if a user has access to a room. If room is fixed to private then
 * it is considered a wizroom so grant permission to any user of WIZ and
 * above for those.
 */
int
has_room_access(UR_OBJECT user, RM_OBJECT rm)
{
  size_t i;

  /* level room checks */
  for (i = 0; priv_room[i].name; ++i) {
    if (!strcmp(rm->name, priv_room[i].name)) {
      break;
    }
  }
  if (user->invite_room == rm) {
    return 1;
  }
  if (priv_room[i].name && user->level < priv_room[i].level) {
    return 0;
  }
  if (!is_private_room(rm)) {
    return 1;
  }
  if (is_personal_room(rm)) {
    return is_my_room(user, rm) || has_room_key(user->name, rm)
      || user->level == GOD;
  }
  if (is_fixed_room(rm)) {
    return user->level >= WIZ;
  }
  return user->level >= amsys->gatecrash_level;
}


/*
 * Check the room you are logging into is not private
 */
int
check_start_room(UR_OBJECT user)
{
  int was_private;
  RM_OBJECT rm;

  rm = user->level == JAILED ? get_room_full(amsys->default_jail)
    : !user->lroom ? room_first : get_room_full(user->logout_room);
  was_private = rm && rm != room_first
    && user->level != JAILED
    && user->lroom != 2 && !has_room_access(user, rm);
  user->room = !rm || was_private ? room_first : rm;
  return was_private;
}
