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

     Universal Pager concept and code by Ardant (ardant@ardant.net)

****************************************************************************/


void
start_pager(UR_OBJECT user)
{
  while (user->pm_first) {
    destruct_pag_mesg(user, user->pm_first);
  }
  user->pm_first = user->pm_last = user->pm_current = NULL;
  user->pm_count = 0;
  user->pm_currcount = 0;
  user->universal_pager = 1;
}


void
stop_pager(UR_OBJECT user)
{
  user->universal_pager = 0;
  if (user->misc_op == 25) {
    write_apager(user);
  } else {
    end_pager(user);
  }
}


void
end_pager(UR_OBJECT user)
{
  while (user->pm_first) {
    destruct_pag_mesg(user, user->pm_first);
  }
  user->pm_first = user->pm_last = user->pm_current = NULL;
  user->pm_count = 0;
  user->pm_currcount = 0;
  user->universal_pager = 0;
  user->misc_op = 0;
}


PM_OBJECT
create_buf_mesg(const char *str)
{
  PM_OBJECT bm;

  bm = (PM_OBJECT) malloc(sizeof *bm);
  if (!bm) {
    return NULL;
  }
  bm->data = (char *) malloc(1 + strlen(str));
  if (!bm->data) {
    free(bm);
    return NULL;
  }
  strcpy(bm->data, str);
  return bm;
}


int
destruct_pag_mesg(UR_OBJECT user, PM_OBJECT bm)
{
  if (!bm) {
    return 0;
  }
  if (bm->data) {
    memset(bm->data, 0, 1 + strlen(bm->data));
    free(bm->data);
  }
  if (bm == user->pm_first) {
    user->pm_first = bm->next;
    if (bm == user->pm_last || !user->pm_first) {
      user->pm_last = NULL;
    } else {
      user->pm_first->prev = NULL;
    }
  } else {
    bm->prev->next = bm->next;
    if (bm == user->pm_last) {
      user->pm_last = bm->prev;
      user->pm_last->next = NULL;
    } else {
      bm->next->prev = bm->prev;
    }
  }
  memset(bm, 0, (sizeof *bm));
  free(bm);
  return 1;
}


void
add_pag_mesg_user(UR_OBJECT user, PM_OBJECT bm)
{
  if (!user->pm_first) {
    user->pm_first = bm;
    bm->prev = NULL;
  } else {
    user->pm_last->next = bm;
    bm->prev = user->pm_last;
  }
  bm->next = NULL;
  user->pm_last = bm;
  ++user->pm_count;
}


int
add_pm(UR_OBJECT user, const char *str)
{
  PM_OBJECT bm;

  bm = create_buf_mesg(str);
  if (!bm) {
    return -1;
  }
  add_pag_mesg_user(user, bm);
  return 1;
}


int
display_pm(UR_OBJECT user)
{
  PM_OBJECT t;
  int pager;
  int i;

  if (!user->pm_first) {
    return -1;
  }
  pager = user->pager < MAX_LINES || user->pager > 999 ? 23 : user->pager;
  i = 0;
  for (t = !user->pm_current ? user->pm_first : user->pm_current; t;
       t = t->next) {
    amsys->is_pager = 1;
    write_user(user, t->data);
    amsys->is_pager = 0;
    if (++i >= pager && t->next) {
      break;
    }
  }
  if (!t) {
    return -1;
  }
  user->pm_current = t->next;
  write_apager(user);
  return 0;
}

/*
   backup=0, redisplay current page
   backup=1, go back one page
   backup=2, go to the first page
*/
int
rewind_pager(UR_OBJECT user, int backup)
{
  PM_OBJECT t;
  int pager;

  if (!user->pm_first) {
    return -1;
  }
  pager = user->pager < MAX_LINES || user->pager > 999 ? 23 : user->pager;
  if (backup == 2) {
    user->pm_current = NULL;
    user->pm_currcount = 0;
  } else if (backup == 1) {
    pager += pager;
    user->pm_currcount -= 2;
  } else {
    user->pm_currcount -= 1;
  }
  if (!user->pm_current || user->pm_currcount < 0) {
    user->pm_currcount = 0;
  }
  for (t = !user->pm_current ? user->pm_first : user->pm_current;
       t->prev; t = t->prev) {
    if (!pager--) {
      break;
    }
  }
  user->pm_current = t;
  return 0;
}


void
write_apager(UR_OBJECT user)
{
  amsys->is_pager = 1;
  vwrite_user(user,
              "~OL~FG-=[%d/%d] (~RS~OLE~FG)xit, (~RS~OLR~FG)edisplay, (~RS~OLB~FG)ack, (~RS~OLT~FG)op, <return> to continue ]~RS ",
              ++user->pm_currcount, (int) (user->pm_count / user->pager) + 1);
  amsys->is_pager = 0;
}
