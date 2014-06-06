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
 * needs only doing once when booting
 */
void
count_suggestions(void)
{
  char filename[80], line[82], id[20], *s, *str;
  FILE *fp;
  int valid;

  sprintf(filename, "%s/%s", MISCFILES, SUGBOARD);
  fp = fopen(filename, "r");
  if (!fp) {
    return;
  }
  valid = 1;
  for (s = fgets(line, 82, fp); s; s = fgets(line, 82, fp)) {
    if (*s == '\n') {
      valid = 1;
    }
    sscanf(s, "%s", id);
    str = colour_com_strip(id);
    if (valid && !strcmp(str, "From:")) {
      ++amsys->suggestion_count;
      valid = 0;
    }
  }
  fclose(fp);
}


/*
 * work out how many motds are stored - if an motd file is deleted after
 * this count has been made then you need to recount them with the recount
 * command.  If you fail to do this and something buggers up because the count
 * is wrong then it is your own fault.
 */
int
count_motds(int forcecnt)
{
	char filename[80];
	DIR *dirp;
	struct dirent *dp;
	int i;

	amsys->motd1_cnt = 0;
	amsys->motd2_cnt = 0;
	for (i = 1; i <= 2; ++i) {
		/* open the directory file up */
		sprintf(filename, "%s/motd%d", MOTDFILES, i);
		dirp = opendir(filename);
		if (!dirp) {
			if (!forcecnt) {
				fprintf(stderr, "Amnuts: Directory open failure in count_motds().\n");
				boot_exit(20);
			}
			return 0;
		}
		/* count up the motd files */
		while ((dp = readdir(dirp)) != NULL) {
			if (strncmp(dp->d_name, "motd", 4) != 0) {
				continue;
			}
			if (i == 1) {
				++amsys->motd1_cnt;
			} else {
				++amsys->motd2_cnt;
			}
		}
		closedir(dirp);
	}
	return 1;
}


/*
 * return a random number for an motd file - if 0 then return 1
 */
int
get_motd_num(int motd)
{
  int num;

  if (!amsys->random_motds) {
    return 1;
  }
  switch (motd) {
  case 1:
    num = rand() % amsys->motd1_cnt + 1;
    break;
  case 2:
    num = rand() % amsys->motd2_cnt + 1;
    break;
  default:
    num = 0;
    break;
  }
  return !num ? 1 : num;
}


/*
 * Remove any expired messages from boards unless force = 2 in which case
 * just do a recount.
 */
void
check_messages(UR_OBJECT user, int chforce)
{
  char id[82], filename[80], line[82], *s;
  FILE *infp, *outfp;
  RM_OBJECT rm;
  int valid, pt, write_rest, board_cnt, old_cnt, bad_cnt, tmp;

  infp = outfp = NULL;
  switch (chforce) {
  case 0:
    break;
  case 1:
    printf("Checking boards...\n");
    break;
  case 2:
    if (word_count >= 2) {
      strtolower(word[1]);
      if (strcmp(word[1], "motds")) {
        write_user(user, "Usage: recount [motds]\n");
        return;
      }
      if (!count_motds(1)) {
        write_user(user,
                   "Sorry, could not recount the motds at this time.\n");
        write_syslog(SYSLOG | ERRLOG, 1,
                     "ERROR: Could not recount motds in check_messages().\n");
        return;
      }
      vwrite_user(user, "There %s %d login motd%s and %d post-login motd%s\n",
                  PLTEXT_WAS(amsys->motd1_cnt), amsys->motd1_cnt,
                  PLTEXT_S(amsys->motd1_cnt), amsys->motd2_cnt,
                  PLTEXT_S(amsys->motd2_cnt));
      write_syslog(SYSLOG, 1, "%s recounted the MOTDS.\n", user->name);
      return;
    }
    break;
  }
  board_cnt = 0;
  old_cnt = 0;
  bad_cnt = 0;
  for (rm = room_first; rm; rm = rm->next) {
    tmp = rm->mesg_cnt;
    rm->mesg_cnt = 0;
    if (is_personal_room(rm)) {
      sprintf(filename, "%s/%s/%s.B", USERFILES, USERROOMS, rm->owner);
    } else {
      sprintf(filename, "%s/%s.B", DATAFILES, rm->name);
    }
    infp = fopen(filename, "r");
    if (!infp) {
      continue;
    }
    if (chforce < 2) {
      outfp = fopen("tempfile", "w");
      if (!outfp) {
        if (chforce) {
          fprintf(stderr, "Amnuts: Cannot open tempfile.\n");
        }
        write_syslog(SYSLOG | ERRLOG, 0,
                     "ERROR: Cannot open tempfile in check_messages().\n");
        fclose(infp);
        return;
      }
    }
    ++board_cnt;
    /*
       We assume that once 1 in date message is encountered all the others
       will be in date too , hence write_rest once set to 1 is never set to
       0 again
     */
    valid = 1;
    write_rest = 0;
    /* max of 80+newline+terminator = 82 */
    for (s = fgets(line, 82, infp); s; s = fgets(line, 82, infp)) {
      if (*s == '\n') {
        valid = 1;
      }
      sscanf(s, "%s %d", id, &pt);
      if (!write_rest) {
        if (valid && !strcmp(id, "PT:")) {
          if (chforce == 2) {
            ++rm->mesg_cnt;
          } else {
            /* 86400 = num. of secs in a day */
            if ((int) time(0) - pt < amsys->mesg_life * 86400) {
              fputs(s, outfp);
              ++rm->mesg_cnt;
              write_rest = 1;
            } else {
              ++old_cnt;
            }
          }
          valid = 0;
        }
      } else {
        fputs(s, outfp);
        if (valid && !strcmp(id, "PT:")) {
          ++rm->mesg_cnt;
          valid = 0;
        }
      }
    }
    fclose(infp);
    if (chforce < 2) {
      fclose(outfp);
      remove(filename);
      if (!write_rest) {
        remove("tempfile");
      } else {
        rename("tempfile", filename);
      }
    }
    if (rm->mesg_cnt != tmp) {
      ++bad_cnt;
    }
  }
  switch (chforce) {
  case 0:
    if (bad_cnt) {
      write_syslog(SYSLOG, 1,
                   "CHECK_MESSAGES: %d file%s checked, %d had an incorrect message count, %d message%s deleted.\n",
                   board_cnt, PLTEXT_S(board_cnt), bad_cnt, old_cnt,
                   PLTEXT_S(old_cnt));
    } else {
      write_syslog(SYSLOG, 1,
                   "CHECK_MESSAGES: %d file%s checked, %d message%s deleted.\n",
                   board_cnt, PLTEXT_S(board_cnt), old_cnt,
                   PLTEXT_S(old_cnt));
    }
    break;
  case 1:
    printf("  %d board file%s checked, %d out of date message%s found.\n",
           board_cnt, PLTEXT_S(board_cnt), old_cnt, PLTEXT_S(old_cnt));
    break;
  case 2:
    vwrite_user(user,
                "%d board file%s checked, %d had an incorrect message count.\n",
                board_cnt, PLTEXT_S(board_cnt), bad_cnt);
    write_syslog(SYSLOG, 1, "%s forced a recount of the message boards.\n",
                 user->name);
    break;
  }
}


/*
 * This is function that sends mail to other users
 */
int
send_mail(UR_OBJECT user, char *to, char *ptr, int iscopy)
{
  char filename[80], header[ARR_SIZE];
  const char *cc;
  struct stat stbuf;
  FILE *infp, *outfp;
  int c, amount, size, tmp1, tmp2;

#ifdef NETLINKS
  tmp1 = mail_nl(user, to, ptr);
  if (tmp1) {
    return tmp1 != -1;
  }
#endif
  outfp = fopen("tempfile", "w");
  if (!outfp) {
    write_user(user, "Error in mail delivery.\n");
    write_syslog(SYSLOG | ERRLOG, 0,
                 "ERROR: Cannot open tempfile in send_mail().\n");
    return 0;
  }
  /* Copy current mail file into tempfile if it exists */
  sprintf(filename, "%s/%s/%s.M", USERFILES, USERMAILS, to);
  /* but first get the original sizes and write those to the temp file */
  amount = mail_sizes(to, 1);   /* amount of new mail */
  if (!amount) {
    size = stat(filename, &stbuf) == -1 ? 0 : stbuf.st_size;
  } else {
    size = mail_sizes(to, 2);
  }
  fprintf(outfp, "%d %d\r", ++amount, size);
  infp = fopen(filename, "r");
  if (infp) {
    /* Discard first line of mail file. */
    fscanf(infp, "%d %d\r", &tmp1, &tmp2);
    /* Copy rest of file */
    for (c = getc(infp); c != EOF; c = getc(infp)) {
      putc(c, outfp);
    }
    fclose(infp);
  }
  cc = iscopy == 1 ? "(CC)" : "";
  *header = '\0';
  /* Put new mail in tempfile */
  if (user) {
#ifdef NETLINKS
    if (user->type == REMOTE_TYPE) {
      sprintf(header, "~OLFrom: %s@%s  %s %s\n", user->bw_recap,
              user->netlink->service, long_date(0), cc);
    } else
#endif
      sprintf(header, "~OLFrom: %s  %s %s\n", user->bw_recap, long_date(0),
              cc);
  } else {
    sprintf(header, "~OLFrom: MAILER  %s %s\n", long_date(0), cc);
  }
  fputs(header, outfp);
  fputs(ptr, outfp);
  fputs("\n", outfp);
  fclose(outfp);
  rename("tempfile", filename);
  switch (iscopy) {
  case 0:
    vwrite_user(user, "Mail is delivered to %s\n", to);
    break;
  case 1:
    vwrite_user(user, "Mail is copied to %s\n", to);
    break;
  case 2:
    break;
  }
  if (user && !iscopy) {
    write_syslog(SYSLOG, 1, "%s sent mail to %s\n", user->name, to);
  }
  write_user(get_user(to), "\07~FC~OL~LI** YOU HAVE NEW MAIL **\n");
  forward_email(to, header, ptr);
  return 1;
}


