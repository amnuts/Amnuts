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
    static char *args[] = {progname, confile, NULL};
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
        break; /* should not need, but good to have */
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
    char bsite[80]; /* XXX: Use NI_MAXHOST */
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
    char bsite[80]; /* XXX: Use NI_MAXHOST */
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
    char ubsite[80]; /* XXX: Use NI_MAXHOST */
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
    char ubsite[80]; /* XXX: Use NI_MAXHOST */
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
            rem_user_node(u->name); /* get rid of name from userlist */
            clean_files(u->name); /* just incase there are any odd files around */
            clean_retire_list(u->name); /* just incase the user is retired */
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
