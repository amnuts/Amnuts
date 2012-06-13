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

/***************************************************************************/

/*
 * Say user speech.
 */
void
say(UR_OBJECT user, char *inpstr)
{
  static const char usage[] = "Usage: say <text>\n";
  const char *type;
  const char *name;

  /* FIXME: Use sentinel other JAILED */
  if (user->muzzled != JAILED) {
    write_user(user, "You are muzzled, you cannot speak.\n");
    return;
  }
  if (!strlen(inpstr)) {
    write_user(user, usage);
    return;
  }
  switch (amsys->ban_swearing) {
  case SBMAX:
    if (contains_swearing(inpstr)) {
      write_user(user, noswearing);
      return;
    }
    break;
  case SBMIN:
    if (!is_private_room(user->room)) {
      inpstr = censor_swear_words(inpstr);
    }
    break;
  case SBOFF:
  default:
    /* do nothing as ban_swearing is off */
    break;
  }
  type = smiley_type(inpstr);
  if (!type) {
    type = "say";
  }
  if (user->type == CLONE_TYPE) {
    sprintf(text, "Clone of %s~RS ~FG%ss~RS: %s\n", user->recap, type,
            inpstr);
    record(user->room, text);
    write_room(user->room, text);
    return;
  }
  if (!user->vis) {
    write_monitor(user, user->room, 0);
  }
  name = user->vis ? user->recap : invisname;
  sprintf(text, "%s~RS ~FG%ss~RS: %s\n", name, type, inpstr);
  record(user->room, text);
  write_room_except(user->room, text, user);
  vwrite_user(user, "You ~FG%s~RS: %s\n", type, inpstr);
}


/*
 * Direct a say to someone, even though the whole room can hear it
 */
void
say_to(UR_OBJECT user, char *inpstr)
{
  static const char usage[] = "Usage: sayto <user> <text>\n";
  const char *type;
  const char *name, *n;
  UR_OBJECT u;

  /* FIXME: Use sentinel other JAILED */
  if (user->muzzled != JAILED) {
    write_user(user, "You are muzzled, you cannot speak.\n");
    return;
  }
  if (word_count < 3 && *inpstr != '-') {
    write_user(user, usage);
    return;
  }
  if (word_count < 2 && *inpstr == '-') {
    write_user(user, usage);
    return;
  }
  u = get_user_name(user, word[1]);
  if (!u) {
    write_user(user, notloggedon);
    return;
  }
  if (u == user) {
    write_user(user, "Talking to yourself is the first sign of madness!\n");
    return;
  }
  if (user->room != u->room || (!u->vis && user->level < u->level)) {
    vwrite_user(user,
                "You cannot see %s~RS, so you cannot say anything to them!\n",
                u->recap);
    return;
  }
  inpstr = remove_first(inpstr);
  switch (amsys->ban_swearing) {
  case SBMAX:
    if (contains_swearing(inpstr)) {
      write_user(user, noswearing);
      return;
    }
    break;
  case SBMIN:
    if (!is_private_room(user->room)) {
      inpstr = censor_swear_words(inpstr);
    }
    break;
  case SBOFF:
  default:
    /* do nothing as ban_swearing is off */
    break;
  }
  type = smiley_type(inpstr);
  if (!type) {
    type = "say";
  }
  if (!user->vis) {
    write_monitor(user, user->room, 0);
  }
  name = user->vis ? user->recap : invisname;
  n = u->vis ? u->recap : invisname;
  sprintf(text, "(%s~RS) %s~RS ~FC%ss~RS: %s\n", n, name, type, inpstr);
  record(user->room, text);
  write_room_except(user->room, text, user);
  vwrite_user(user, "(%s~RS) You ~FC%s~RS: %s\n", u->recap, type, inpstr);
}


/*
 * Shout something
 */
void
shout(UR_OBJECT user, char *inpstr)
{
  static const char usage[] = "Usage: shout <text>\n";
  const char *type;
  const char *name;

  /* FIXME: Use sentinel other JAILED */
  if (user->muzzled != JAILED) {
    write_user(user, "You are muzzled, you cannot speak.\n");
    return;
  }
  if (word_count < 2) {
    write_user(user, usage);
    return;
  }
  switch (amsys->ban_swearing) {
  case SBMAX:
    if (contains_swearing(inpstr)) {
      write_user(user, noswearing);
      return;
    }
    break;
  case SBMIN:
    inpstr = censor_swear_words(inpstr);
    break;
  case SBOFF:
  default:
    /* do nothing as ban_swearing is off */
    break;
  }
  type = smiley_type(inpstr);
  if (!type) {
    type = "shout";
  }
  if (!user->vis) {
    write_monitor(user, NULL, 0);
  }
  name = user->vis ? user->recap : invisname;
  sprintf(text, "~OL!~RS %s~RS ~OL%ss~RS: %s\n", name, type, inpstr);
  record_shout(text);
  write_room_except(NULL, text, user);
  vwrite_user(user, "~OL!~RS You ~OL%s~RS: %s\n", type, inpstr);
}

/*
 * Shout something to someone
 */
void
sto(UR_OBJECT user, char *inpstr)
{
  static const char usage[] = "Usage: sto <user> <text>\n";
  const char *type;
  const char *name, *n;
  UR_OBJECT u;
  
  /* FIXME: Use sentinel other JAILED */
  if (user->muzzled != JAILED) {
    write_user(user, "You are muzzled, you cannot speak.\n");
    return;
  }
  if (word_count < 3) {
    write_user(user, usage);
    return;
  }
  u = get_user_name(user, word[1]);
  if (!u) {
    write_user(user, notloggedon);
    return;
  }
  if (u == user) {
    write_user(user, "Talking to yourself is the first sign of madness.\n");
    return;
  }
  if (check_igusers(u, user) && user->level < GOD) {
    vwrite_user(user, "%s~RS is ignoring you.\n",
                u->recap);
    return;
  }
  if (u->igntells && (user->level < WIZ || u->level > user->level)) {
    vwrite_user(user, "%s~RS is ignoring stuff at the moment.\n",
                u->recap);
    return;
  }
  inpstr = remove_first(inpstr);
  switch (amsys->ban_swearing) {
  case SBMAX:
    if (contains_swearing(inpstr)) {
      write_user(user, noswearing);
      return;
    }
    break;
  case SBMIN:
    inpstr = censor_swear_words(inpstr);
    break;
  case SBOFF:
  default:
    /* do nothing as ban_swearing is off */
    break;
  }
  type = smiley_type(inpstr);
  if (!type) {
    type = "shout";
  }
  if (!user->vis) {
    write_monitor(user, NULL, 0);
  }
  name = user->vis ? user->recap : invisname;
  n = u->vis ? u->recap : invisname;
  sprintf(text, "~OL!~RS %s~RS ~OL%ss~RS to ~OL%s~RS: %s~RS\n", name, type, n, inpstr);
  record_shout(text);
  write_room_except(NULL, text, user);
  vwrite_user(user, "~OL!~RS You ~OL%s~RS to ~OL%s~RS: %s~RS\n", type, n, inpstr);
}

/*
 * Tell another user something
 */