/*
 * Send mail message
 */
void
smail(UR_OBJECT user, char *inpstr)
{
  if (inpstr) {
    static const char usage[] = "Usage: smail <user>[@<service>] [<text>]\n";
    char *remote;

    /* FIXME: Use sentinel other JAILED */
    if (user->muzzled != JAILED) {
      write_user(user, "You are muzzled, you cannot mail anyone.\n");
      return;
    }
    if (word_count < 2) {
      write_user(user, usage);
      return;
    }
    *word[1] = toupper(*word[1]);
    remote = strchr(word[1], '@');
    /* See if user exists */
    if (!remote) {
      UR_OBJECT u = get_user(word[1]);

      /* See if user has local account */
      if (!find_user_listed(word[1])) {
        if (!u) {
          write_user(user, nosuchuser);
        } else {
          vwrite_user(user,
                      "%s is a remote user and does not have a local account.\n",
                      u->name);
        }
        return;
      }
      if (u) {
        if (u == user && user->level < ARCH) {
          write_user(user,
                     "Trying to mail yourself is the fifth sign of madness.\n");
          return;
        }
        /* FIXME: Should check for offline users as well */
        if (check_igusers(u, user) && user->level < GOD) {
          vwrite_user(user, "%s~RS is ignoring smails from you.\n", u->recap);
          return;
        }
        strcpy(word[1], u->name);
      }
    } else if (remote == word[1]) {
      write_user(user, "Users name missing before @ sign.\n");
      return;
    }
    strcpy(user->mail_to, word[1]);
    if (word_count < 3) {
#ifdef NETLINKS
      if (user->type == REMOTE_TYPE) {
        write_user(user,
                   "Sorry, due to software limitations remote users cannot use the line editor.\nUse the \".smail <user> <text>\" method instead.\n");
        return;
      }
#endif
      vwrite_user(user, "\n~BB*** Writing mail message to %s ***\n\n",
                  user->mail_to);
      user->misc_op = 4;
      editor(user, NULL);
      return;
    }
    /* One line mail */
    strcat(inpstr, "\n");
    inpstr = remove_first(inpstr);
  } else {
    if (*user->malloc_end-- != '\n') {
      *user->malloc_end-- = '\n';
    }
    inpstr = user->malloc_start;
  }
  strtoname(user->mail_to);
  send_mail(user, user->mail_to, inpstr, 0);
  send_copies(user, inpstr);
  *user->mail_to = '\0';
}


/*
 * Read your mail
 */
void
rmail(UR_OBJECT user)
{
  char filename[80];
  int ret;

  sprintf(filename, "%s/%s/%s.M", USERFILES, USERMAILS, user->name);

  /* Just reading the one message or all new mail */
  if (word_count > 1) {
    strtolower(word[1]);
    if (!strcmp(word[1], "new")) {
      read_new_mail(user);
    } else {
      read_specific_mail(user);
    }
    return;
  }
  /* Update last read / new mail received time at head of file */
  if (!reset_mail_counts(user)) {
    write_user(user, "You do not have any mail.\n");
    return;
  }
  /* Reading the whole mail box */
  write_user(user, "\n~BB*** Your mailbox has the following messages ***\n\n");
  ret = more(user, user->socket, filename);
  if (ret == 1) {
    user->misc_op = 2;
  }
}


/*
 * allows a user to choose what message to read
 */
void
read_specific_mail(UR_OBJECT user)
{
  char w1[ARR_SIZE], line[ARR_SIZE], filename[80], *s;
  FILE *fp;
  int valid, cnt, total, smail_number, tmp1, tmp2;

  if (word_count > 2) {
    write_user(user, "Usage: rmail [new/<message #>]\n");
    return;
  }
  total = mail_sizes(user->name, 0);
  if (!total) {
    write_user(user, "You currently have no mail.\n");
    return;
  }
  smail_number = atoi(word[1]);
  if (!smail_number) {
    write_user(user, "Usage: rmail [new/<message #>]\n");
    return;
  }
  if (smail_number > total) {
    vwrite_user(user, "You only have %d message%s in your mailbox.\n", total,
                PLTEXT_S(total));
    return;
  }
  /* Update last read / new mail received time at head of file */
  if (!reset_mail_counts(user)) {
    write_user(user, "You do not have any mail.\n");
    return;
  }
  sprintf(filename, "%s/%s/%s.M", USERFILES, USERMAILS, user->name);
  fp = fopen(filename, "r");
  if (!fp) {
    write_user(user, "There was an error trying to read your mailbox.\n");
    write_syslog(SYSLOG | ERRLOG, 0,
                 "Unable to open %s's mailbox in read_mail_specific.\n",
                 user->name);
    return;
  }
  valid = cnt = 1;
  fscanf(fp, "%d %d\r", &tmp1, &tmp2);
  for (s = fgets(line, ARR_SIZE - 1, fp); s;
       s = fgets(line, ARR_SIZE - 1, fp)) {
    if (*s == '\n') {
      valid = 1;
    }
    sscanf(s, "%s", w1);
    if (valid && !strcmp(colour_com_strip(w1), "From:")) {
      valid = 0;
      if (smail_number == cnt++) {
        break;
      }
    }
  }
  write_user(user, "\n");
  for (; s; s = fgets(line, ARR_SIZE - 1, fp)) {
    if (*s == '\n') {
      break;
    }
    write_user(user, s);
  }
  fclose(fp);
  vwrite_user(user,
              "\nMail message number ~FM~OL%d~RS out of ~FM~OL%d~RS.\n\n",
              smail_number, total);
}


/*
 * Read just the new mail, taking the fseek size from the stat st_buf saved in the
 * mail file first line - along with how many new mail messages there are
 */
void
read_new_mail(UR_OBJECT user)
{
  char filename[80];
  int total, newcnt;

  /* Get total number of mail */
  total = mail_sizes(user->name, 0);
  if (!total) {
    write_user(user, "You do not have any mail.\n");
    return;
  }
  /* Get the amount of new mail */
  newcnt = mail_sizes(user->name, 1);
  if (!newcnt) {
    write_user(user, "You do not have any new mail.\n");
    return;
  }
  if (newcnt == total) {
    /* Update last read / new mail received time at head of file */
    if (!(reset_mail_counts(user))) {
      write_user(user, "You do not have any mail.\n");
      return;
    }
    sprintf(filename, "%s/%s/%s.M", USERFILES, USERMAILS, user->name);
    write_user(user,
               "\n~BB*** These are the new mail messages you have in your mailbox ***\n\n");
    more(user, user->socket, filename);
    return;
  }
  /* Get where new mail starts */
  user->filepos = mail_sizes(user->name, 2);
  /* Update last read / new mail received time at head of file */
  if (!reset_mail_counts(user)) {
    write_user(user, "You do not have any mail.\n");
    return;
  }
  sprintf(filename, "%s/%s/%s.M", USERFILES, USERMAILS, user->name);
  write_user(user,
             "\n~BB*** These are the new mail messages you have in your mailbox ***\n\n");
  if (more(user, user->socket, filename) != 1) {
    user->filepos = 0;
  } else {
    user->misc_op = 2;
  }
}


/*
 * Delete some or all of your mail. A problem here is once we have deleted
 * some mail from the file do we mark the file as read? If not we could
 * have a situation where the user deletes all his mail but still gets
 * the YOU HAVE UNREAD MAIL message on logging on if the idiot forgot to
 * read it first.
 */
void
dmail(UR_OBJECT user)
{
  int num, cnt;
  char filename[80];

  if (word_count < 2) {
    write_user(user, "Usage: dmail all\n");
    write_user(user, "Usage: dmail <#>\n");
    write_user(user, "Usage: dmail to <#>\n");
    write_user(user, "Usage: dmail from <#> to <#>\n");
    return;
  }
  if (get_wipe_parameters(user) == -1) {
    return;
  }
  num = mail_sizes(user->name, 0);
  if (!num) {
    write_user(user, "You have no mail to delete.\n");
    return;
  }
  sprintf(filename, "%s/%s/%s.M", USERFILES, USERMAILS, user->name);
  if (user->wipe_from == -1) {
    write_user(user, "\07~OL~FR~LIDelete all of your mail?~RS (y/n): ");
    user->misc_op = 18;
    return;
  }
  if (user->wipe_from > num) {
    vwrite_user(user, "You only have %d mail message%s.\n", num,
                PLTEXT_S(num));
    return;
  }
  cnt = wipe_messages(filename, user->wipe_from, user->wipe_to, 1);
  reset_mail_counts(user);
  if (cnt == num) {
    remove(filename);
    vwrite_user(user, "There %s only %d mail message%s, all now deleted.\n",
                PLTEXT_WAS(cnt), cnt, PLTEXT_S(cnt));
    return;
  }
  vwrite_user(user, "%d mail message%s deleted.\n", cnt, PLTEXT_S(cnt));
  user->read_mail = time(0) + 1;
}


/*
 * Show list of people your mail is from without seeing the whole lot
 */
