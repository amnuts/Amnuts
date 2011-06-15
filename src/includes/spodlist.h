/****************************************************************************
         Amnuts version 2.3.0 - Copyright (C) Andrew Collington, 2003
                      Last update: 2003-08-04

                              amnuts@talker.com
                          http://amnuts.talker.com/

                                   based on

   NUTS version 3.3.3 (Triple Three :) - Copyright (C) Neil Robertson 1996

 ***************************************************************************

 This spod code is based on that from Richard Lawrence's PG+ Spodlist code,
 as found in Playground+ (PG+).  Credits are as follows:

 Playground+ - spodlist.c v1.0
 Spodlist code copyright (c) Richard Lawrence (Silver) 1998

 Permission granted for extraction and usage in a running talker if
 you place credit for it either in help credits or your talkers
 "version" command and mail me the talker addy so I can see :o)

 <silver@ewtoo.org>

 ***************************************************************************/

#ifndef AMNUTS_SPODLIST_H
#define AMNUTS_SPODLIST_H

typedef struct spodlist *SP_OBJECT;
/*
 * spod list structure
 */
struct spodlist
{
  char name[USER_NAME_LEN];
  int login;
  SP_OBJECT next;
};

SP_OBJECT first_spod = NULL;

#endif