void
tell_user(UR_OBJECT user, char *inpstr)
{
  static const char usage[] = "Usage: tell <user> <text>\n";
  static const char qcusage[] = "Usage: ,<text>\n";
  const char *type;
  const char *name;
  UR_OBJECT u;

  /* FIXME: Use sentinel other JAILED */
  if (user->muzzled != JAILED) {
    write_user(user, "You are muzzled, you cannot speak.\n");
    return;
  }
  /* determine whether this is a quick call */
  if (*inpstr == ',') {
    if (!*user->call) {
      write_user(user, "Quick call not set.\n");
      return;
    }
    u = get_user_name(user, user->call);
    /* if quick call with no message */
    if (word_count < 2) {
      write_user(user, qcusage);
      return;
    }
    inpstr = remove_first(inpstr);
  } else {
    /* if tell by itself, review tells */
    if (word_count < 2) {
      revtell(user);
      return;
    }
    u = get_user_name(user, word[1]);
    /* if tell <user> with no message */
    if (word_count < 3) {
      write_user(user, usage);
      return;
    }
    inpstr = remove_first(inpstr);
  }
  if (!u) {
    write_user(user, notloggedon);
    return;
  }
  if (u == user) {
    write_user(user, "Talking to yourself is the first sign of madness.\n");
    return;
  }
  if (check_igusers(u, user) && user->level < GOD) {
    vwrite_user(user, "%s~RS is ignoring tells from you.\n", u->recap);
    return;
  }
  if (u->igntells && (user->level < WIZ || u->level > user->level)) {
    vwrite_user(user, "%s~RS is ignoring tells at the moment.\n", u->recap);
    return;
  }
  switch (amsys->ban_swearing) {
  case SBMAX:
    if (contains_swearing(inpstr)) {
      write_user(user, noswearing);
      return;
    }
    break;
  case SBMIN:
    inpstr = censor_swear_words(inpstr);
    break;
  case SBOFF:
  default:
    /* do nothing as ban_swearing is off */
    break;
  }
  type = smiley_type(inpstr);
  if (!type) {
    type = "say";
  }
  name = user->vis || u->level >= user->level ? user->recap : invisname;
  sprintf(text, "~OL~FG>~RS %s~RS ~FC%ss~RS: %s\n", name, type, inpstr);
  if (u->afk) {
    record_afk(user, u, text);
    if (*u->afk_mesg) {
      vwrite_user(user, "%s~RS is ~FRAFK~RS, message is: %s\n", u->recap,
                  u->afk_mesg);
    } else {
      vwrite_user(user, "%s~RS is ~FRAFK~RS at the moment.\n", u->recap);
    }
    write_user(user, "Sending message to their afk review buffer.\n");
    return;
  }
  if (u->malloc_start) {
    record_edit(user, u, text);
    vwrite_user(user,
                "%s~RS is in ~FCEDIT~RS mode at the moment (using the line editor).\n",
                u->recap);
    write_user(user, "Sending message to their edit review buffer.\n");
    return;
  }
  if (u->ignall && (user->level < WIZ || u->level > user->level)) {
    vwrite_user(user, "%s~RS is ignoring everyone at the moment.\n",
                u->recap);
    return;
  }
#ifdef NETLINKS
  if (!u->room) {
    vwrite_user(user,
                "%s~RS is offsite and would not be able to reply to you.\n",
                u->recap);
    return;
  }
#endif
  record_tell(user, u, text);
  write_user(u, text);
  sprintf(text, "~OL~FG>~RS (%s~RS) You ~FC%s~RS: %s\n", u->recap, type,
          inpstr);
  record_tell(user, user, text);
  write_user(user, text);
}


/*
 * Emote something
 */
void
emote(UR_OBJECT user, char *inpstr)
{
  static const char usage[] = "Usage: emote <text>\n";
  const char *name;

  /* FIXME: Use sentinel other JAILED */
  if (user->muzzled != JAILED) {
    write_user(user, "You are muzzled, you cannot emote.\n");
    return;
  }
  if (word_count < 2) {
    write_user(user, usage);
    return;
  }
  switch (amsys->ban_swearing) {
  case SBMAX:
    if (contains_swearing(inpstr)) {
      write_user(user, noswearing);
      return;
    }
    break;
  case SBMIN:
    if (!is_private_room(user->room)) {
      inpstr = censor_swear_words(inpstr);
    }
    break;
  case SBOFF:
  default:
    /* do nothing as ban_swearing is off */
    break;
  }
  if (!user->vis) {
    write_monitor(user, user->room, 0);
  }
  name = user->vis ? user->recap : invisname;
  if (user->type == CLONE_TYPE) {
    sprintf(text, "Clone of %s~RS%s%s\n", name, *inpstr != '\'' ? " " : "",
            inpstr);
    record(user->room, text);
    write_room(user->room, text);
    return;
  }
  sprintf(text, "%s~RS%s%s\n", name, *inpstr != '\'' ? " " : "", inpstr);
  record(user->room, text);
  write_room_ignore(user->room, user, text);
}


/*
 * Do a shout emote
 */
void
semote(UR_OBJECT user, char *inpstr)
{
  static const char usage[] = "Usage: semote <text>\n";
  const char *name;

  /* FIXME: Use sentinel other JAILED */
  if (user->muzzled != JAILED) {
    write_user(user, "You are muzzled, you cannot emote.\n");
    return;
  }
  if (word_count < 2) {
    write_user(user, usage);
    return;
  }
  switch (amsys->ban_swearing) {
  case SBMAX:
    if (contains_swearing(inpstr)) {
      write_user(user, noswearing);
      return;
    }
    break;
  case SBMIN:
    inpstr = censor_swear_words(inpstr);
    break;
  case SBOFF:
  default:
    /* do nothing as ban_swearing is off */
    break;
  }
  if (!user->vis) {
    write_monitor(user, NULL, 0);
  }
  name = user->vis ? user->recap : invisname;
  sprintf(text, "~OL!~RS %s~RS%s%s\n", name, *inpstr != '\'' ? " " : "",
          inpstr);
  record_shout(text);
  write_room_ignore(NULL, user, text);
}


/*
 * Do a private emote
 */
void
pemote(UR_OBJECT user, char *inpstr)
{
  static const char usage[] = "Usage: pemote <user> <text>\n";
  const char *name;
  UR_OBJECT u;

  /* FIXME: Use sentinel other JAILED */
  if (user->muzzled != JAILED) {
    write_user(user, "You are muzzled, you cannot emote.\n");
    return;
  }
  if (word_count < 3 && !strchr("</", *inpstr)) {
    write_user(user, usage);
    return;
  }
  if (word_count < 2 && strchr("</", *inpstr)) {
    write_user(user, usage);
    return;
  }
  u = get_user_name(user, word[1]);
  if (!u) {
    write_user(user, notloggedon);
    return;
  }
  if (u == user) {
    write_user(user, "Talking to yourself is the first sign of madness.\n");
    return;
  }
  if (check_igusers(u, user) && user->level < GOD) {
    vwrite_user(user, "%s~RS is ignoring private emotes from you.\n",
                u->recap);
    return;
  }
  if (u->igntells && (user->level < WIZ || u->level > user->level)) {
    vwrite_user(user, "%s~RS is ignoring private emotes at the moment.\n",
                u->recap);
    return;
  }
  inpstr = remove_first(inpstr);
  switch (amsys->ban_swearing) {
  case SBMAX:
    if (contains_swearing(inpstr)) {
      write_user(user, noswearing);
      return;
    }
    break;
  case SBMIN:
    inpstr = censor_swear_words(inpstr);
    break;
  case SBOFF:
  default:
    /* do nothing as ban_swearing is off */
    break;
  }
  name = user->vis || u->level >= user->level ? user->recap : invisname;
  sprintf(text, "~OL~FG>~RS %s~RS%s%s\n", name, *inpstr != '\'' ? " " : "",
          inpstr);
  if (u->afk) {
    record_afk(user, u, text);
    if (*u->afk_mesg) {
      vwrite_user(user, "%s~RS is ~FRAFK~RS, message is: %s\n", u->recap,
                  u->afk_mesg);
    } else {
      vwrite_user(user, "%s~RS is ~FRAFK~RS at the moment.\n", u->recap);
    }
    write_user(user, "Sending message to their afk review buffer.\n");
    return;
  }
  if (u->malloc_start) {
    record_edit(user, u, text);
    vwrite_user(user,
                "%s~RS is in ~FCEDIT~RS mode at the moment (using the line editor).\n",
                u->recap);
    write_user(user, "Sending message to their edit review buffer.\n");
    return;
  }
  if (u->ignall && (user->level < WIZ || u->level > user->level)) {
    vwrite_user(user, "%s~RS is ignoring everyone at the moment.\n",
                u->recap);
    return;
  }
#ifdef NETLINKS
  if (!u->room) {
    vwrite_user(user,
                "%s~RS is offsite and would not be able to reply to you.\n",
                u->recap);
    return;
  }
#endif
  record_tell(user, u, text);
  write_user(u, text);
  sprintf(text, "~FG~OL>~RS (%s~RS) %s~RS%s%s\n", u->recap, name,
          *inpstr != '\'' ? " " : "", inpstr);
  record_tell(user, user, text);
  write_user(user, text);
}


/*
 * Echo something to screen
 */
void
echo(UR_OBJECT user, char *inpstr)
{
  static const char usage[] = "Usage: echo <text>\n";

  /* FIXME: Use sentinel other JAILED */
  if (user->muzzled != JAILED) {
    write_user(user, "You are muzzled, you cannot echo.\n");
    return;
  }
  if (word_count < 2) {
    write_user(user, usage);
    return;
  }
  switch (amsys->ban_swearing) {
  case SBMAX:
    if (contains_swearing(inpstr)) {
      write_user(user, noswearing);
      return;
    }
    break;
  case SBMIN:
    if (!is_private_room(user->room)) {
      inpstr = censor_swear_words(inpstr);
    }
    break;
  case SBOFF:
  default:
    /* do nothing as ban_swearing is off */
    break;
  }
  write_monitor(user, user->room, 0);
  sprintf(text, "+ %s\n", inpstr);
  record(user->room, text);
  write_room(user->room, text);
}


/*
 * Tell something to everyone but one person
 */