void
mail_from(UR_OBJECT user)
{
  char w1[ARR_SIZE], line[ARR_SIZE], filename[80], *s;
  FILE *fp;
  int valid, cnt, tmp1, tmp2, nmail;

  sprintf(filename, "%s/%s/%s.M", USERFILES, USERMAILS, user->name);
  fp = fopen(filename, "r");
  if (!fp) {
    write_user(user, "You have no mail.\n");
    return;
  }
  write_user(user, "\n~BB*** Mail from ***\n\n");
  valid = 1;
  cnt = 0;
  fscanf(fp, "%d %d\r", &tmp1, &tmp2);
  for (s = fgets(line, ARR_SIZE - 1, fp); s;
       s = fgets(line, ARR_SIZE - 1, fp)) {
    if (*s == '\n') {
      valid = 1;
    }
    sscanf(s, "%s", w1);
    if (valid && !strcmp(colour_com_strip(w1), "From:")) {
      vwrite_user(user, "~FC%2d)~RS %s", ++cnt, remove_first(s));
      valid = 0;
    }
  }
  fclose(fp);
  nmail = mail_sizes(user->name, 1);
  vwrite_user(user,
              "\nTotal of ~OL%d~RS message%s, ~OL%d~RS of which %s new.\n\n",
              cnt, PLTEXT_S(cnt), nmail, PLTEXT_IS(nmail));
}


/*
 * get users which to send copies of smail to
 */
void
copies_to(UR_OBJECT user)
{
  int i, found, x, y;
  char *remote;

  if (com_num == NOCOPIES) {
    for (i = 0; i < MAX_COPIES; ++i) {
      *user->copyto[i] = '\0';
    }
    write_user(user, "Sending no copies of your next smail.\n");
    return;
  }
  if (word_count < 2) {
    *text = '\0';
    found = 0;
    for (i = 0; i < MAX_COPIES; ++i) {
      if (!*user->copyto[i]) {
        continue;
      }
      if (!found++) {
        write_user(user, "Sending copies of your next smail to...\n");
      }
      strcat(text, "   ");
      strcat(text, user->copyto[i]);
    }
    strcat(text, "\n\n");
    if (!found) {
      write_user(user, "You are not sending a copy to anyone.\n");
    } else {
      write_user(user, text);
    }
    return;
  }
  if (word_count > MAX_COPIES + 1) {    /* +1 because 1 count for command */
    vwrite_user(user, "You can only copy to a maximum of %d people.\n",
                MAX_COPIES);
    return;
  }
  /* check to see the user is not trying to send duplicates */
  for (x = 1; x < word_count; ++x) {
    for (y = x + 1; y < word_count; ++y) {
      if (!strcasecmp(word[x], word[y])) {
        write_user(user, "Cannot send to same person more than once.\n");
        return;
      }
    }
  }
  write_user(user, "\n");
  for (i = 0; i < MAX_COPIES; ++i) {
    *user->copyto[i] = '\0';
  }
  i = 0;
  for (x = 1; x < word_count; ++x) {
    /* See if its to another site */
    if (*word[x] == '@') {
      vwrite_user(user,
                  "Name missing before @ sign for copy to name \"%s\".\n",
                  word[x]);
      continue;
    }
    remote = strchr(word[x], '@');
    if (!remote) {
      *word[x] = toupper(*word[x]);
      /* See if user exists */
      if (get_user(word[x]) == user && user->level < ARCH) {
        write_user(user, "You cannot send yourself a copy.\n");
        continue;
      }
      if (!find_user_listed(word[x])) {
        vwrite_user(user,
                    "There is no such user with the name \"%s\" to copy to.\n",
                    word[x]);
        continue;
      }
    }
    strcpy(user->copyto[i++], word[x]);
  }
  *text = '\0';
  found = 0;
  for (i = 0; i < MAX_COPIES; ++i) {
    if (!*user->copyto[i]) {
      continue;
    }
    if (!found++) {
      write_user(user, "Sending copies of your next smail to...\n");
    }
    strcat(text, "   ");
    strcat(text, user->copyto[i]);
  }
  strcat(text, "\n\n");
  if (!found) {
    write_user(user, "You are not sending a copy to anyone.\n");
  } else {
    write_user(user, text);
  }
}


/*
 * send out copy of smail to anyone that is in user->copyto
 */
void
send_copies(UR_OBJECT user, char *ptr)
{
  int i, found = 0;

  for (i = 0; i < MAX_COPIES; ++i) {
    if (!*user->copyto[i]) {
      continue;
    }
    if (!found++) {
      write_user(user, "Attempting to send copies of smail...\n");
    }
    if (send_mail(user, user->copyto[i], ptr, 1)) {
      write_syslog(SYSLOG, 1, "%s sent a copy of mail to %s.\n", user->name,
                   user->copyto[i]);
    }
    *user->copyto[i] = '\0';
  }
}


/*
 * Send mail message to everyone
 */
void
level_mail(UR_OBJECT user, char *inpstr)
{
  if (inpstr) {
    static const char usage[] = "Usage: lmail <level>|wizzes|all [<text>]\n";

    /* FIXME: Use sentinel other JAILED */
    if (user->muzzled != JAILED) {
      write_user(user, "You are muzzled, you cannot mail anyone.\n");
      return;
    }
    if (word_count < 2) {
      write_user(user, usage);
      return;
    }
    strtoupper(word[1]);
    user->lmail_lev = get_level(word[1]);
    if (user->lmail_lev == NUM_LEVELS) {
      if (!strcmp(word[1], "WIZZES")) {
        user->lmail_lev = WIZ;
      } else if (!strcmp(word[1], "ALL")) {
        user->lmail_lev = JAILED;
      } else {
        write_user(user, usage);
        return;
      }
      user->lmail_all = !0;
    } else {
      user->lmail_all = 0;
    }
    if (word_count < 3) {
#ifdef NETLINKS
      if (user->type == REMOTE_TYPE) {
        write_user(user,
                   "Sorry, due to software limitations remote users cannot use the line editor.\nUse the \".lmail <level>|wizzes|all <text>\" method instead.\n");
        return;
      }
#endif
      if (!user->lmail_all) {
        vwrite_user(user,
                    "\n~FG*** Writing broadcast level mail message to all the %ss ***\n\n",
                    user_level[user->lmail_lev].name);
      } else {
        if (user->lmail_lev == WIZ) {
          write_user(user,
                     "\n~FG*** Writing broadcast level mail message to all the Wizzes ***\n\n");
        } else {
          write_user(user,
                     "\n~FG*** Writing broadcast level mail message to everyone ***\n\n");
        }
      }
      user->misc_op = 9;
      editor(user, NULL);
      return;
    }
    strcat(inpstr, "\n");       /* XXX: risky but hopefully it will be ok */
    inpstr = remove_first(inpstr);
  } else {
    inpstr = user->malloc_start;
  }
  if (!send_broadcast_mail(user, inpstr, user->lmail_lev, user->lmail_all)) {
    write_user(user, "There does not seem to be anyone to send mail to.\n");
    user->lmail_all = 0;
    user->lmail_lev = NUM_LEVELS;
    return;
  }
  if (!user->lmail_all) {
    vwrite_user(user, "You have sent mail to all the %ss.\n",
                user_level[user->lmail_lev].name);
    write_syslog(SYSLOG, 1, "%s sent mail to all the %ss.\n", user->name,
                 user_level[user->lmail_lev].name);
  } else {
    if (user->lmail_lev == WIZ) {
      write_user(user, "You have sent mail to all the Wizzes.\n");
      write_syslog(SYSLOG, 1, "%s sent mail to all the Wizzes.\n",
                   user->name);
    } else {
      write_user(user, "You have sent mail to all the users.\n");
      write_syslog(SYSLOG, 1, "%s sent mail to all the users.\n", user->name);
    }
  }
  user->lmail_all = 0;
  user->lmail_lev = NUM_LEVELS;
}


/*
 * This is function that sends mail to other users
 */
int
send_broadcast_mail(UR_OBJECT user, char *ptr, enum lvl_value lvl, int all)
{
  char header[ARR_SIZE], filename[80];
  struct stat stbuf;
  const char *cc;
  FILE *infp, *outfp;
  UD_OBJECT entry;
  int tmp1, tmp2, amount, size, cnt;
  int c;

  cnt = 0;
  for (entry = first_user_entry; entry; entry = entry->next) {
    if (entry->level < lvl) {
      continue;
    }
    if (!all && entry->level != lvl) {
      continue;
    }
    outfp = fopen("tempfile", "w");
    if (!outfp) {
      write_user(user, "Error in mail delivery.\n");
      write_syslog(SYSLOG | ERRLOG, 0,
                   "ERROR: Cannot open tempfile in send_broadcast_mail().\n");
      continue;
    }
    /* Write current time on first line of tempfile */
    sprintf(filename, "%s/%s/%s.M", USERFILES, USERMAILS, entry->name);
    /* first get old file size if any new mail, and also new mail count */
    amount = mail_sizes(entry->name, 1);
    if (!amount) {
      size = stat(filename, &stbuf) == -1 ? 0 : stbuf.st_size;
    } else {
      size = mail_sizes(entry->name, 2);
    }
    fprintf(outfp, "%d %d\r", ++amount, size);
    infp = fopen(filename, "r");
    if (infp) {
      /* Discard first line of mail file. */
      fscanf(infp, "%d %d\r", &tmp1, &tmp2);
      /* Copy rest of file */
      for (c = getc(infp); c != EOF; c = getc(infp)) {
        putc(c, outfp);
      }
      fclose(infp);
    }
    if (!all) {
      sprintf(text, "(BCLM %s lvl)", user_level[lvl].name);
      cc = text;
    } else {
      cc = lvl == WIZ ? "(BCLM Wizzes)" : "(BCLM All users)";
    }
    *header = '\0';
    if (!user) {
      sprintf(header, "~OLFrom: MAILER  %s %s\n", long_date(0), cc);
    } else {
#ifdef NETLINKS
      if (user->type == REMOTE_TYPE) {
        sprintf(header, "~OLFrom: %s@%s  %s %s\n", user->name,
                user->netlink->service, long_date(0), cc);
      } else
#endif
      {
        sprintf(header, "~OLFrom: %s  %s %s\n", user->bw_recap, long_date(0),
                cc);
      }
    }
    fprintf(outfp, "%s", header);
    fputs(ptr, outfp);
    fputs("\n", outfp);
    fclose(outfp);
    rename("tempfile", filename);
    forward_email(entry->name, header, ptr);
    write_user(get_user(entry->name),
               "\07~FC~OL~LI*** YOU HAVE NEW MAIL ***\n");
    ++cnt;
  }
  return cnt;
}


