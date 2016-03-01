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