void
mutter(UR_OBJECT user, char *inpstr)
{
  static const char usage[] = "Usage: mutter <user> <text>\n";
  const char *type;
  const char *name, *n;
  UR_OBJECT u;

  if (word_count < 3) {
    write_user(user, usage);
    return;
  }
  u = get_user_name(user, word[1]);
  if (!u) {
    write_user(user, notloggedon);
    return;
  }
  if (u == user) {
    write_user(user, "Talking about yourself is a sign of madness!\n");
    return;
  }
  if (user->room != u->room || (!u->vis && user->level < u->level)) {
    vwrite_user(user, "You cannot see %s~RS, so speak freely of them.\n",
                u->recap);
    return;
  }
  inpstr = remove_first(inpstr);
  switch (amsys->ban_swearing) {
  case SBMAX:
    if (contains_swearing(inpstr)) {
      write_user(user, noswearing);
      return;
    }
    break;
  case SBMIN:
    if (!is_private_room(user->room)) {
      inpstr = censor_swear_words(inpstr);
    }
    break;
  case SBOFF:
  default:
    /* do nothing as ban_swearing is off */
    break;
  }
  type = smiley_type(inpstr);
  if (!type) {
    type = "mutter";
  }
  if (!user->vis) {
    write_monitor(user, user->room, 0);
  }
  name = user->vis ? user->recap : invisname;
  n = u->vis ? u->recap : invisname;
  sprintf(text, "(NOT %s~RS) %s~RS ~FC%ss~RS: %s\n", n, name, type,
          inpstr);
#if !!0
  record(user->room, text);
#endif
  write_room_except_both(user->room, text, user, u);
  vwrite_user(user, "(NOT %s~RS) You ~FC%s~RS: %s\n", u->recap, type,
              inpstr);
}


/*
 * ask all the law, (sos), no muzzle restrictions
 */
void
plead(UR_OBJECT user, char *inpstr)
{
  static const char usage[] = "Usage: sos <text>\n";
  UR_OBJECT u;
  const char *type;

  if (word_count < 2) {
    write_user(user, usage);
    return;
  }
  if (user->level >= WIZ) {
    write_user(user, "You are already a wizard!\n");
    return;
  }
  for (u = user_first; u; u = u->next) {
    if (u->level >= WIZ && !u->login) {
      break;
    }
  }
  if (!u) {
    write_user(user, "Sorry, but there are no wizzes currently logged on.\n");
    return;
  }
  switch (amsys->ban_swearing) {
  case SBMAX:
    if (contains_swearing(inpstr)) {
      write_user(user, noswearing);
      return;
    }
    break;
  case SBMIN:
    inpstr = censor_swear_words(inpstr);
    break;
  case SBOFF:
  default:
    /* do nothing as ban_swearing is off */
    break;
  }
  type = smiley_type(inpstr);
  if (!type) {
    type = "plead";
  }
  vwrite_level(WIZ, 1, RECORD, user,
               "~OL~FG>~RS [~FRSOS~RS] %s~RS ~OL%ss~RS: %s\n", user->recap,
               type, inpstr);
  sprintf(text, "~OL~FG>~RS [~FRSOS~RS] You ~OL%s~RS: %s\n", type, inpstr);
  record_tell(user, user, text);
  write_user(user, text);
}


/*
 * Shout something to other wizes and gods. If the level isnt given it
 * defaults to WIZ level.
 */
void
wizshout(UR_OBJECT user, char *inpstr)
{
  static const char usage[] = "Usage: twiz [<level>] <text>\n";
  const char *type;
  enum lvl_value lev;

  /* FIXME: Use sentinel other JAILED */
  if (user->muzzled != JAILED) {
    write_user(user, "You are muzzled, you cannot speak.\n");
    return;
  }
  if (word_count < 2) {
    write_user(user, usage);
    return;
  }
  strtoupper(word[1]);
  lev = get_level(word[1]);
  if (lev == NUM_LEVELS) {
    lev = WIZ;
  } else {
    if (lev < WIZ || word_count < 3) {
      write_user(user, usage);
      return;
    }
    if (lev > user->level) {
      write_user(user,
                 "You cannot specifically shout to users of a higher level than yourself.\n");
      return;
    }
    inpstr = remove_first(inpstr);
  }
  /* Even wizzes cannot escapde the swear ban! MWHAHahaha.... ahem. */
  switch (amsys->ban_swearing) {
  case SBMAX:
    if (contains_swearing(inpstr)) {
      write_user(user, noswearing);
      return;
    }
    break;
  case SBMIN:
    inpstr = censor_swear_words(inpstr);
    break;
  case SBOFF:
  default:
    /* do nothing as ban_swearing is off */
    break;
  }
  type = smiley_type(inpstr);
  if (!type) {
    type = "say";
  }
  vwrite_level(lev, 1, RECORD, user,
               "~OL~FG>~RS [~FY%s~RS] %s~RS ~FY%ss~RS: %s\n",
               user_level[lev].name, user->recap, type, inpstr);
  sprintf(text, "~OL~FG>~RS [~FY%s~RS] You ~FY%s~RS: %s\n",
          user_level[lev].name, type, inpstr);
  record_tell(user, user, text);
  write_user(user, text);
}


/*
 * Emote something to other wizes and gods. If the level isnt given it
 * defaults to WIZ level.
 */
void
wizemote(UR_OBJECT user, char *inpstr)
{
  static const char usage[] = "Usage: ewiz [<level>] <text>\n";
  enum lvl_value lev;

  /* FIXME: Use sentinel other JAILED */
  if (user->muzzled != JAILED) {
    write_user(user, "You are muzzled, you cannot emote.\n");
    return;
  }
  if (word_count < 2) {
    write_user(user, usage);
    return;
  }
  strtoupper(word[1]);
  lev = get_level(word[1]);
  if (lev == NUM_LEVELS) {
    lev = WIZ;
  } else {
    if (lev < WIZ || word_count < 3) {
      write_user(user, usage);
      return;
    }
    if (lev > user->level) {
      write_user(user,
                 "You cannot specifically emote to users of a higher level than yourself.\n");
      return;
    }
    inpstr = remove_first(inpstr);
  }
  switch (amsys->ban_swearing) {
  case SBMAX:
    if (contains_swearing(inpstr)) {
      write_user(user, noswearing);
      return;
    }
    break;
  case SBMIN:
    inpstr = censor_swear_words(inpstr);
    break;
  case SBOFF:
  default:
    /* do nothing as ban_swearing is off */
    break;
  }
  vwrite_level(lev, 1, RECORD, NULL, "~OL~FG>~RS [~FY%s~RS] %s~RS%s%s\n",
               user_level[lev].name, user->recap, *inpstr != '\'' ? " " : "",
               inpstr);
}


/*
 * Displays a picture to a person
 */
void
picture_tell(UR_OBJECT user)
{
  static const char usage[] = "Usage: ptell <user> <picture>\n";
  char filename[80];
  const char *name;
  UR_OBJECT u;
  FILE *fp;

  /* FIXME: Use sentinel other JAILED */
  if (user->muzzled != JAILED) {
    write_user(user, "You are muzzled, you cannot send pictures.\n");
    return;
  }
  if (word_count < 3) {
    write_user(user, usage);
    return;
  }
  u = get_user_name(user, word[1]);
  if (!u) {
    write_user(user, notloggedon);
    return;
  }
  if (u == user) {
    write_user(user, "There is an easier way to see pictures...\n");
    return;
  }
  if (check_igusers(u, user) && user->level < GOD) {
    vwrite_user(user, "%s~RS is ignoring pictures from you.\n", u->recap);
    return;
  }
  if (u->ignpics && (user->level < WIZ || u->level > user->level)) {
    vwrite_user(user, "%s~RS is ignoring pictures at the moment.\n",
                u->recap);
    return;
  }
  if (u->afk) {
    if (*u->afk_mesg) {
      vwrite_user(user, "%s~RS is ~FRAFK~RS, message is: %s\n", u->recap,
                  u->afk_mesg);
    } else {
      vwrite_user(user, "%s~RS is ~FRAFK~RS at the moment.\n", u->recap);
    }
    return;
  }
  if (u->malloc_start) {
    vwrite_user(user, "%s~RS is writing a message at the moment.\n",
                u->recap);
    return;
  }
  if (u->ignall && (user->level < WIZ || u->level > user->level)) {
    vwrite_user(user, "%s~RS is not listening at the moment.\n", u->recap);
    return;
  }
#ifdef NETLINKS
  if (!u->room) {
    vwrite_user(user,
                "%s~RS is offsite and would not be able to reply to you.\n",
                u->recap);
    return;
  }
#endif
  if (strpbrk(word[2], "./")) {
    write_user(user, "Sorry, there is no picture with that name.\n");
    return;
  }
  sprintf(filename, "%s/%s", PICTFILES, word[2]);
  fp = fopen(filename, "r");
  if (!fp) {
    write_user(user, "Sorry, there is no picture with that name.\n");
    return;
  }
  fclose(fp);
  name = user->vis || u->level >= user->level ? user->recap : invisname;
  vwrite_user(u, "%s~RS ~OL~FGshows you the following picture...\n\n", name);
  switch (more(u, u->socket, filename)) {
  case 0:
    break;
  case 1:
    u->misc_op = 2;
    break;
  }
  vwrite_user(user, "You ~OL~FGshow the following picture to~RS %s\n\n",
              u->recap);
  switch (more(user, user->socket, filename)) {
  case 0:
    break;
  case 1:
    user->misc_op = 2;
    break;
  }
}