/*
 * type=0 => return total message count
 * type=1 => return new messages count
 * type=2 => return new messages file position
 */
int
mail_sizes(char *name, int type)
{
  char w1[ARR_SIZE], line[ARR_SIZE], filename[80], *s;
  FILE *fp;
  int valid, cnt, newcnt, size;

  cnt = newcnt = size = 0;
  *name = toupper(*name);
  sprintf(filename, "%s/%s/%s.M", USERFILES, USERMAILS, name);
  fp = fopen(filename, "r");
  if (!fp) {
    return cnt;
  }
  valid = 1;
  fscanf(fp, "%d %d\r", &newcnt, &size);
  /* return amount of new mail or size of mail file */
  if (type) {
    fclose(fp);
    return type == 1 ? newcnt : size;
  }
  /* count up total mail */
  for (s = fgets(line, ARR_SIZE - 1, fp); s;
       s = fgets(line, ARR_SIZE - 1, fp)) {
    if (*s == '\n') {
      valid = 1;
    }
    sscanf(s, "%s", w1);
    if (valid && !strcmp(colour_com_strip(w1), "From:")) {
      ++cnt;
      valid = 0;
    }
  }
  fclose(fp);
  return cnt;
}


/*
 * Reset the new mail count and file size at the top of a user mail file
 */
int
reset_mail_counts(UR_OBJECT user)
{
  char filename[80];
  struct stat stbuf;
  FILE *infp, *outfp;
  int c, size, tmp1, tmp2;

  sprintf(filename, "%s/%s/%s.M", USERFILES, USERMAILS, user->name);
  /* get file size */
  size = stat(filename, &stbuf) == -1 ? 0 : stbuf.st_size;
  infp = fopen(filename, "r");
  if (!infp) {
    return 0;
  }
  /* Update last read / new mail received time at head of file */
  outfp = fopen("tempfile", "w");
  if (outfp) {
    fprintf(outfp, "0 %d\r", size);
    /* skip first line of mail file */
    fscanf(infp, "%d %d\r", &tmp1, &tmp2);
    /* Copy rest of file */
    for (c = getc(infp); c != EOF; c = getc(infp)) {
      putc(c, outfp);
    }
    fclose(outfp);
    rename("tempfile", filename);
  }
  user->read_mail = time(0);
  fclose(infp);
  return 1;
}


/*
 * verify user email when it is set specified
 */
void
set_forward_email(UR_OBJECT user)
{
  char filename[80], subject[80];
  FILE *fp;

  if (!*user->email) {
    write_user(user,
               "Your email address is currently ~FRunset~RS.  If you wish to use the\nauto-forwarding function then you must set your email address.\n\n");
    user->autofwd = 0;
    return;
  }
  if (!amsys->forwarding) {
    write_user(user,
               "Even though you have set your email, the auto-forwarding function is currently unavaliable.\n");
    user->mail_verified = 0;
    user->autofwd = 0;
    return;
  }
  user->mail_verified = 0;
  user->autofwd = 0;
  /* Let them know by email */
  sprintf(filename, "%s/%s.FWD", MAILSPOOL, user->name);
  fp = fopen(filename, "w");
  if (!fp) {
    write_syslog(SYSLOG, 0,
                 "Unable to open forward mail file in set_forward_email()\n");
    return;
  }
  sprintf(user->verify_code, "amnuts%d", rand() % 999);
  /* email header */
  fprintf(fp, "From: %s\n", TALKER_NAME);
  fprintf(fp, "To: %s <%s>\n\n", user->name, user->email);
  /* email body */
  fprintf(fp, "Hello, %s.\n\n", user->name);
  fprintf(fp,
          "Thank you for setting your email address, and now that you have done so you are\n");
  fprintf(fp,
          "able to use the auto-forwarding function on The Talker to have any smail sent to\n");
  fprintf(fp,
          "your email address.  To be able to do this though you must verify that you have\n");
  fprintf(fp, "received this email.\n\n");
  fprintf(fp, "Your verification code is: %s\n\n", user->verify_code);
  fprintf(fp,
          "Use this code with the \"verify\" command when you next log onto the talker.\n");
  fprintf(fp,
          "You will then have to use the \"set\" command to turn on/off auto-forwarding.\n\n");
  fprintf(fp,
          "Thank you for coming to our talker - we hope you enjoy it!\n\n   The Staff.\n\n");
  fputs(talker_signature, fp);
  fclose(fp);
  /* send the mail */
  sprintf(subject, "Verification of auto-mail (%s).\n", user->name);
  send_forward_email(user->email, filename, subject);
  write_syslog(SYSLOG, 1,
               "%s had mail sent to them by set_forward_email().\n",
               user->name);
  /* Inform them online */
  write_user(user,
             "Now that you have set your email you can use the auto-forward functions.\n");
  write_user(user,
             "You must verify your address with the code you will receive shortly, via email.\n");
  write_user(user,
             "If you do not receive any email, then use ~FCset email <email>~RS again, making\nsure you have the correct address.\n\n");
}


/*
 * verify that mail has been sent to the address supplied
 */
void
verify_email(UR_OBJECT user)
{
  if (word_count < 2) {
    write_user(user, "Usage: verify <verification code>\n");
    return;
  }
  if (!*user->email) {
    write_user(user,
               "You have not yet set your email address.  You must do this first.\n");
    return;
  }
  if (user->mail_verified) {
    write_user(user,
               "You have already verified your current email address.\n");
    return;
  }
  if (strcmp(user->verify_code, word[1]) || !*user->verify_code) {
    write_user(user,
               "That does not match your verification code.  Check your code and try again.\n");
    return;
  }
  *user->verify_code = '\0';
  user->mail_verified = 1;
  write_user(user,
             "\nThe verification code you gave was correct.\nYou may now use the auto-forward functions.\n\n");
}


/*
 * send smail to the email ccount
 */
void
forward_email(char *name, char *from, char *message)
{
  char filename[80], subject[80];
  FILE *fp;
  UR_OBJECT u;
  int on;

  if (!amsys->forwarding) {
    return;
  }
  u = get_user(name);
  on = !!u;
  if (!on) {
    /* Have to create temp user if not logged on to check if email verified, etc */
    u = create_user();
    if (!u) {
      write_syslog(SYSLOG | ERRLOG, 0,
                   "ERROR: Unable to create temporary user object in forward_email().\n");
      return;
    }
    strcpy(u->name, name);
    if (!load_user_details(u)) {
      destruct_user(u);
      destructed = 0;
      return;
    }
  }
  if (!u->mail_verified) {
    if (!on) {
      destruct_user(u);
      destructed = 0;
    }
    return;
  }
  if (!u->autofwd) {
    if (!on) {
      destruct_user(u);
      destructed = 0;
    }
    return;
  }
  sprintf(filename, "%s/%s.FWD", MAILSPOOL, u->name);
  fp = fopen(filename, "w");
  if (!fp) {
    write_syslog(SYSLOG, 0,
                 "Unable to open forward mail file in set_forward_email()\n");
    return;
  }
  fprintf(fp, "From: %s\n", TALKER_NAME);
  fprintf(fp, "To: %s <%s>\n\n", u->name, u->email);
  from = colour_com_strip(from);
  fputs(from, fp);
  fputs("\n", fp);
  message = colour_com_strip(message);
  fputs(message, fp);
  fputs("\n\n", fp);
  fputs(talker_signature, fp);
  fclose(fp);
  sprintf(subject, "Auto-forward of smail (%s).\n", u->name);
  send_forward_email(u->email, filename, subject);
  write_syslog(SYSLOG, 1, "%s had mail sent to their email address.\n",
               u->name);
  if (!on) {
    destruct_user(u);
    destructed = 0;
  }
  return;
}


/*
 * stop zombie processes
 */
int
send_forward_email(char *send_to, char *mail_file, char *subject)
{
#ifdef DOUBLEFORK
  switch (double_fork()) {
  case -1:
    remove(mail_file);
    return -1;                  /* double_fork() failed */
  case 0:
    sprintf(text, "mail '%s' -s '%s' < %s", send_to, subject, mail_file);
    system(text);
    remove(mail_file);
    _exit(0);
    break;                      /* should never get here */
  default:
    break;
  }
#else
  sprintf(text, "mail '%s' -s '%s' < %s &", send_to, subject, mail_file);
  system(text);
  remove(mail_file);
#endif
  return 1;
}


#ifdef DOUBLEFORK
/*
 * signal trapping not working, so fork twice
 */
int
double_fork(void)
{
  pid_t pid;
  int dfstatus;

  pid = fork();
  if (!pid) {
    switch (fork()) {
    case 0:
      return 0;
    case -1:
      _exit(127);
    default:
      _exit(0);
    }
  }
  if (pid < 0 || waitpid(pid, &dfstatus, 0) < 0) {
    return -1;
  }
  if (WIFEXITED(dfstatus)) {
    if (!WEXITSTATUS(dfstatus)) {
      return 1;
    } else {
      errno = WEXITSTATUS(dfstatus);
    }
  } else {
    errno = EINTR;
  }
  return -1;
}
#endif


