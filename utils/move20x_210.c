/*****************************************************************************
        Directory structure conversion utility for Amnuts version 2.1.0
      Copyright (C) Andrew Collington - Last update : 28th November, 1998
 *****************************************************************************

 This utility DOES NOT ALTER the userfiles themselves.  That is up to either
 the talker server itself (with the load_oldversion_user routine) or up to the
 owner of the talker if they have altered the user structure in any way from
 the original Amnuts structures.

 This utility is used to make it easier for you to sort out the new directory
 structure.  What you need to do is this:  Copy this .c file to the root
 directory of your talker (where the talker .c/.h files are) and compile it
 there using 'gcc move20x_210.c' and then execute it.

 I take no responsibility for this utility - you use at your own risk.  If
 you don't feel safe about using this utility then you should back up your
 userfiles before you start.  Although this is just a cheap and cheerful
 hack of a program I have used it myself and it did work.

 ****************************************************************************/


/* defines */

#include <stdio.h>
#ifdef _AIX
#include <sys/select.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <dirent.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <setjmp.h>

#include "amnuts210.h"
#define AMNUTSVER "2.1.0"

/* main part */

int
main(int argc, char *argv[])
{
  int level, cnt_m, cnt_f, cnt_c, cnt_p, cnt_mac, cnt_d, cnt_h;
  char dirname[80], name[USER_NAME_LEN + 4], new_filename[80],
    old_filename[80], cmd[80];
  DIR *dirp;
  struct dirent *dp;

/* create new directories */
  sprintf(cmd, "mkdir %s/%s", USERFILES, USERMAILS);
  system(cmd);
  sprintf(cmd, "mkdir %s/%s", USERFILES, USERPROFILES);
  system(cmd);
  sprintf(cmd, "mkdir %s/%s", USERFILES, USERFRIENDS);
  system(cmd);
  sprintf(cmd, "mkdir %s/%s", USERFILES, USERHISTORYS);
  system(cmd);
  sprintf(cmd, "mkdir %s/%s", USERFILES, USERCOMMANDS);
  system(cmd);
  sprintf(cmd, "mkdir %s/%s", USERFILES, USERMACROS);
  system(cmd);
  sprintf(cmd, "mkdir %s/%s", USERFILES, USERROOMS);
  system(cmd);

/* reset stuff */
  cnt_m = cnt_f = cnt_c = cnt_p = cnt_mac = cnt_d = cnt_h = 0;

/* move user files */
  for (level = JAILED; level <= GOD; level++) {
    sprintf(dirname, "%s/%s", USERFILES, level_name[level]);
    printf("Processing %s\n", dirname);
    dirp = opendir(dirname);
    if (dirp == NULL) {
      printf("ERROR: Could not open directory -  Exiting...\n");
      exit(0);
    }
    while ((dp = readdir(dirp)) != NULL) {
      if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
        continue;
      if (strstr(dp->d_name, ".D")) {
        sprintf(old_filename, "%s/%s", dirname, dp->d_name);
        sprintf(new_filename, "%s/%s", USERFILES, dp->d_name);
        cnt_d++;
      } else if (strstr(dp->d_name, ".M")) {
        sprintf(old_filename, "%s/%s", dirname, dp->d_name);
        sprintf(new_filename, "%s/%s/%s", USERFILES, USERMAILS, dp->d_name);
        cnt_m++;
      } else if (strstr(dp->d_name, ".F")) {
        sprintf(old_filename, "%s/%s", dirname, dp->d_name);
        sprintf(new_filename, "%s/%s/%s", USERFILES, USERFRIENDS, dp->d_name);
        cnt_f++;
      } else if (strstr(dp->d_name, ".C")) {
        sprintf(old_filename, "%s/%s", dirname, dp->d_name);
        sprintf(new_filename, "%s/%s/%s", USERFILES, USERCOMMANDS,
                dp->d_name);
        cnt_c++;
      } else if (strstr(dp->d_name, ".H")) {
        sprintf(old_filename, "%s/%s", dirname, dp->d_name);
        sprintf(new_filename, "%s/%s/%s", USERFILES, USERHISTORYS,
                dp->d_name);
        cnt_h++;
      } else if (strstr(dp->d_name, ".P")) {
        sprintf(old_filename, "%s/%s", dirname, dp->d_name);
        sprintf(new_filename, "%s/%s/%s", USERFILES, USERPROFILES,
                dp->d_name);
        cnt_p++;
      } else if (strstr(dp->d_name, ".MAC")) {
        sprintf(old_filename, "%s/%s", dirname, dp->d_name);
        sprintf(new_filename, "%s/%s/%s", USERFILES, USERCOMMANDS,
                dp->d_name);
        cnt_mac++;
      }
      rename(old_filename, new_filename);
    }
    (void) closedir(dirp);
    sprintf(cmd, "rmdir %s", dirname);
    system(cmd);
  }

/* print out results */
  printf("User files moved    (.D) : %d\n", cnt_d);
  printf("Mail files moved    (.M) : %d\n", cnt_m);
  printf("Profile files moved (.P) : %d\n", cnt_p);
  printf("Friend files moved  (.F) : %d\n", cnt_f);
  printf("History files moved (.H) : %d\n", cnt_h);
  printf("Command files moved (.C) : %d\n", cnt_c);
  printf("Macro files moved (.MAC) : %d\n", cnt_mac);
}