/*
 * see list of pictures availiable--file dictated in "go" script
 */
void
preview(UR_OBJECT user)
{
#if !!0
  static const char usage[] = "Usage: preview [<picture>]\n";
#endif
  char filename[80], line[100];
  FILE *fp;
  DIR *dirp;
  struct dirent *dp;
  int cnt, total;

  if (word_count < 2) {
    /* open the directory file up */
    dirp = opendir(PICTFILES);
    if (!dirp) {
      write_user(user, "No list of the picture files is availiable.\n");
      return;
    }
    *line = '\0';
    cnt = total = 0;
    /* go through directory and list files */
    for (dp = readdir(dirp); dp; dp = readdir(dirp)) {
      if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, "..")) {
        continue;
      }
      if (!total++) {
        write_user(user,
                   "+----------------------------------------------------------------------------+\n");
        write_user(user,
                   "| ~OL~FCPictures available to view~RS                                                 |\n");
        write_user(user,
                   "+----------------------------------------------------------------------------+\n");
      }
      sprintf(text, "%-12.12s   ", dp->d_name);
      strcat(line, text);
      if (++cnt == 5) {
        write_user(user, align_string(0, 78, 1, "|", "  %s", line));
        *line = '\0';
        cnt = 0;
      }
    }
    closedir(dirp);
    if (total) {
      if (cnt) {
        write_user(user, align_string(0, 78, 1, "|", "  %s", line));
      }
      write_user(user,
                 "+----------------------------------------------------------------------------+\n");
      write_user(user,
                 align_string(0, 78, 1, "|", "  There are %d picture%s  ",
                              total, PLTEXT_S(total)));
      write_user(user,
                 "+----------------------------------------------------------------------------+\n\n");
    } else {
      write_user(user, "There are no pictures available to be viewed.\n");
    }
    return;
  }
  if (strpbrk(word[1], "./")) {
    write_user(user, "Sorry, there is no picture with that name.\n");
    return;
  }
  sprintf(filename, "%s/%s", PICTFILES, word[1]);
  fp = fopen(filename, "r");
  if (!fp) {
    write_user(user, "Sorry, there is no picture with that name.\n");
    return;
  }
  fclose(fp);
  write_user(user, "You ~OL~FGpreview the following picture...\n\n");
  switch (more(user, user->socket, filename)) {
  case 0:
    break;
  case 1:
    user->misc_op = 2;
    break;
  }
}


/*
 * Show a picture to the whole room that the user is in
 */
void
picture_all(UR_OBJECT user)
{
  static const char usage[] = "Usage: picture <picture>\n";
  char filename[80];
  const char *name;
  UR_OBJECT u;
  FILE *fp;

  /* FIXME: Use sentinel other JAILED */
  if (user->muzzled != JAILED) {
    write_user(user, "You are muzzled, you cannot send pictures.\n");
    return;
  }
  if (word_count < 2) {
    write_user(user, usage);
    return;
  }
  if (strpbrk(word[1], "./")) {
    write_user(user, "Sorry, there is no picture with that name.\n");
    return;
  }
  sprintf(filename, "%s/%s", PICTFILES, word[1]);
  fp = fopen(filename, "r");
  if (!fp) {
    write_user(user, "Sorry, there is no picture with that name.\n");
    return;
  }
  fclose(fp);
  for (u = user_first; u; u = u->next) {
    if (u->login || !u->room || (u->room != user->room && user->room)
        || (u->ignall && !force_listen)
        || u->ignpics || u == user) {
      continue;
    }
    if (check_igusers(u, user) && user->level < GOD) {
      continue;
    }
    name = user->vis || u->level > user->level ? user->recap : invisname;
    if (u->type == CLONE_TYPE) {
      if (u->clone_hear == CLONE_HEAR_NOTHING || u->owner->ignall
          || u->clone_hear == CLONE_HEAR_SWEARS) {
        continue;
      }
      /*
       * Ignore anything not in clones room, eg shouts, system messages
       * and shemotes since the clones owner will hear them anyway.
       */
      if (user->room != u->room) {
        continue;
      }
      vwrite_user(u->owner,
                  "~FC[ %s ]:~RS %s~RS ~OL~FGshows the following picture...\n\n",
                  u->room->name, name);
      switch (more(u, u->socket, filename)) {
      case 0:
        break;
      case 1:
        u->misc_op = 2;
        break;
      }
    } else {
      vwrite_user(u, "%s~RS ~OL~FGshows the following picture...\n\n", name);
      switch (more(u, u->socket, filename)) {
      case 0:
        break;
      case 1:
        u->misc_op = 2;
        break;
      }
    }
  }
  write_user(user, "You ~OL~FGshow the following picture to the room...\n\n");
  switch (more(user, user->socket, filename)) {
  case 0:
    break;
  case 1:
    user->misc_op = 2;
    break;
  }
}


/*
 * print out greeting in large letters
 */