/*
 * allows a user to email specific messages to themselves
 */
void
forward_specific_mail(UR_OBJECT user)
{
  char w1[ARR_SIZE], line[ARR_SIZE], filenamei[80], filenameo[80],
    subject[80], *s;
  FILE *fpi, *fpo;
  int valid, cnt, total, smail_number, tmp1, tmp2;

  if (word_count < 2) {
    write_user(user, "Usage: fmail all|<mail number>\n");
    return;
  }
  total = mail_sizes(user->name, 0);
  if (!total) {
    write_user(user, "You currently have no mail.\n");
    return;
  }
  if (!user->mail_verified) {
    write_user(user, "You have not yet verified your email address.\n");
    return;
  }
  sprintf(subject, "Manual forwarding of smail (%s)", user->name);
  /* send all smail */
  if (!strcasecmp(word[1], "all")) {
    sprintf(filenameo, "%s/%s.FWD", MAILSPOOL, user->name);
    fpo = fopen(filenameo, "w");
    if (!fpo) {
      write_syslog(SYSLOG, 0,
                   "Unable to open forward mail file in forward_specific_mail()\n");
      write_user(user, "Sorry, could not forward any mail to you.\n");
      return;
    }
    sprintf(filenamei, "%s/%s/%s.M", USERFILES, USERMAILS, user->name);
    fpi = fopen(filenamei, "r");
    if (!fpi) {
      write_user(user, "Sorry, could not forward any mail to you.\n");
      write_syslog(SYSLOG, 0,
                   "Unable to open %s's mailbox in forward_specific_mail()\n",
                   user->name);
      fclose(fpo);
      return;
    }
    fprintf(fpo, "From: %s\n", TALKER_NAME);
    fprintf(fpo, "To: %s <%s>\n\n", user->name, user->email);
    fscanf(fpi, "%d %d\r", &tmp1, &tmp2);
    for (s = fgets(line, ARR_SIZE - 1, fpi); s;
         s = fgets(line, ARR_SIZE - 1, fpi)) {
      fprintf(fpo, "%s", colour_com_strip(s));
    }
    fputs(talker_signature, fpo);
    fclose(fpi);
    fclose(fpo);
    send_forward_email(user->email, filenameo, subject);
    write_user(user,
               "You have now sent ~OL~FRall~RS your smails to your email account.\n");
    return;
  }
  /* send just a specific smail */
  smail_number = atoi(word[1]);
  if (!smail_number) {
    write_user(user, "Usage: fmail all/<mail number>\n");
    return;
  }
  if (smail_number > total) {
    vwrite_user(user, "You only have %d message%s in your mailbox.\n", total,
                PLTEXT_S(total));
    return;
  }
  sprintf(filenameo, "%s/%s.FWD", MAILSPOOL, user->name);
  fpo = fopen(filenameo, "w");
  if (!fpo) {
    write_syslog(SYSLOG, 0,
                 "Unable to open forward mail file in forward_specific_mail()\n");
    write_user(user, "Sorry, could not forward any mail to you.\n");
    return;
  }
  sprintf(filenamei, "%s/%s/%s.M", USERFILES, USERMAILS, user->name);
  fpi = fopen(filenamei, "r");
  if (!fpi) {
    write_user(user, "Sorry, could not forward any mail to you.\n");
    write_syslog(SYSLOG, 0,
                 "Unable to open %s's mailbox in forward_specific_mail()\n",
                 user->name);
    fclose(fpo);
    return;
  }
  fprintf(fpo, "From: %s\n", TALKER_NAME);
  fprintf(fpo, "To: %s <%s>\n\n", user->name, user->email);
  valid = cnt = 1;
  fscanf(fpi, "%d %d\r", &tmp1, &tmp2);
  for (s = fgets(line, ARR_SIZE - 1, fpi); s;
       s = fgets(line, ARR_SIZE - 1, fpi)) {
    if (*s == '\n') {
      valid = 1;
    }
    sscanf(s, "%s", w1);
    if (valid && !strcmp(colour_com_strip(w1), "From:")) {
      valid = 0;
      if (smail_number == cnt++) {
        break;
      }
    }
  }
  for (; s; s = fgets(line, ARR_SIZE - 1, fpi)) {
    if (*s == '\n') {
      break;
    }
    fprintf(fpo, "%s", colour_com_strip(s));
  }
  fputs(talker_signature, fpo);
  fclose(fpi);
  fclose(fpo);
  send_forward_email(user->email, filenameo, subject);
  vwrite_user(user,
              "You have now sent smail number ~FM~OL%d~RS to your email account.\n",
              smail_number);
}


/***** Message boards *****/


/*
 * Read the message board
 */
void
read_board(UR_OBJECT user)
{
  char filename[80];
  const char *name;
  RM_OBJECT rm;
  int ret;

  rm = NULL;
  if (word_count < 2) {
    rm = user->room;
  } else {
    if (word_count >= 3) {
      rm = get_room(word[1]);
      if (!rm) {
        write_user(user, nosuchroom);
        return;
      }
      ret = atoi(word[2]);
      read_board_specific(user, rm, ret);
      return;
    }
    ret = atoi(word[1]);
    if (ret) {
      read_board_specific(user, user->room, ret);
      return;
    }
    rm = get_room(word[1]);
    if (!rm) {
      write_user(user, nosuchroom);
      return;
    }
    if (!has_room_access(user, rm)) {
      write_user(user,
                 "That room is currently private, you cannot read the board.\n");
      return;
    }
  }
  vwrite_user(user, "\n~BB*** The %s message board ***\n\n", rm->name);
  if (is_personal_room(rm)) {
    sprintf(filename, "%s/%s/%s.B", USERFILES, USERROOMS, rm->owner);
  } else {
    sprintf(filename, "%s/%s.B", DATAFILES, rm->name);
  }
  ret = more(user, user->socket, filename);
  if (!ret) {
    write_user(user, "There are no messages on the board.\n\n");
  } else if (ret == 1) {
    user->misc_op = 2;
  }
  name = user->vis ? user->recap : invisname;
  if (rm == user->room) {
    vwrite_room_except(user->room, user, "%s~RS reads the message board.\n",
                       name);
  }
}


/*
 * Allows a user to read a specific message number
 */
void
read_board_specific(UR_OBJECT user, RM_OBJECT rm, int msg_number)
{
  char id[ARR_SIZE], line[ARR_SIZE], filename[80], *s;
  const char *name;
  FILE *fp;
  int valid, cnt, pt;

  if (!rm->mesg_cnt) {
    vwrite_user(user, "There are no messages posted on the %s board.\n",
                rm->name);
    return;
  }
  if (!msg_number) {
    write_user(user, "Usage: read [<room>] [<message #>]\n");
    return;
  }
  if (msg_number > rm->mesg_cnt) {
    vwrite_user(user, "There %s only %d message%s posted on the %s board.\n",
                PLTEXT_IS(rm->mesg_cnt), rm->mesg_cnt, PLTEXT_S(rm->mesg_cnt),
                rm->name);
    return;
  }
  if (rm != user->room && !has_room_access(user, rm)) {
    write_user(user,
               "That room is currently private, you cannot read the board.\n");
    return;
  }
  if (is_personal_room(rm)) {
    sprintf(filename, "%s/%s/%s.B", USERFILES, USERROOMS, rm->owner);
  } else {
    sprintf(filename, "%s/%s.B", DATAFILES, rm->name);
  }
  fp = fopen(filename, "r");
  if (!fp) {
    write_user(user,
               "There was an error trying to read the message board.\n");
    write_syslog(SYSLOG, 0,
                 "Unable to open message board for %s in read_board_specific().\n",
                 rm->name);
    return;
  }
  vwrite_user(user, "\n~BB~FG*** The %s message board ***\n\n", rm->name);
  valid = cnt = 1;
  *id = '\0';
  for (s = fgets(line, ARR_SIZE - 1, fp); s;
       s = fgets(line, ARR_SIZE - 1, fp)) {
    if (*s == '\n') {
      valid = 1;
    }
    sscanf(s, "%s %d", id, &pt);
    if (valid && !strcmp(id, "PT:")) {
      valid = 0;
      if (msg_number == cnt++) {
        break;
      }
    }
  }
  for (; s; s = fgets(line, ARR_SIZE - 1, fp)) {
    if (*s == '\n') {
      break;
    }
    write_user(user, s);
  }
  fclose(fp);
  vwrite_user(user, "\nMessage number ~FM~OL%d~RS out of ~FM~OL%d~RS.\n\n",
              msg_number, rm->mesg_cnt);
  name = user->vis ? user->recap : invisname;
  if (rm == user->room) {
    if (user->level < GOD || user->vis) {
      vwrite_room_except(user->room, user, "%s~RS reads the message board.\n",
                         name);
    }
  }
}


/*
 * Write on the message board
 */
