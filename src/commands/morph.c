#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"
#ifndef __SDS_H
#include "../vendors/sds/sds.h"
#endif

/*
 * Change a user name from their existing one to whatever
 */
void
change_user_name(UR_OBJECT user)
{
    char oldname[ARR_SIZE], newname[ARR_SIZE];
    sds oldfile, newfile;
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
    /* all memory occurrences should be done.  now do files */
    oldfile = sdscatfmt(sdsempty(), "%s/%s.D", USERFILES, oldname);
    newfile = sdscatfmt(sdsempty(), "%s/%s.D", USERFILES, newname);
    rename(oldfile, newfile);
    oldfile = sdscatfmt(sdsempty(), "%s/%s/%s.M", USERFILES, USERMAILS, oldname);
    newfile = sdscatfmt(sdsempty(), "%s/%s/%s.M", USERFILES, USERMAILS, newname);
    rename(oldfile, newfile);
    oldfile = sdscatfmt(sdsempty(), "%s/%s/%s.P", USERFILES, USERPROFILES, oldname);
    newfile = sdscatfmt(sdsempty(), "%s/%s/%s.P", USERFILES, USERPROFILES, newname);
    rename(oldfile, newfile);
    oldfile = sdscatfmt(sdsempty(), "%s/%s/%s.H", USERFILES, USERHISTORYS, oldname);
    newfile = sdscatfmt(sdsempty(), "%s/%s/%s.H", USERFILES, USERHISTORYS, newname);
    rename(oldfile, newfile);
    oldfile = sdscatfmt(sdsempty(), "%s/%s/%s.C", USERFILES, USERCOMMANDS, oldname);
    newfile = sdscatfmt(sdsempty(), "%s/%s/%s.C", USERFILES, USERCOMMANDS, newname);
    rename(oldfile, newfile);
    oldfile = sdscatfmt(sdsempty(), "%s/%s/%s.MAC", USERFILES, USERMACROS, oldname);
    newfile = sdscatfmt(sdsempty(), "%s/%s/%s.MAC", USERFILES, USERMACROS, newname);
    rename(oldfile, newfile);
    oldfile = sdscatfmt(sdsempty(), "%s/%s/%s.R", USERFILES, USERROOMS, oldname);
    newfile = sdscatfmt(sdsempty(), "%s/%s/%s.R", USERFILES, USERROOMS, newname);
    rename(oldfile, newfile);
    oldfile = sdscatfmt(sdsempty(), "%s/%s/%s.B", USERFILES, USERROOMS, oldname);
    newfile = sdscatfmt(sdsempty(), "%s/%s/%s.B", USERFILES, USERROOMS, newname);
    rename(oldfile, newfile);
    oldfile = sdscatfmt(sdsempty(), "%s/%s/%s.K", USERFILES, USERROOMS, oldname);
    newfile = sdscatfmt(sdsempty(), "%s/%s/%s.K", USERFILES, USERROOMS, newname);
    rename(oldfile, newfile);
    oldfile = sdscatfmt(sdsempty(), "%s/%s/%s.REM", USERFILES, USERREMINDERS, oldname);
    newfile = sdscatfmt(sdsempty(), "%s/%s/%s.REM", USERFILES, USERREMINDERS, newname);
    rename(oldfile, newfile);
    oldfile = sdscatfmt(sdsempty(), "%s/%s/%s.U", USERFILES, USERFLAGGED, oldname);
    newfile = sdscatfmt(sdsempty(), "%s/%s/%s.U", USERFILES, USERFLAGGED, newname);
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
    sdsfree(oldfile);
    sdsfree(newfile);
}