void
greet(UR_OBJECT user, char *inpstr)
{
  static const int biglet[26][5][5] = {
    {{0, 1, 1, 1, 0}, {1, 0, 0, 0, 1}, {1, 1, 1, 1, 1}, {1, 0, 0, 0, 1},
     {1, 0, 0, 0, 1}},
    {{1, 1, 1, 1, 0}, {1, 0, 0, 0, 1}, {1, 1, 1, 1, 0}, {1, 0, 0, 0, 1},
     {1, 1, 1, 1, 0}},
    {{0, 1, 1, 1, 1}, {1, 0, 0, 0, 0}, {1, 0, 0, 0, 0}, {1, 0, 0, 0, 0},
     {0, 1, 1, 1, 1}},
    {{1, 1, 1, 1, 0}, {1, 0, 0, 0, 1}, {1, 0, 0, 0, 1}, {1, 0, 0, 0, 1},
     {1, 1, 1, 1, 0}},
    {{1, 1, 1, 1, 1}, {1, 0, 0, 0, 0}, {1, 1, 1, 1, 0}, {1, 0, 0, 0, 0},
     {1, 1, 1, 1, 1}},
    {{1, 1, 1, 1, 1}, {1, 0, 0, 0, 0}, {1, 1, 1, 1, 0}, {1, 0, 0, 0, 0},
     {1, 0, 0, 0, 0}},
    {{0, 1, 1, 1, 0}, {1, 0, 0, 0, 0}, {1, 0, 1, 1, 0}, {1, 0, 0, 0, 1},
     {0, 1, 1, 1, 0}},
    {{1, 0, 0, 0, 1}, {1, 0, 0, 0, 1}, {1, 1, 1, 1, 1}, {1, 0, 0, 0, 1},
     {1, 0, 0, 0, 1}},
    {{0, 1, 1, 1, 0}, {0, 0, 1, 0, 0}, {0, 0, 1, 0, 0}, {0, 0, 1, 0, 0},
     {0, 1, 1, 1, 0}},
    {{0, 0, 0, 0, 1}, {0, 0, 0, 0, 1}, {0, 0, 0, 0, 1}, {1, 0, 0, 0, 1},
     {0, 1, 1, 1, 0}},
    {{1, 0, 0, 0, 1}, {1, 0, 0, 1, 0}, {1, 0, 1, 0, 0}, {1, 0, 0, 1, 0},
     {1, 0, 0, 0, 1}},
    {{1, 0, 0, 0, 0}, {1, 0, 0, 0, 0}, {1, 0, 0, 0, 0}, {1, 0, 0, 0, 0},
     {1, 1, 1, 1, 1}},
    {{1, 0, 0, 0, 1}, {1, 1, 0, 1, 1}, {1, 0, 1, 0, 1}, {1, 0, 0, 0, 1},
     {1, 0, 0, 0, 1}},
    {{1, 0, 0, 0, 1}, {1, 1, 0, 0, 1}, {1, 0, 1, 0, 1}, {1, 0, 0, 1, 1},
     {1, 0, 0, 0, 1}},
    {{0, 1, 1, 1, 0}, {1, 0, 0, 0, 1}, {1, 0, 0, 0, 1}, {1, 0, 0, 0, 1},
     {0, 1, 1, 1, 0}},
    {{1, 1, 1, 1, 0}, {1, 0, 0, 0, 1}, {1, 1, 1, 1, 0}, {1, 0, 0, 0, 0},
     {1, 0, 0, 0, 0}},
    {{0, 1, 1, 1, 0}, {1, 0, 0, 0, 1}, {1, 0, 1, 0, 1}, {1, 0, 0, 1, 1},
     {0, 1, 1, 1, 0}},
    {{1, 1, 1, 1, 0}, {1, 0, 0, 0, 1}, {1, 1, 1, 1, 0}, {1, 0, 0, 1, 0},
     {1, 0, 0, 0, 1}},
    {{0, 1, 1, 1, 1}, {1, 0, 0, 0, 0}, {0, 1, 1, 1, 0}, {0, 0, 0, 0, 1},
     {1, 1, 1, 1, 0}},
    {{1, 1, 1, 1, 1}, {0, 0, 1, 0, 0}, {0, 0, 1, 0, 0}, {0, 0, 1, 0, 0},
     {0, 0, 1, 0, 0}},
    {{1, 0, 0, 0, 1}, {1, 0, 0, 0, 1}, {1, 0, 0, 0, 1}, {1, 0, 0, 0, 1},
     {1, 1, 1, 1, 1}},
    {{1, 0, 0, 0, 1}, {1, 0, 0, 0, 1}, {0, 1, 0, 1, 0}, {0, 1, 0, 1, 0},
     {0, 0, 1, 0, 0}},
    {{1, 0, 0, 0, 1}, {1, 0, 0, 0, 1}, {1, 0, 1, 0, 1}, {1, 1, 0, 1, 1},
     {1, 0, 0, 0, 1}},
    {{1, 0, 0, 0, 1}, {0, 1, 0, 1, 0}, {0, 0, 1, 0, 0}, {0, 1, 0, 1, 0},
     {1, 0, 0, 0, 1}},
    {{1, 0, 0, 0, 1}, {0, 1, 0, 1, 0}, {0, 0, 1, 0, 0}, {0, 0, 1, 0, 0},
     {0, 0, 1, 0, 0}},
    {{1, 1, 1, 1, 1}, {0, 0, 0, 1, 0}, {0, 0, 1, 0, 0}, {0, 1, 0, 0, 0},
     {1, 1, 1, 1, 1}}
  };
  static const int bigsym[32][5][5] = {
    {{0, 0, 1, 0, 0}, {0, 0, 1, 0, 0}, {0, 0, 1, 0, 0}, {0, 0, 0, 0, 0},
     {0, 0, 1, 0, 0}},
    {{0, 1, 0, 1, 0}, {0, 1, 0, 1, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0}},
    {{0, 1, 0, 1, 0}, {1, 1, 1, 1, 1}, {0, 1, 0, 1, 0}, {1, 1, 1, 1, 1},
     {0, 1, 0, 1, 0}},
    {{0, 1, 1, 1, 1}, {1, 0, 1, 0, 0}, {0, 1, 1, 1, 0}, {0, 0, 1, 0, 1},
     {1, 1, 1, 1, 0}},
    {{1, 1, 0, 0, 1}, {1, 1, 0, 1, 0}, {0, 0, 1, 0, 0}, {0, 1, 0, 1, 1},
     {1, 0, 0, 1, 1}},
    {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0}},
    {{0, 0, 1, 0, 0}, {0, 0, 1, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0}},
    {{0, 0, 1, 1, 0}, {0, 1, 0, 0, 0}, {0, 1, 0, 0, 0}, {0, 1, 0, 0, 0},
     {0, 0, 1, 1, 0}},
    {{0, 1, 1, 0, 0}, {0, 0, 0, 1, 0}, {0, 0, 0, 1, 0}, {0, 0, 0, 1, 0},
     {0, 1, 1, 0, 0}},
    {{1, 0, 1, 0, 1}, {0, 1, 1, 1, 0}, {1, 1, 1, 1, 1}, {0, 1, 1, 1, 0},
     {1, 0, 1, 0, 1}},
    {{0, 0, 1, 0, 0}, {0, 0, 1, 0, 0}, {1, 1, 1, 1, 1}, {0, 0, 1, 0, 0},
     {0, 0, 1, 0, 0}},
    {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 1, 0, 0, 0},
     {1, 1, 0, 0, 0}},
    {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {1, 1, 1, 1, 1}, {0, 0, 0, 0, 0},
     {0, 0, 0, 0, 0}},
    {{0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 0, 0, 0}, {1, 1, 0, 0, 0},
     {1, 1, 0, 0, 0}},
    {{0, 0, 0, 0, 1}, {0, 0, 0, 1, 0}, {0, 0, 1, 0, 0}, {0, 1, 0, 0, 0},
     {1, 0, 0, 0, 0}},
    {{0, 1, 1, 1, 0}, {1, 0, 0, 1, 1}, {1, 0, 1, 0, 1}, {1, 1, 0, 0, 1},
     {0, 1, 1, 1, 0}},
    {{0, 0, 1, 0, 0}, {0, 1, 1, 0, 0}, {0, 0, 1, 0, 0}, {0, 0, 1, 0, 0},
     {0, 1, 1, 1, 0}},
    {{1, 1, 1, 1, 0}, {0, 0, 0, 0, 1}, {0, 1, 1, 1, 0}, {1, 0, 0, 0, 0},
     {1, 1, 1, 1, 1}},
    {{1, 1, 1, 1, 0}, {0, 0, 0, 0, 1}, {0, 1, 1, 1, 0}, {0, 0, 0, 0, 1},
     {1, 1, 1, 1, 0}},
    {{0, 0, 1, 1, 0}, {0, 1, 0, 0, 0}, {1, 0, 0, 1, 0}, {1, 1, 1, 1, 1},
     {0, 0, 0, 1, 0}},
    {{1, 1, 1, 1, 1}, {1, 0, 0, 0, 0}, {1, 1, 1, 1, 0}, {0, 0, 0, 0, 1},
     {1, 1, 1, 1, 0}},
    {{0, 1, 1, 1, 0}, {1, 0, 0, 0, 0}, {1, 1, 1, 1, 0}, {1, 0, 0, 0, 1},
     {0, 1, 1, 1, 0}},
    {{1, 1, 1, 1, 1}, {0, 0, 0, 0, 1}, {0, 0, 0, 1, 0}, {0, 0, 1, 0, 0},
     {0, 1, 0, 0, 0}},
    {{0, 1, 1, 1, 0}, {1, 0, 0, 0, 1}, {0, 1, 1, 1, 0}, {1, 0, 0, 0, 1},
     {0, 1, 1, 1, 0}},
    {{1, 1, 1, 1, 1}, {1, 0, 0, 0, 1}, {1, 1, 1, 1, 1}, {0, 0, 0, 0, 1},
     {0, 0, 0, 0, 1}},
    {{0, 0, 0, 0, 0}, {0, 0, 1, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 1, 0, 0},
     {0, 0, 0, 0, 0}},
    {{0, 0, 0, 0, 0}, {0, 0, 1, 0, 0}, {0, 0, 0, 0, 0}, {0, 0, 1, 0, 0},
     {0, 1, 0, 0, 0}},
    {{0, 0, 0, 1, 0}, {0, 0, 1, 0, 0}, {0, 1, 0, 0, 0}, {0, 0, 1, 0, 0},
     {0, 0, 0, 1, 0}},
    {{0, 0, 0, 0, 0}, {1, 1, 1, 1, 1}, {0, 0, 0, 0, 0}, {1, 1, 1, 1, 1},
     {0, 0, 0, 0, 0}},
    {{0, 1, 0, 0, 0}, {0, 0, 1, 0, 0}, {0, 0, 0, 1, 0}, {0, 0, 1, 0, 0},
     {0, 1, 0, 0, 0}},
    {{0, 1, 1, 1, 1}, {0, 0, 0, 0, 1}, {0, 0, 1, 1, 1}, {0, 0, 0, 0, 0},
     {0, 0, 1, 0, 0}},
    {{0, 1, 0, 0, 0}, {1, 0, 1, 1, 1}, {1, 0, 1, 0, 1}, {1, 0, 1, 1, 1},
     {0, 1, 1, 1, 0}}
  };
  static const char usage[] = "Usage: greet <text>\n";
  static const char *const clr[] =
    { "~OL~FR", "~OL~FY", "~OL~FG", "~OL~FC", "~OL~FB", "~OL~FM" };
  char pbuff[ARR_SIZE], temp[8];
  int slen, lc, c, i, j;

  /* FIXME: Use sentinel other JAILED */
  if (user->muzzled != JAILED) {
    write_user(user, "You are muzzled, you cannot greet.\n");
    return;
  }
  if (word_count < 2) {
    write_user(user, usage);
    return;
  }
  switch (amsys->ban_swearing) {
  case SBMAX:
    if (contains_swearing(inpstr)) {
      write_user(user, noswearing);
      return;
    }
    break;
  case SBMIN:
    if (!is_private_room(user->room)) {
      inpstr = censor_swear_words(inpstr);
    }
    break;
  case SBOFF:
  default:
    /* do nothing as ban_swearing is off */
    break;
  }
  slen = strlen(inpstr);
  if (slen > 11) {
    write_user(user, "You can only have up to 11 letters in the greet.\n");
    return;
  }
  write_monitor(user, user->room, 0);
  write_room(user->room, "\n");
  for (i = 0; i < 5; ++i) {
    *pbuff = '\0';
    *temp = '\0';
    for (c = 0; c < slen; ++c) {
      /* check to see if it is a character a-z */
      if (isalpha(inpstr[c])) {
        lc = tolower(inpstr[c]) - 'a';
        if (lc >= 0 && lc < 26) {
          for (j = 0; j < 5; ++j) {
            if (biglet[lc][i][j]) {
              sprintf(temp, "%s#", clr[rand() % SIZEOF(clr)]);
              strcat(pbuff, temp);
            } else {
              strcat(pbuff, " ");
            }
          }
          strcat(pbuff, "  ");
        }
      }
      /* check if it is a character from ! to @ */
      if (ispunct(inpstr[c]) || isdigit(inpstr[c])) {
        lc = inpstr[c] - '!';
        if (lc >= 0 && lc < 32) {
          for (j = 0; j < 5; ++j) {
            if (bigsym[lc][i][j]) {
              sprintf(temp, "%s#", clr[rand() % SIZEOF(clr)]);
              strcat(pbuff, temp);
            } else {
              strcat(pbuff, " ");
            }
          }
          strcat(pbuff, "  ");
        }
      }
    }
    vwrite_room(user->room, "%s\n", pbuff);
  }
}


