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
#include "spodlist.h"
#include "prototypes.h"

/***************************************************************************

 This spod code is based on that from Richard Lawrence's PG+ Spodlist code,
 as found in Playground+ (PG+). Credits are as follows:

 Playground+--spodlist.c v1.0
 Spodlist code copyright (c) Richard Lawrence (Silver) 1998

 Permission granted for extraction and usage in a running talker if
 you place credit for it either in help credits or your talkers
 "version" command and mail me the talker addy so I can see :o)

 <silver@ewtoo.org>

 ***************************************************************************/


/*
 * Find a person in the spodlist and return their position in the spodlist
 */
int
find_spodlist_position(char *name)
{
  SP_OBJECT sp;
  int position = 0;

  for (sp = first_spod; sp; sp = sp->next) {
    ++position;
    if (!strcasecmp(name, sp->name)) {
      break;
    }
  }
  return !sp ? 0 : position;
}


/*
 * Return the number of people in the spodlist
 */
int
people_in_spodlist(void)
{
  SP_OBJECT sp;
  int people = 0;

  for (sp = first_spod; sp; sp = sp->next) {
    ++people;
  }
  return people;
}


/*
 * Add a name to the spodlist
 */
void
add_name_to_spodlist(char *name, int logintime)
{
  SP_OBJECT cur, prev, new_node;

  new_node = (SP_OBJECT) malloc(sizeof *new_node);
  if (!new_node) {              /* oops! no memory */
    write_syslog(SYSLOG, 1, "ERROR: Out of memory to malloc in spodlist.c");
    return;
  }
  memset(new_node, 0, (sizeof *new_node));
  new_node->login = logintime;
  strcpy(new_node->name, name);
  prev = NULL;
  for (cur = first_spod; cur; cur = cur->next) {
    if (new_node->login >= cur->login) {
      break;
    }
    prev = cur;
  }
  new_node->next = cur;
  if (!prev) {
    first_spod = new_node;
  } else {
    prev->next = new_node;
  }
}


/*
 * Delete whole list.
 * (I could update the spodlist database each time it is run but the time
 * taken to find an element and then update it is longer than just deleting
 * the whole lot and re-doing the whole database - trust me, I tested it)
 */
void
delete_spodlist(void)
{
  SP_OBJECT curr, next;

  for (curr = first_spod; curr; curr = next) {
    next = curr->next;
    memset(curr, 0, (sizeof *curr));
    free(curr);
  }
  first_spod = NULL;
}


/*
  Calculate the spodlist
*/
void
calc_spodlist(void)
{
  UR_OBJECT u;
  UD_OBJECT entry;

  /* Delete the whole list */
  delete_spodlist();

  /* Save all players so times are accurate */
  for (u = user_first; u; u = u->next) {
#ifdef NETLINKS
    if (u->type == REMOTE_TYPE) {
      continue;
    }
#endif
    if (u->type == CLONE_TYPE || u->login) {
      continue;
    }
    save_user_details(u, 1);
  }

  /* Create spod list */
  for (entry = first_user_entry; entry; entry = entry->next) {
    u = retrieve_user(NULL, entry->name);
    if (!u) {
      continue;
    }
    add_name_to_spodlist(u->name, u->total_login);
    done_retrieve(u);
  }
}


void
show_spodlist(UR_OBJECT user)
{
  int start_pos = 1, end_pos, listed, pos = 0, hilight = 0;
  SP_OBJECT sp;

  calc_spodlist();
  sp = first_spod;

  listed = people_in_spodlist();

  if (*word[1]) {
    /* We will assume it is a position they want. If it is a name then we
       can easily overwrite it */
    hilight = atoi(word[1]);

    if (!hilight) {
      if (!find_user_listed(word[1])) {
        write_user(user, nosuchuser);
        return;
      } else {
        hilight = find_spodlist_position(word[1]);
      }
    }

    if (hilight > listed) {
      start_pos = listed - 16;
      hilight = 0;
    } else {
      start_pos = hilight - 8;
      if (start_pos < 1) {
        start_pos = 1;
      }
    }
  } else {
    /* I personally like it defaulting to a user name */
    hilight = find_spodlist_position(user->name);
    start_pos = hilight - 8;
    if (start_pos < 1) {
      start_pos = 1;
    }
  }

  end_pos = start_pos + 16;
  if (end_pos > listed) {
    end_pos = listed;
    start_pos = end_pos - 16;
  }


  /* Create the page */
  write_user(user,
             "+----------------------------------------------------------------------------+\n");
  write_user(user,
             "| ~FC~OLSpod List~RS                                                                  |\n");
  write_user(user,
             "+----------------------------------------------------------------------------+\n");


  for (pos = 1; pos <= end_pos && sp; ++pos, sp = sp->next) {
    if (pos >= start_pos) {
      vwrite_user(user, "| %s%4d. %-20s   %45s~RS |\n",
                  (pos == hilight ? "~OL~FG" : ""), pos, sp->name,
                  word_time(sp->login));
    }
  }

  if (start_pos < 1) {
    start_pos = 1;
  }
  write_user(user,
             "+----------------------------------------------------------------------------+\n");
  write_user(user,
             align_string(0, 78, 1, "|",
                          "  Positions %d to %d (out of %d users) ",
                          start_pos, end_pos, listed));
  write_user(user,
             "+----------------------------------------------------------------------------+\n");
}