void
write_board(UR_OBJECT user, char *inpstr)
{
  char filename[80];
  const char *name;
  char *c;
  FILE *fp;
  int cnt;

  if (inpstr) {
    /* FIXME: Use sentinel other JAILED */
    if (user->muzzled != JAILED) {
      write_user(user, "You are muzzled, you cannot write on the board.\n");
      return;
    }
    if (word_count < 2) {
#ifdef NETLINKS
      if (user->type == REMOTE_TYPE) {
        /* Editor will not work over netlink because all the prompts will go wrong */
        write_user(user,
                   "Sorry, due to software limitations remote users cannot use the line editor.\nUse the \".write <text>\" method instead.\n");
        return;
      }
#endif
      write_user(user, "\n~BB*** Writing board message ***\n\n");
      user->misc_op = 3;
      editor(user, NULL);
      return;
    }
    strcat(inpstr, "\n");       /* XXX: risky but hopefully it will be ok */
  } else {
    inpstr = user->malloc_start;
  }
  switch (amsys->ban_swearing) {
  case SBMAX:
    if (contains_swearing(inpstr)) {
      write_user(user, noswearing);
      return;
    }
    break;
  case SBMIN:
    if (!is_personal_room(user->room)) {
      inpstr = censor_swear_words(inpstr);
    }
    break;
  case SBOFF:
  default:
    /* do nothing as ban_swearing is off */
    break;
  }
  if (is_personal_room(user->room)) {
    sprintf(filename, "%s/%s/%s.B", USERFILES, USERROOMS, user->room->owner);
  } else {
    sprintf(filename, "%s/%s.B", DATAFILES, user->room->name);
  }
  fp = fopen(filename, "a");
  if (!fp) {
    vwrite_user(user, "%s: cannot write to file.\n", syserror);
    write_syslog(SYSLOG | ERRLOG, 0,
                 "ERROR: Cannot open file %s to append in write_board().\n",
                 filename);
    return;
  }
  name = user->vis ? user->recap : invisname;
  /*
     The posting time (PT) is the time its written in machine readable form, this
     makes it easy for this program to check the age of each message and delete
     as appropriate in check_messages()
   */
#ifdef NETLINKS
  if (user->type == REMOTE_TYPE) {
    sprintf(text, "PT: %d\r~OLFrom: %s@%s  %s\n", (int) (time(0)), name,
            user->netlink->service, long_date(0));
  } else
#endif
  {
    sprintf(text, "PT: %d\r~OLFrom: %s  %s\n", (int) (time(0)), name,
            long_date(0));
  }
  fputs(text, fp);
  cnt = 0;
  for (c = inpstr; *c; ++c) {
    putc(*c, fp);
    if (*c == '\n') {
      cnt = 0;
    } else {
      ++cnt;
    }
    if (cnt == 80) {
      putc('\n', fp);
      cnt = 0;
    }
  }
  putc('\n', fp);
  fclose(fp);
  write_user(user, "You write the message on the board.\n");
  vwrite_room_except(user->room, user, "%s writes a message on the board.\n",
                     name);
  ++user->room->mesg_cnt;
}


/*
 * Wipe some messages off the board
 */
void
wipe_board(UR_OBJECT user)
{
  char filename[80];
  const char *name;
  RM_OBJECT rm;
  int cnt;

  rm = user->room;
  if (word_count < 2 && ((user->level >= WIZ && !is_personal_room(rm))
                         || (is_personal_room(rm)
                             && (is_my_room(user, rm)
                                 || user->level >= GOD)))) {
    write_user(user, "Usage: wipe all\n");
    write_user(user, "Usage: wipe <#>\n");
    write_user(user, "Usage: wipe to <#>\n");
    write_user(user, "Usage: wipe from <#> to <#>\n");
    return;
  } else if (word_count < 2 && ((user->level < WIZ && !is_personal_room(rm))
                                || (is_personal_room(rm)
                                    && !is_my_room(user, rm)
                                    && user->level < GOD))) {
    write_user(user, "Usage: wipe <#>\n");
    return;
  }
  if (is_personal_room(rm)) {
    if (!is_my_room(user, rm) && user->level < GOD && !check_board_wipe(user)) {
      return;
    } else if (get_wipe_parameters(user) == -1) {
      return;
    }
  } else {
    if (user->level < WIZ && !(check_board_wipe(user))) {
      return;
    } else if (get_wipe_parameters(user) == -1) {
      return;
    }
  }
  name = user->vis ? user->recap : invisname;
  if (is_personal_room(rm)) {
    sprintf(filename, "%s/%s/%s.B", USERFILES, USERROOMS, rm->owner);
  } else {
    sprintf(filename, "%s/%s.B", DATAFILES, rm->name);
  }
  if (!rm->mesg_cnt) {
    write_user(user, "There are no messages on the room board.\n");
    return;
  }
  if (user->wipe_from == -1) {
    remove(filename);
    write_user(user, "All messages deleted.\n");
    if (user->level < GOD || user->vis) {
      vwrite_room_except(rm, user, "%s~RS wipes the message board.\n", name);
    }
    write_syslog(SYSLOG, 1,
                 "%s wiped all messages from the board in the %s.\n",
                 user->name, rm->name);
    rm->mesg_cnt = 0;
    return;
  }
  if (user->wipe_from > rm->mesg_cnt) {
    vwrite_user(user, "There %s only %d message%s on the board.\n",
                PLTEXT_IS(rm->mesg_cnt), rm->mesg_cnt,
                PLTEXT_S(rm->mesg_cnt));
    return;
  }
  cnt = wipe_messages(filename, user->wipe_from, user->wipe_to, 0);
  if (cnt == rm->mesg_cnt) {
    remove(filename);
    vwrite_user(user,
                "There %s only %d message%s on the board, all now deleted.\n",
                PLTEXT_WAS(rm->mesg_cnt), rm->mesg_cnt,
                PLTEXT_S(rm->mesg_cnt));
    if (user->level < GOD || user->vis) {
      vwrite_room_except(rm, user, "%s wipes the message board.\n", name);
    }
    write_syslog(SYSLOG, 1,
                 "%s wiped all messages from the board in the %s.\n",
                 user->name, rm->name);
    rm->mesg_cnt = 0;
    return;
  }
  rm->mesg_cnt -= cnt;
  vwrite_user(user, "%d board message%s deleted.\n", cnt, PLTEXT_S(cnt));
  if (user->level < GOD || user->vis) {
    vwrite_room_except(rm, user, "%s wipes some messages from the board.\n",
                       name);
  }
  write_syslog(SYSLOG, 1, "%s wiped %d message%s from the board in the %s.\n",
               user->name, cnt, PLTEXT_S(cnt), rm->name);
}


/*
 * Check if a normal user can remove a message
 */
int
check_board_wipe(UR_OBJECT user)
{
  char w1[ARR_SIZE], w2[ARR_SIZE], line[ARR_SIZE], filename[80], id[ARR_SIZE], *s, *s2;
  FILE *fp;
  int valid, cnt, msg_number, pt;
  RM_OBJECT rm;

  if (word_count < 2) {
    write_user(user, "Usage: wipe <message #>\n");
    return 0;
  }
  rm = user->room;
  if (!rm->mesg_cnt) {
    write_user(user, "There are no messages on this board.\n");
    return 0;
  }
  msg_number = atoi(word[1]);
  if (!msg_number) {
    write_user(user, "Usage: wipe <#>\n");
    return 0;
  }
  if (msg_number > rm->mesg_cnt) {
    vwrite_user(user, "There %s only %d message%s on the board.\n",
                PLTEXT_IS(rm->mesg_cnt), rm->mesg_cnt,
                PLTEXT_S(rm->mesg_cnt));
    return 0;
  }
  if (is_personal_room(rm)) {
    sprintf(filename, "%s/%s/%s.B", USERFILES, USERROOMS, rm->owner);
  } else {
    sprintf(filename, "%s/%s.B", DATAFILES, rm->name);
  }
  fp = fopen(filename, "r");
  if (!fp) {
    write_user(user, "There was an error trying to read message board.\n");
    write_syslog(SYSLOG, 0,
                 "Unable to open message board for %s in check_board_wipe().\n",
                 rm->name);
    return 0;
  }
  *id = '\0';
  *w1 = '\0';
  *w2 = '\0';
  valid = cnt = 1;
  for (s = fgets(line, ARR_SIZE - 1, fp); s;
       s = fgets(line, ARR_SIZE - 1, fp)) {
    if (*s == '\n') {
      valid = 1;
    }
    sscanf(s, "%s %d %s %s", id, &pt, w1, w2);
    if (valid && !strcmp(id, "PT:")) {
      valid = 0;
      if (msg_number == cnt++) {
        break;
      }
    }
  }
  fclose(fp);
  s2 = colour_com_strip(w2);
  /* lower case the name incase of recapping */
  strtolower(s2);
  *s2 = toupper(*s2);
  if (strcmp(s2, user->name)) {
    write_user(user,
               "You did not post that message. Use ~FCbfrom~RS to check the number again.\n");
    return 0;
  }
  user->wipe_from = msg_number;
  user->wipe_to = msg_number;
  return 1;
}


/*
 * Show list of people board posts are from without seeing the whole lot
 */
void
board_from(UR_OBJECT user)
{
  char id[ARR_SIZE], line[ARR_SIZE], filename[80], *s;
  FILE *fp;
  RM_OBJECT rm;
  int cnt;

  if (word_count < 2) {
    rm = user->room;
  } else {
    rm = get_room(word[1]);
    if (!rm) {
      write_user(user, nosuchroom);
      return;
    }
    if (!has_room_access(user, rm)) {
      write_user(user,
                 "That room is currently private, you cannot read the board.\n");
      return;
    }
  }
  if (!rm->mesg_cnt) {
    write_user(user, "That room has no messages on its board.\n");
    return;
  }
  if (is_personal_room(rm)) {
    sprintf(filename, "%s/%s/%s.B", USERFILES, USERROOMS, rm->owner);
  } else {
    sprintf(filename, "%s/%s.B", DATAFILES, rm->name);
  }
  fp = fopen(filename, "r");
  if (!fp) {
    write_user(user, "There was an error trying to read message board.\n");
    write_syslog(SYSLOG, 0,
                 "Unable to open message board for %s in board_from().\n",
                 rm->name);
    return;
  }
  vwrite_user(user, "\n~FG~BB*** Posts on the %s message board from ***\n\n",
              rm->name);
  cnt = 0;
  for (s = fgets(line, ARR_SIZE - 1, fp); s;
       s = fgets(line, ARR_SIZE - 1, fp)) {
    sscanf(s, "%s", id);
    if (!strcmp(id, "PT:")) {
      vwrite_user(user, "~FC%2d)~RS %s", ++cnt,
                  remove_first(remove_first(remove_first(s))));
    }
  }
  fclose(fp);
  vwrite_user(user, "\nTotal of ~OL%d~RS messages.\n\n", rm->mesg_cnt);
}