/*
 * put speech in a think bubbles
 */
void
think_it(UR_OBJECT user, char *inpstr)
{
#if !!0
  static const char usage[] = "Usage: think [<text>]\n";
#endif
  const char *name;

  /* FIXME: Use sentinel other JAILED */
  if (user->muzzled != JAILED) {
    write_user(user, "You are muzzled, you cannot think out loud.\n");
    return;
  }
  switch (amsys->ban_swearing) {
  case SBMAX:
    if (contains_swearing(inpstr)) {
      write_user(user, noswearing);
      return;
    }
    break;
  case SBMIN:
    if (!is_private_room(user->room)) {
      inpstr = censor_swear_words(inpstr);
    }
    break;
  case SBOFF:
  default:
    /* do nothing as ban_swearing is off */
    break;
  }
  if (!user->vis) {
    write_monitor(user, user->room, 0);
  }
  name = user->vis ? user->recap : invisname;
  if (word_count < 2) {
    sprintf(text, "%s~RS thinks nothing--now that is just typical!\n", name);
  } else {
    sprintf(text, "%s~RS thinks . o O ( %s~RS )\n", name, inpstr);
  }
  record(user->room, text);
  write_room(user->room, text);
}


/*
 * put speech in a music notes
 */
void
sing_it(UR_OBJECT user, char *inpstr)
{
#if !!0
  static const char usage[] = "Usage: sing [<text>]\n";
#endif
  const char *name;

  /* FIXME: Use sentinel other JAILED */
  if (user->muzzled != JAILED) {
    write_user(user, "You are muzzled, you cannot sing.\n");
    return;
  }
  switch (amsys->ban_swearing) {
  case SBMAX:
    if (contains_swearing(inpstr)) {
      write_user(user, noswearing);
      return;
    }
    break;
  case SBMIN:
    if (!is_private_room(user->room)) {
      inpstr = censor_swear_words(inpstr);
    }
    break;
  case SBOFF:
  default:
    /* do nothing as ban_swearing is off */
    break;
  }
  if (!user->vis) {
    write_monitor(user, user->room, 0);
  }
  name = user->vis ? user->recap : invisname;
  if (word_count < 2) {
    sprintf(text, "%s~RS sings a tune...BADLY!\n", name);
  } else {
    sprintf(text, "%s~RS sings o/~ %s~RS o/~\n", name, inpstr);
  }
  record(user->room, text);
  write_room(user->room, text);
}


/*
 * Broadcast an important message
 */
void
bcast(UR_OBJECT user, char *inpstr, int beeps)
{
  static const char usage[] = "Usage: bcast <text>\n";
  static const char busage[] = "Usage: bbcast <text>\n";

  /* FIXME: Use sentinel other JAILED */
  if (user->muzzled != JAILED) {
    write_user(user, "You are muzzled, you cannot broadcast.\n");
    return;
  }
  if (word_count < 2) {
    switch (beeps) {
    case 0:
      write_user(user, usage);
      return;
    case 1:
      write_user(user, busage);
      return;
    }
  }
  /* wizzes should be trusted...But they are not! */
  switch (amsys->ban_swearing) {
  case SBMAX:
    if (contains_swearing(inpstr)) {
      write_user(user, noswearing);
      return;
    }
    break;
  case SBMIN:
    inpstr = censor_swear_words(inpstr);
    break;
  case SBOFF:
  default:
    /* do nothing as ban_swearing is off */
    break;
  }
  force_listen = 1;
  write_monitor(user, NULL, 0);
  vwrite_room(NULL, "%s~OL~FR--==<~RS %s~RS ~OL~FR>==--\n",
              beeps ? "\007" : "", inpstr);
}


/*
 * Wake up some idle sod
 */
void
wake(UR_OBJECT user)
{
  static const char usage[] = "Usage: wake <user>\n";
  UR_OBJECT u;
  const char *name;

  /* FIXME: Use sentinel other JAILED */
  if (user->muzzled != JAILED) {
    write_user(user, "You are muzzled, you cannot wake anyone.\n");
    return;
  }
  if (word_count < 2) {
    write_user(user, usage);
    return;
  }
  u = get_user_name(user, word[1]);
  if (!u) {
    write_user(user, notloggedon);
    return;
  }
  if (u == user) {
    write_user(user,
               "Trying to wake yourself up is the eighth sign of madness.\n");
    return;
  }
  if (check_igusers(u, user) && user->level < GOD) {
    vwrite_user(user, "%s~RS is ignoring wakes from you.\n", u->recap);
    return;
  }
  if (u->ignbeeps && (user->level < WIZ || u->level > user->level)) {
    vwrite_user(user, "%s~RS is ignoring wakes at the moment.\n", u->recap);
    return;
  }
  if (u->afk) {
    write_user(user, "You cannot wake someone who is AFK.\n");
    return;
  }
  if (u->malloc_start) {
    write_user(user, "You cannot wake someone who is in the editor.\n");
    return;
  }
  if (u->ignall && (user->level < WIZ || u->level > user->level)) {
    write_user(user, "You cannot wake someone who ignoring everything.\n");
    return;
  }
#ifdef NETLINKS
  if (!u->room) {
    vwrite_user(user,
                "%s~RS is offsite and would not be able to reply to you.\n",
                u->recap);
    return;
  }
#endif
  name = user->vis ? user->recap : invisname;
  vwrite_user(u,
              "\n%s~BR***~RS %s~RS ~BRsays~RS: ~OL~LI~BRHEY! WAKE UP!!!~RS ~BR***\n\n",
              u->ignbeeps ? "" : "\007", name);
  write_user(user, "Wake up call sent.\n");
}


/*
 * Beep a user - as tell but with audio warning
 */
void
beep(UR_OBJECT user, char *inpstr)
{
  static const char usage[] = "Usage: beep <user> [<text>]\n";
  const char *name;
  UR_OBJECT u;

  /* FIXME: Use sentinel other JAILED */
  if (user->muzzled != JAILED) {
    write_user(user, "You are muzzled, you cannot beep.\n");
    return;
  }
  if (word_count < 2) {
    write_user(user, usage);
    return;
  }
  u = get_user_name(user, word[1]);
  if (!u) {
    write_user(user, notloggedon);
    return;
  }
  if (u == user) {
    write_user(user, "Beeping yourself is yet another sign of madness!\n");
    return;
  }
  if (check_igusers(u, user) && user->level < GOD) {
    vwrite_user(user, "%s~RS is ignoring beeps from you.\n", u->recap);
    return;
  }
  if (u->ignbeeps && (user->level < WIZ || u->level > user->level)) {
    vwrite_user(user, "%s~RS is ignoring beeps at the moment.\n", u->recap);
    return;
  }
  if (u->afk) {
    write_user(user, "You cannot beep someone who is AFK.\n");
    return;
  }
  if (u->malloc_start) {
    vwrite_user(user, "%s~RS is writing a message at the moment.\n",
                u->recap);
    return;
  }
  if (u->ignall && (user->level < WIZ || u->level > user->level)) {
    vwrite_user(user, "%s~RS is not listening at the moment.\n", u->recap);
    return;
  }
#ifdef NETLINKS
  if (!u->room) {
    vwrite_user(user,
                "%s~RS is offsite and would not be able to reply to you.\n",
                u->recap);
    return;
  }
#endif
  name = user->vis || u->level >= user->level ? user->recap : invisname;
  if (word_count < 3) {
    vwrite_user(u, "\007%s~RS ~OL~FRbeeps to you~RS: ~FR-=[*] BEEP [*]=-\n",
                name);
    vwrite_user(user, "\007You ~OL~FRbeep to~RS %s~RS: ~FR-=[*] BEEP [*]=-\n",
                u->recap);
    return;
  }
  inpstr = remove_first(inpstr);
  switch (amsys->ban_swearing) {
  case SBMAX:
    if (contains_swearing(inpstr)) {
      write_user(user, noswearing);
      return;
    }
    break;
  case SBMIN:
    inpstr = censor_swear_words(inpstr);
    break;
  case SBOFF:
  default:
    /* do nothing as ban_swearing is off */
    break;
  }
  vwrite_user(u, "\007%s~RS ~OL~FRbeeps to you~RS: %s\n", name, inpstr);
  vwrite_user(user, "\007You ~OL~FRbeep to~RS %s~RS: %s\n", u->recap,
              inpstr);
}


/*
 * set a name for a quick call
 */
void
quick_call(UR_OBJECT user)
{
#if !!0
  static const char usage[] = "Usage: call [<user>]\n";
#endif
  UR_OBJECT u;

  if (word_count < 2) {
    if (!*user->call) {
      write_user(user, "Quick call not set.\n");
      return;
    }
    vwrite_user(user, "Quick call to: %s.\n", user->call);
    return;
  }
  u = get_user_name(user, word[1]);
  if (!u) {
    write_user(user, notloggedon);
    return;
  }
  if (u == user) {
    write_user(user, "You cannot set your quick call to yourself.\n");
    return;
  }
  strcpy(user->call, u->name);
  *user->call = toupper(*user->call);
  vwrite_user(user, "You have set a quick call to: %s.\n", user->call);
}


/*
 * Show recorded tells and pemotes
 */
void
revtell(UR_OBJECT user)
{
#if !!0
  static const char usage[] = "Usage: revtell\n";
#endif

  start_pager(user);
  write_user(user, "\n~BB~FG*** Your tell buffer ***\n\n");
  if (!review_buffer(user, rbfTELL)) {
    write_user(user, "Revtell buffer is empty.\n");
  }
  write_user(user, "\n~BB~FG*** End ***\n\n");
  stop_pager(user);
}


/*
 * Show recorded tells and pemotes
 */
void
revedit(UR_OBJECT user)
{
#if !!0
  static const char usage[] = "Usage: revedit\n";
#endif

  start_pager(user);
  write_user(user, "\n~BB~FG*** Your EDIT review buffer ***\n\n");
  if (!review_buffer(user, rbfEDIT)) {
    write_user(user, "EDIT buffer is empty.\n");
  }
  write_user(user, "\n~BB~FG*** End ***\n\n");
  stop_pager(user);
}


/*
 * Show recorded tells and pemotes
 */
void
revafk(UR_OBJECT user)
{
#if !!0
  static const char usage[] = "Usage: revafk\n";
#endif

  start_pager(user);
  write_user(user, "\n~BB~FG*** Your AFK review buffer ***\n\n");
  if (!review_buffer(user, rbfAFK)) {
    write_user(user, "AFK buffer is empty.\n");
  }
  write_user(user, "\n~BB~FG*** End ***\n\n");
  stop_pager(user);
}


/*
 * See review of shouts
 */
void
revshout(UR_OBJECT user)
{
#if !!0
  static const char usage[] = "Usage: revshout\n";
#endif
  int i, line, cnt;

  cnt = 0;
  start_pager(user);
  if (user->reverse_buffer) {
    for (i = REVIEW_LINES - 1; i >= 0; --i) {
      line = (amsys->sbuffline + i) % REVIEW_LINES;
      if (*amsys->shoutbuff[line]) {
        if (!cnt++) {
          write_user(user, "~BB~FG*** Shout review buffer ***\n\n");
        }
        write_user(user, amsys->shoutbuff[line]);
      }
    }
  } else {
    for (i = 0; i < REVIEW_LINES; ++i) {
      line = (amsys->sbuffline + i) % REVIEW_LINES;
      if (*amsys->shoutbuff[line]) {
        if (!cnt++) {
          write_user(user, "~BB~FG*** Shout review buffer ***\n\n");
        }
        write_user(user, amsys->shoutbuff[line]);
      }
    }
  }
  if (!cnt) {
    write_user(user, "Shout review buffer is empty.\n");
  } else {
    write_user(user, "\n~BB~FG*** End ***\n\n");
  }
  stop_pager(user);
}


/*
 * See review of conversation
 */
void
review(UR_OBJECT user)
{
#if !!0
  static const char usage[] = "Usage: review\n";
#endif
  RM_OBJECT rm;
  int i, line, cnt;

  if (word_count < 2 || user->level < GOD) {
    rm = user->room;
  } else {
    rm = get_room(word[1]);
    if (!rm) {
      write_user(user, nosuchroom);
      return;
    }
    if (!has_room_access(user, rm)) {
      write_user(user,
                 "That room is currently private, you cannot review the conversation.\n");
      return;
    }
    vwrite_user(user, "~FC(Review of %s room)\n", rm->name);
  }
  cnt = 0;
  start_pager(user);
  if (user->reverse_buffer) {
    for (i = REVIEW_LINES - 1; i >= 0; --i) {
      line = (rm->revline + i) % REVIEW_LINES;
      if (*rm->revbuff[line]) {
        if (!cnt++) {
          write_user(user, "\n~BB~FG*** Room conversation buffer ***\n\n");
        }
        write_user(user, rm->revbuff[line]);
      }
    }
  } else {
    for (i = 0; i < REVIEW_LINES; ++i) {
      line = (rm->revline + i) % REVIEW_LINES;
      if (*rm->revbuff[line]) {
        if (!cnt++) {
          write_user(user, "\n~BB~FG*** Room conversation buffer ***\n\n");
        }
        write_user(user, rm->revbuff[line]);
      }
    }
  }
  if (!cnt) {
    write_user(user, "Review buffer is empty.\n");
  } else {
    write_user(user, "\n~BB~FG*** End ***\n\n");
  }
  stop_pager(user);
}


/*
 * review from the user review buffer
 */
int
review_buffer(UR_OBJECT user, unsigned flags)
{
  int count = 0;
  RB_OBJECT rb, next;

  if (user->reverse_buffer) {
    for (rb = user->rb_last; rb; rb = next) {
      next = rb->prev;
      if (rb->flags & flags) {
        write_user(user, rb->buffer);
        ++count;
      }
    }
  } else {
    for (rb = user->rb_first; rb; rb = next) {
      next = rb->next;
      if (rb->flags & flags) {
        write_user(user, rb->buffer);
        ++count;
      }
    }
  }
  return count;
}


/*
 * Record speech and emotes in the room.
 */
void
record(RM_OBJECT rm, char *str)
{
  *rm->revbuff[rm->revline] = '\0';
  strncat(rm->revbuff[rm->revline], str, REVIEW_LEN);
  rm->revbuff[rm->revline][REVIEW_LEN] = '\n';
  rm->revbuff[rm->revline][REVIEW_LEN + 1] = '\0';
  rm->revline = (rm->revline + 1) % REVIEW_LINES;
}


/*
 * Records shouts and shemotes sent over the talker.
 */
void
record_shout(const char *str)
{
  *amsys->shoutbuff[amsys->sbuffline] = '\0';
  strncat(amsys->shoutbuff[amsys->sbuffline], str, REVIEW_LEN);
  amsys->shoutbuff[amsys->sbuffline][REVIEW_LEN] = '\n';
  amsys->shoutbuff[amsys->sbuffline][REVIEW_LEN + 1] = '\0';
  amsys->sbuffline = (amsys->sbuffline + 1) % REVIEW_LINES;
}


/*
 * Records tells and pemotes sent to the user.
 */
void
record_tell(UR_OBJECT from, UR_OBJECT to, const char *str)
{
  int count;

  if (!create_review_buffer_entry(to, !from ? "?" : from->name, str, rbfTELL)) {
    write_syslog(ERRLOG, 1,
                 "Could not create tell review buffer entry for %s.\n",
                 to->name);
    return;
  }
  /* check to see if we need to prune */
  count = has_review(to, rbfTELL);
  if (count > REVTELL_LINES) {
    destruct_review_buffer_type(to, rbfTELL, 1);
  }
}


/*
 * Records tells and pemotes sent to the user when afk.
 */
void
record_afk(UR_OBJECT from, UR_OBJECT to, const char *str)
{
  int count;

  if (!create_review_buffer_entry(to, !from ? "?" : from->name, str, rbfAFK)) {
    write_syslog(ERRLOG, 1,
                 "Could not create afk review buffer entry for %s.\n",
                 to->name);
    return;
  }
  /* check to see if we need to prune */
  count = has_review(to, rbfAFK);
  if (count > REVTELL_LINES) {
    destruct_review_buffer_type(to, rbfAFK, 1);
  }
}