/*
 * Search all the boards for the words given in the list. Rooms fixed to
 * private will be ignore if the users level is less than gatecrash_level
 */
void
search_boards(UR_OBJECT user)
{
  char filename[80], line[82], buff[(MAX_LINES + 1) * 82], w1[81], *s;
  FILE *fp;
  RM_OBJECT rm;
  int w, cnt, message, yes, room_given;

  if (word_count < 2) {
    write_user(user, "Usage: search <word list>\n");
    return;
  }
  /* Go through rooms */
  cnt = 0;
  for (rm = room_first; rm; rm = rm->next) {
    if (!has_room_access(user, rm)) {
      continue;
    }
    if (is_personal_room(rm)) {
      sprintf(filename, "%s/%s/%s.B", USERFILES, USERROOMS, rm->owner);
    } else {
      sprintf(filename, "%s/%s.B", DATAFILES, rm->name);
    }
    fp = fopen(filename, "r");
    if (!fp) {
      continue;
    }
    /* Go through file */
    *buff = '\0';
    yes = message = room_given = 0;
    for (s = fgets(line, 81, fp); s; s = fgets(line, 81, fp)) {
      if (*s == '\n') {
        if (yes) {
          strcat(buff, "\n");
          write_user(user, buff);
        }
        message = yes = 0;
        *buff = '\0';
      }
      if (!message) {
        *w1 = '\0';
        sscanf(s, "%s", w1);
        if (!strcmp(w1, "PT:")) {
          message = 1;
          strcpy(buff, remove_first(remove_first(s)));
        }
      } else {
        strcat(buff, s);
      }
      for (w = 1; w < word_count; ++w) {
        if (!yes && strstr(s, word[w])) {
          if (!room_given) {
            vwrite_user(user, "~BB*** %s ***\n\n", rm->name);
            room_given = 1;
          }
          yes = 1;
          ++cnt;
        }
      }
    }
    if (yes) {
      strcat(buff, "\n");
      write_user(user, buff);
    }
    fclose(fp);
  }
  if (cnt) {
    vwrite_user(user, "Total of %d matching message%s.\n\n", cnt,
                PLTEXT_S(cnt));
  } else {
    write_user(user, "No occurences found.\n");
  }
}


/*
 * Write a suggestion to the board, or read if if you can
 */
void
suggestions(UR_OBJECT user, char *inpstr)
{
  char filename[30], *c;
  FILE *fp;
  int cnt;

  if (com_num == RSUG) {
    sprintf(filename, "%s/%s", MISCFILES, SUGBOARD);
    write_user(user,
               "~BB~FG*** The Suggestions board has the following ideas ***\n\n");
    switch (more(user, user->socket, filename)) {
    case 0:
      write_user(user, "There are no suggestions.\n\n");
      break;
    case 1:
      user->misc_op = 2;
      break;
    }
    return;
  }
  if (inpstr) {
    /* FIXME: Use sentinel other JAILED */
    if (user->muzzled != JAILED) {
      write_user(user, "You are muzzled, you cannot make suggestions.\n");
      return;
    }
    if (word_count < 2) {
#ifdef NETLINKS
      if (user->type == REMOTE_TYPE) {
        write_user(user,
                   "Sorry, due to software limitations remote users cannot use the line editor.\nUse the \".suggest <text>\" method instead.\n");
        return;
      }
#endif
      write_user(user, "~BB~FG*** Writing a suggestion ***\n\n");
      user->misc_op = 8;
      editor(user, NULL);
      return;
    }
    strcat(inpstr, "\n");       /* XXX: risky but hopefully it will be ok */
  } else {
    inpstr = user->malloc_start;
  }
  sprintf(filename, "%s/%s", MISCFILES, SUGBOARD);
  fp = fopen(filename, "a");
  if (!fp) {
    vwrite_user(user, "%s: cannot add suggestion.\n", syserror);
    write_syslog(SYSLOG | ERRLOG, 0,
                 "ERROR: Cannot open file %s to write in suggestions().\n",
                 filename);
    return;
  }
  sprintf(text, "~OLFrom: %s  %s\n", user->bw_recap, long_date(0));
  fputs(text, fp);
  cnt = 0;
  for (c = inpstr; *c; ++c) {
    putc(*c, fp);
    if (*c == '\n') {
      cnt = 0;
    } else {
      ++cnt;
    }
    if (cnt == 80) {
      putc('\n', fp);
      cnt = 0;
    }
  }
  putc('\n', fp);
  fclose(fp);
  write_user(user, "Suggestion written.  Thank you for your contribution.\n");
  ++amsys->suggestion_count;
}


/*
 * delete suggestions from the board
 */
void
delete_suggestions(UR_OBJECT user)
{
  char filename[80];
  int cnt;

  if (word_count < 2) {
    write_user(user, "Usage: dsug all\n");
    write_user(user, "Usage: dsug <#>\n");
    write_user(user, "Usage: dsug to <#>\n");
    write_user(user, "Usage: dsug from <#> to <#>\n");
    return;
  }
  if (get_wipe_parameters(user) == -1) {
    return;
  }
  if (!amsys->suggestion_count) {
    write_user(user, "There are no suggestions to delete.\n");
    return;
  }
  sprintf(filename, "%s/%s", MISCFILES, SUGBOARD);
  if (user->wipe_from == -1) {
    remove(filename);
    write_user(user, "All suggestions deleted.\n");
    write_syslog(SYSLOG, 1, "%s wiped all suggestions from the %s board\n",
                 user->name, SUGBOARD);
    amsys->suggestion_count = 0;
    return;
  }
  if (user->wipe_from > amsys->suggestion_count) {
    vwrite_user(user, "There %s only %d suggestion%s on the board.\n",
                PLTEXT_IS(amsys->suggestion_count), amsys->suggestion_count,
                PLTEXT_S(amsys->suggestion_count));
    return;
  }
  cnt = wipe_messages(filename, user->wipe_from, user->wipe_to, 0);
  if (cnt == amsys->suggestion_count) {
    remove(filename);
    vwrite_user(user,
                "There %s only %d suggestion%s on the board, all now deleted.\n",
                PLTEXT_WAS(cnt), cnt, PLTEXT_S(cnt));
    write_syslog(SYSLOG, 1, "%s wiped all suggestions from the %s board\n",
                 user->name, SUGBOARD);
    amsys->suggestion_count = 0;
    return;
  }
  amsys->suggestion_count -= cnt;
  vwrite_user(user, "%d suggestion%s deleted.\n", cnt, PLTEXT_S(cnt));
  write_syslog(SYSLOG, 1, "%s wiped %d suggestion%s from the %s board\n",
               user->name, cnt, PLTEXT_S(cnt), SUGBOARD);
}


/*
 * Show list of people suggestions are from without seeing the whole lot
 */
void
suggestions_from(UR_OBJECT user)
{
  char id[ARR_SIZE], line[ARR_SIZE], filename[80], *s, *str;
  FILE *fp;
  int valid;
  int cnt;

  if (!amsys->suggestion_count) {
    write_user(user, "There are currently no suggestions.\n");
    return;
  }
  sprintf(filename, "%s/%s", MISCFILES, SUGBOARD);
  fp = fopen(filename, "r");
  if (!fp) {
    write_user(user,
               "There was an error trying to read the suggestion board.\n");
    write_syslog(SYSLOG, 0,
                 "Unable to open suggestion board in suggestions_from().\n");
    return;
  }
  vwrite_user(user, "\n~BB*** Suggestions on the %s board from ***\n\n",
              SUGBOARD);
  valid = 1;
  cnt = 0;
  for (s = fgets(line, ARR_SIZE - 1, fp); s;
       s = fgets(line, ARR_SIZE - 1, fp)) {
    if (*s == '\n') {
      valid = 1;
    }
    sscanf(s, "%s", id);
    str = colour_com_strip(id);
    if (valid && !strcmp(str, "From:")) {
      vwrite_user(user, "~FC%2d)~RS %s", ++cnt, remove_first(s));
      valid = 0;
    }
  }
  fclose(fp);
  vwrite_user(user, "\nTotal of ~OL%d~RS suggestions.\n\n",
              amsys->suggestion_count);
}


/*
 * delete lines from boards or mail or suggestions, etc
 */
int
get_wipe_parameters(UR_OBJECT user)
{
  /* get delete paramters */
  strtolower(word[1]);
  if (!strcmp(word[1], "all")) {
    user->wipe_from = -1;
    user->wipe_to = -1;
  } else if (!strcmp(word[1], "from")) {
    if (word_count < 4 || strcmp(word[3], "to")) {
      write_user(user, "Usage: <command> from <#> to <#>\n");
      return -1;
    }
    user->wipe_from = atoi(word[2]);
    user->wipe_to = atoi(word[4]);
  } else if (!strcmp(word[1], "to")) {
    if (word_count < 2) {
      write_user(user, "Usage: <command> to <#>\n");
      return -1;
    }
    user->wipe_from = 0;
    user->wipe_to = atoi(word[2]);
  } else {
    user->wipe_from = atoi(word[1]);
    user->wipe_to = atoi(word[1]);
  }
  if (user->wipe_from > user->wipe_to) {
    write_user(user,
               "The first number must be smaller than the second number.\n");
    return -1;
  }
  return 1;
}