/*
 * Records tells and pemotes sent to the user when in the line editor.
 */
void
record_edit(UR_OBJECT from, UR_OBJECT to, const char *str)
{
  int count;

  if (!create_review_buffer_entry(to, !from ? "?" : from->name, str, rbfEDIT)) {
    write_syslog(ERRLOG, 1,
                 "Could not create edit review buffer entry for %s.\n",
                 to->name);
    return;
  }
  /* check to see if we need to prune */
  count = has_review(to, rbfEDIT);
  if (count > REVTELL_LINES) {
    destruct_review_buffer_type(to, rbfEDIT, 1);
  }
}


/*
 * Clear the review buffer
 */
void
revclr(UR_OBJECT user)
{
#if !!0
  static const char usage[] = "Usage: cbuff\n";
#endif
  const char *name;

  clear_revbuff(user->room);
  name = user->vis ? user->recap : invisname;
  vwrite_room_except(user->room, user, "%s~RS clears the review buffer.\n",
                     name);
  write_user(user, "You clear the review buffer.\n");
}


/*
* Clear the tell buffer of the user
*/
void
clear_afk(UR_OBJECT user)
{
  destruct_review_buffer_type(user, rbfAFK, 0);
}


/*
 * Clear the review buffer in the room
 */
void
clear_revbuff(RM_OBJECT rm)
{
  int i;

  for (i = 0; i < REVIEW_LINES; ++i) {
    *rm->revbuff[i] = '\0';
  }
  rm->revline = 0;
}


/*
 * Clear the tell buffer of the user
 */
void
clear_tells(UR_OBJECT user)
{
  destruct_review_buffer_type(user, rbfTELL, 0);
}


/*
 * Clear the shout buffer of the talker
 */
void
clear_shouts(void)
{
  int i;

  for (i = 0; i < REVIEW_LINES; ++i) {
    *amsys->shoutbuff[i] = '\0';
  }
  amsys->sbuffline = 0;
}


/*
 * Clear the tell buffer of the user
 */
void
clear_edit(UR_OBJECT user)
{
  destruct_review_buffer_type(user, rbfEDIT, 0);
}

/*
 * count up how many review buffers of a certain type
 */
int
has_review(UR_OBJECT user, unsigned flags)
{
  int count = 0;
  RB_OBJECT rb;

  for (rb = user->rb_first; rb; rb = rb->next) {
    if (rb->flags & flags) {
      ++count;
    }
  }
  return count;
}


/*
 * Clear the screen
 */
void
cls(UR_OBJECT user)
{
  int i;
  /* For clients that don't support telnet clear screen escape character, we flood them with new lines... */
  for (i = 0; i < 6; ++i) {
    write_user(user, "\n\n\n\n\n\n\n\n\n\n");
  }
  /* ...and for the others, here's The Real Thing (TM) */
  write_user(user, "\x01B[2J");
}
 

/*
 * Make a clone speak
 */
void
clone_say(UR_OBJECT user, char *inpstr)
{
  static const char usage[] = "Usage: csay <room> <text>\n";
  RM_OBJECT rm;
  UR_OBJECT u;

  /* FIXME: Use sentinel other JAILED */
  if (user->muzzled != JAILED) {
    write_user(user, "You are muzzled, your clone cannot speak.\n");
    return;
  }
  if (word_count < 3) {
    write_user(user, usage);
    return;
  }
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
  say(u, remove_first(inpstr));
}


/*
 * Make a clone emote
 */
void
clone_emote(UR_OBJECT user, char *inpstr)
{
  static const char usage[] = "Usage: cemote <room> <text>\n";
  RM_OBJECT rm;
  UR_OBJECT u;

  /* FIXME: Use sentinel other JAILED */
  if (user->muzzled != JAILED) {
    write_user(user, "You are muzzled, your clone cannot emote.\n");
    return;
  }
  if (word_count < 3) {
    write_user(user, usage);
    return;
  }
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
  emote(u, remove_first(inpstr));
}


/*
 * Set what a clone will hear, either all speach , just bad language
 * or nothing.
 */
void
clone_hear(UR_OBJECT user)
{
  static const char usage[] = "Usage: chear <room> all|swears|nothing\n";
  RM_OBJECT rm;
  UR_OBJECT u;

  if (word_count < 3) {
    write_user(user, usage);
    return;
  }
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
  strtolower(word[2]);
  if (!strcmp(word[2], "all")) {
    u->clone_hear = CLONE_HEAR_ALL;
    write_user(user, "Clone will now hear everything.\n");
    return;
  }
  if (!strcmp(word[2], "swears")) {
    u->clone_hear = CLONE_HEAR_SWEARS;
    write_user(user, "Clone will now only hear swearing.\n");
    return;
  }
  if (!strcmp(word[2], "nothing")) {
    u->clone_hear = CLONE_HEAR_NOTHING;
    write_user(user, "Clone will now hear nothing.\n");
    return;
  }
  write_user(user, usage);
}


/*
 * Show command, i.e., "Type --> <text>"
 */
void
show(UR_OBJECT user, char *inpstr)
{
  static const char usage[] = "Usage: show <text>\n";

  /* FIXME: Use sentinel other JAILED */
  if (user->muzzled != JAILED) {
    write_user(user, "You are currently muzzled and cannot show.\n");
    return;
  }
  if (word_count < 2 && strlen(inpstr) < 2) {
    write_user(user, usage);
    return;
  }
  switch (amsys->ban_swearing) {
  case SBMAX:
    if (contains_swearing(inpstr)) {
      write_user(user, noswearing);
      return;
    }
    break;
  case SBMIN:
    if (!is_private_room(user->room)) {
      inpstr = censor_swear_words(inpstr);
    }
    break;
  case SBOFF:
  default:
    /* do nothing as ban_swearing is off */
    break;
  }
  write_monitor(user, user->room, 0);
  vwrite_room(user->room, "~OL~FCType -->~RS %s\n", inpstr);
}


/*
 * Say user speech to all people listed on users friends list
 */
void
friend_say(UR_OBJECT user, char *inpstr)
{
  static const char usage[] = "Usage: fsay <text>\n";
  const char *type;
  const char *name;

  /* FIXME: Use sentinel other JAILED */
  if (user->muzzled != JAILED) {
    write_user(user, "You are muzzled, you cannot speak.\n");
    return;
  }
  if (word_count < 2) {
    write_user(user, usage);
    return;
  }
  /* check to see if use has friends listed */
  if (!count_friends(user)) {
    write_user(user, "You have no friends listed.\n");
    return;
  }
  /* sort our swearing */
  switch (amsys->ban_swearing) {
  case SBMAX:
    if (contains_swearing(inpstr)) {
      write_user(user, noswearing);
      return;
    }
    break;
  case SBMIN:
    inpstr = censor_swear_words(inpstr);
    break;
  case SBOFF:
  default:
    /* do nothing as ban_swearing is off */
    break;
  }
  type = smiley_type(inpstr);
  if (!type) {
    type = "say";
  }
  name = user->vis ? user->recap : invisname;
  sprintf(text, "~OL~FG>~RS [~FGFriend~RS] %s~RS ~FG%ss~RS: %s\n", name,
          type, inpstr);
  write_friends(user, text, 1);
  sprintf(text, "~OL~FG>~RS [~FGFriend~RS] You ~FG%s~RS: %s\n", type,
          inpstr);
  record_tell(user, user, text);
  write_user(user, text);
}


/*
 * Emote something to all the people on the suers friends list
 */
void
friend_emote(UR_OBJECT user, char *inpstr)
{
  static const char usage[] = "Usage: femote <text>\n";
  const char *name;

  /* FIXME: Use sentinel other JAILED */
  if (user->muzzled != JAILED) {
    write_user(user, "You are muzzled, you cannot emote.\n");
    return;
  }
  if (word_count < 2) {
    write_user(user, usage);
    return;
  }
  /* check to see if use has friends listed */
  if (!count_friends(user)) {
    write_user(user, "You have no friends listed.\n");
    return;
  }
  /* sort out swearing */
  switch (amsys->ban_swearing) {
  case SBMAX:
    if (contains_swearing(inpstr)) {
      write_user(user, noswearing);
      return;
    }
    break;
  case SBMIN:
    inpstr = censor_swear_words(inpstr);
    break;
  case SBOFF:
  default:
    /* do nothing as ban_swearing is off */
    break;
  }
  name = user->vis ? user->recap : invisname;
  sprintf(text, "~OL~FG>~RS [~FGFriend~RS] %s~RS%s%s\n", name,
          *inpstr != '\'' ? " " : "", inpstr);
  write_friends(user, text, 1);
  record_tell(user, user, text);
  write_user(user, text);
}