/*
 * Removes messages from one of the board types: room boards, smail,
 * suggestions, etc. It works on the premise that every message is
 * seperated by a newline on a line by itself. And as all messages have
 * this form--no probs :) Just do not go screwing with how the messages
 * are stored and you will be ok :P
 *
 * from = message to start deleting at
 * to = message to stop deleting at (both inclusive)
 * type = 1 then board is mail, otherwise any other board
 */
int
wipe_messages(char *filename, int from, int to, int type)
{
  char line[ARR_SIZE], *s;
  FILE *fpi, *fpo;
  int rem, i, tmp1, tmp2;

  fpi = fopen(filename, "r");
  /* return on no messages to delete */
  if (!fpi) {
    return 0;
  }
  fpo = fopen("tempfile", "w");
  if (!fpo) {
    write_syslog(SYSLOG | ERRLOG, 0,
                 "ERROR: Cannot open tempfile in wipe_message().\n");
    fclose(fpi);
    return -1;                  /* return on error */
  }
  /* if type is mail */
  if (type == 1) {
    fscanf(fpi, "%d %d\r", &tmp1, &tmp2);
    fprintf(fpo, "%d %d\r", tmp1, tmp2);
  }
  rem = 0;
  i = 1;
  for (s = fgets(line, ARR_SIZE - 1, fpi); s;
       s = fgets(line, ARR_SIZE - 1, fpi)) {
    if (i < from || i > to) {
      fputs(s, fpo);
    } else {
      if (*s == '\n') {
        ++rem;
      }
    }
    /* message ended */
    if (*s == '\n') {
      ++i;
    }
  }
  fclose(fpi);
  fclose(fpo);
  remove(filename);
  rename("tempfile", filename);
  return rem;
}


/*
 * Send mail message to all people on your friends list
 */
void
friend_smail(UR_OBJECT user, char *inpstr)
{
  FU_OBJECT fu;

  if (inpstr) {
    /* FIXME: Use sentinel other JAILED */
    if (user->muzzled != JAILED) {
      write_user(user, "You are muzzled, you cannot mail anyone.\n");
      return;
    }
    /* check to see if use has friends listed */
    if (!count_friends(user)) {
      write_user(user, "You have no friends listed.\n");
      return;
    }
    if (word_count < 2) {
      /* go to the editor to smail */
#ifdef NETLINKS
      if (user->type == REMOTE_TYPE) {
        write_user(user,
                   "Sorry, due to software limitations remote users cannot use the line editor.\nUse the \".fsmail <text>\" method instead.\n");
        return;
      }
#endif
      write_user(user,
                 "\n~BB*** Writing mail message to all your friends ***\n\n");
      user->misc_op = 24;
      editor(user, NULL);
    }
    /* do smail - no editor */
    strcat(inpstr, "\n");
  } else {
    /* now do smail - out of editor */
    if (*user->malloc_end-- != '\n') {
      *user->malloc_end-- = '\n';
    }
    inpstr = user->malloc_start;
  }
  for (fu = user->fu_first; fu; fu = fu->next) {
    if (fu->flags & fufFRIEND) {
      send_mail(user, fu->name, inpstr, 2);
    }
  }
  write_user(user, "Mail sent to all people on your friends list.\n");
  write_syslog(SYSLOG, 1,
               "%s sent mail to all people on their friends list.\n",
               user->name);
}


/*
 * The editor used for writing profiles, mail and messages on the boards
 */
void
editor(UR_OBJECT user, char *inpstr)
{
  static const char edprompt[] =
    "\n~FG(~OLS~RS~FG)ave~RS, ~FC(~OLV~RS~FC)iew~RS, ~FY(~OLR~RS~FY)edo~RS or ~FR(~OLA~RS~FR)bort~RS: ";
  const char *name;
  char *ptr;
  int cnt, line;

  if (user->edit_op) {
    switch (tolower(*inpstr)) {
    case 's':
#if !!0
      if (*user->malloc_end-- != '\n') {
        *user->malloc_end-- = '\n';
      }
#endif
      name = user->vis ? user->recap : invisname;
      if (!user->vis) {
        write_monitor(user, user->room, 0);
      }
      vwrite_room_except(user->room, user,
                         "%s~RS finishes composing some text.\n", name);
      switch (user->misc_op) {
      case 3:
        write_board(user, NULL);
        break;
      case 4:
        smail(user, NULL);
        break;
      case 5:
        enter_profile(user, NULL);
        break;
      case 8:
        suggestions(user, NULL);
        break;
      case 9:
        level_mail(user, NULL);
        break;
      case 19:
        personal_room_decorate(user, NULL);
        break;
      case 24:
        friend_smail(user, NULL);
        break;
      }
      editor_done(user);
      return;
    case 'r':
      user->edit_op = 0;
      user->edit_line = 1;
      user->charcnt = 0;
      user->malloc_end = user->malloc_start;
      *user->malloc_start = '\0';
      write_user(user, "\nRedo message...\n\n");
      vwrite_user(user,
                  "[---------------- Please try to keep between these two markers ----------------]\n\n~FG%d>~RS",
                  user->edit_line);
      return;
    case 'a':
      write_user(user, "\nMessage aborted.\n");
      name = user->vis ? user->recap : invisname;
      if (!user->vis) {
        write_monitor(user, user->room, 0);
      }
      vwrite_room_except(user->room, user,
                         "%s~RS gives up composing some text.\n", name);
      editor_done(user);
      return;
    case 'v':
      write_user(user, "\nYou have composed the following text...\n\n");
      write_user(user, user->malloc_start);
      write_user(user, edprompt);
      return;
    default:
      write_user(user, edprompt);
      return;
    }
  }
  /* Allocate memory if user has just started editing */
  if (!user->malloc_start) {
    user->malloc_start = (char *) malloc(MAX_LINES * 81);
    if (!user->malloc_start) {
      vwrite_user(user, "%s: failed to allocate buffer memory.\n", syserror);
      write_syslog(SYSLOG | ERRLOG, 0,
                   "ERROR: Failed to allocate memory in editor().\n");
      user->misc_op = 0;
      prompt(user);
      return;
    }
    memset(user->malloc_start, 0, MAX_LINES * 81);
    clear_edit(user);
    user->ignall_store = user->ignall;
    user->ignall = 1;           /* Dont want chat mucking up the edit screen */
    user->edit_line = 1;
    user->charcnt = 0;
    user->malloc_end = user->malloc_start;
    *user->malloc_start = '\0';
    vwrite_user(user,
                "~FCMaximum of %d lines, end with a \".\" on a line by itself.\n\n",
                MAX_LINES);
    write_user(user,
               "[---------------- Please try to keep between these two markers ----------------]\n\n~FG1>~RS");
    name = user->vis ? user->recap : invisname;
    if (!user->vis) {
      write_monitor(user, user->room, 0);
    }
    vwrite_room_except(user->room, user,
                       "%s~RS starts composing some text...\n", name);
    return;
  }
  /* Check for empty line */
  if (!word_count) {
    if (!user->charcnt) {
      vwrite_user(user, "~FG%d>~RS", user->edit_line);
      return;
    }
    *user->malloc_end++ = '\n';
    if (user->edit_line == MAX_LINES) {
      goto END;
    }
    vwrite_user(user, "~FG%d>~RS", ++user->edit_line);
    user->charcnt = 0;
    return;
  }
  /* If nothing carried over and a dot is entered then end */
  if (!user->charcnt && !strcmp(inpstr, ".")) {
    goto END;
  }
  line = user->edit_line;
  cnt = user->charcnt;
  /* loop through input and store in allocated memory */
  while (*inpstr) {
    *user->malloc_end++ = *inpstr++;
    if (++cnt == 80) {
      ++user->edit_line;
      cnt = 0;
    }
    if (user->edit_line > MAX_LINES
        || user->malloc_end - user->malloc_start >= MAX_LINES * 81) {
      goto END;
    }
  }
  if (line != user->edit_line) {
    ptr = user->malloc_end - cnt;
    *user->malloc_end = '\0';
    vwrite_user(user, "~FG%d>~RS%s", user->edit_line, ptr);
    user->charcnt = cnt;
    return;
  } else {
    *user->malloc_end++ = '\n';
    user->charcnt = 0;
  }
  if (user->edit_line != MAX_LINES) {
    vwrite_user(user, "~FG%d>~RS", ++user->edit_line);
    return;
  }
  /* User has finished his message , prompt for what to do now */
END:
  *user->malloc_end = '\0';
  if (*user->malloc_start) {
    write_user(user, edprompt);
    user->edit_op = 1;
    return;
  }
  write_user(user, "\nNo text.\n");
  name = user->vis ? user->recap : invisname;
  if (!user->vis) {
    write_monitor(user, user->room, 0);
  }
  vwrite_room_except(user->room, user,
                     "%s~RS gives up composing some text.\n", name);
  editor_done(user);
}


/*
 * Reset some values at the end of editing
 */
void
editor_done(UR_OBJECT user)
{
  user->misc_op = 0;
  user->edit_op = 0;
  user->edit_line = 0;
  if (user->malloc_start) {
    memset(user->malloc_start, 0, MAX_LINES * 81);
    free(user->malloc_start);
    user->malloc_start = NULL;
    user->malloc_end = NULL;
  }
  user->ignall = user->ignall_store;
  if (has_review(user, rbfEDIT)) {
    write_user(user,
               "\nYou have some tells in your edit review buffer.  Use ~FCrevedit~RS to view them.\n\n");
  }
  prompt(user);
}
