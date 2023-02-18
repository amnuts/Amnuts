/****************************************************************************
             Amnuts - Copyright (C) Andrew Collington, 1996-2023
                        Last update: Sometime in 2023

                   talker@amnuts.net - https://amnuts.net/

                                 based on

   NUTS version 3.3.3 (Triple Three :) - Copyright (C) Neil Robertson 1996
 ***************************************************************************/

#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/***************************************************************************/

/*
 * Construct system object and reset some global variables
 */
void
create_system(void)
{
    int i;

    amsys = (SYS_OBJECT) malloc(sizeof *amsys);
    if (!amsys) {
        fprintf(stderr,
                "Amnuts: Failed to create system object in create_system().\n");
        boot_exit(21);
        return;
    }
    memset(amsys, 0, (sizeof *amsys));
#ifdef NETLINKS
    amsys->auto_connect = 1;
#endif
    amsys->max_users = 50;
    amsys->max_clones = 1;
    amsys->ban_swearing = 0;
    amsys->heartbeat = 2;
#ifdef NETLINKS
    amsys->keepalive_interval = 60; /* DO NOT TOUCH!!! */
    amsys->net_idle_time = 300; /* Must be > than the above */
#endif
    amsys->login_idle_time = 180;
    amsys->user_idle_time = 300;
    amsys->time_out_afks = 0;
#ifdef WIZPORT
    amsys->wizport_level = WIZ;
#endif
    amsys->minlogin_level = NUM_LEVELS;
    amsys->mesg_life = 1;
    amsys->num_of_users = 0;
    amsys->num_of_logins = 0;
    amsys->logging = SYSLOG | REQLOG | NETLOG | ERRLOG;
    amsys->ignore_sigterm = 0;
    amsys->crash_action = 0;
    amsys->prompt_def = 1;
    amsys->colour_def = 1;
    amsys->charecho_def = 0;
    amsys->passwordecho_def = 0;
    amsys->time_out_maxlevel = USER;
    amsys->mesg_check_hour = 0;
    amsys->mesg_check_min = 0;
    amsys->mesg_check_done = -1;
    amsys->rs_countdown = 0;
    amsys->rs_announce = 0;
    amsys->rs_which = -1;
    amsys->rs_user = NULL;
    amsys->gatecrash_level = NUM_LEVELS; /* minimum user level which can enter private rooms */
    amsys->min_private_users = 2; /* minimum num. of users in room before can set to priv */
    amsys->ignore_mp_level = GOD; /* User level which can ignore the above var. */
#ifdef NETLINKS
    amsys->rem_user_maxlevel = USER;
    amsys->rem_user_deflevel = USER;
#endif
    amsys->logons_old = 0;
    amsys->logons_new = 0;
    amsys->purge_count = 0;
    amsys->purge_skip = 0;
    amsys->users_purged = 0;
    amsys->auto_purge_date = -1;
    amsys->suggestion_count = 0;
    amsys->forwarding = 1;
    amsys->user_count = 0;
    amsys->allow_recaps = 1;
    amsys->auto_promote = 1;
    amsys->personal_rooms = 1;
    amsys->startup_room_parse = 1;
    amsys->motd1_cnt = 0;
    amsys->motd2_cnt = 0;
    amsys->random_motds = 1;
    amsys->last_cmd_cnt = 0;
    amsys->resolve_ip = 1; /* auto resolve ip */
    amsys->flood_protect = 1;
    amsys->boot_off_min = 0;
    amsys->stop_logins = 0;
    *amsys->default_warp = '\0';
    *amsys->default_jail = '\0';
#ifdef GAMES
    *amsys->default_bank = '\0';
    *amsys->default_shoot = '\0';
#endif
    amsys->mport_socket = -1;
    *amsys->mport_port = '\0';
#ifdef WIZPORT
    amsys->wport_socket = -1;
    *amsys->wport_port = '\0';
#endif
#ifdef NETLINKS
    amsys->nlink_socket = -1;
    *amsys->nlink_port = '\0';
    *amsys->verification = '\0';
#endif
#ifdef IDENTD
    amsys->ident_pid = 0;
    amsys->ident_socket = -1;
    amsys->ident_state = 0;
#endif
    amsys->is_pager = 0;
    time(&amsys->boot_time);
    if (uname(&amsys->uts) < 0) {
        *amsys->uts.sysname = '\0';
        *amsys->uts.machine = '\0';
        *amsys->uts.release = '\0';
        *amsys->uts.version = '\0';
        *amsys->uts.nodename = '\0';
        strncat(amsys->uts.sysname, "[undetermined]",
                (sizeof amsys->uts.sysname));
        strncat(amsys->uts.machine, "[undetermined]",
                (sizeof amsys->uts.machine));
        strncat(amsys->uts.release, "[undetermined]",
                (sizeof amsys->uts.release));
        strncat(amsys->uts.version, "[undetermined]",
                (sizeof amsys->uts.version));
        strncat(amsys->uts.nodename, "[undetermined]",
                (sizeof amsys->uts.nodename));
    }
    user_first = NULL;
    user_last = NULL;
    room_first = NULL;
    room_last = NULL; /* This variable is not used yet */
    first_user_entry = NULL;
    last_user_entry = NULL;
    first_command = NULL;
    force_listen = 0;
    no_prompt = 0;
    logon_flag = 0;
    for (i = 0; i < LASTLOGON_NUM; ++i) {
        *last_login_info[i].name = '\0';
        *last_login_info[i].time = '\0';
        last_login_info[i].on = 0;
    }
    for (i = 0; i < 16; ++i) {
        *cmd_history[i] = '\0';
    }
    clear_words();
#ifdef NETLINKS
    nl_first = NULL;
    nl_last = NULL;
#endif
}

/*
 * Construct user/clone object
 */
UR_OBJECT
create_user(void)
{
    UR_OBJECT user;

    user = (UR_OBJECT) malloc(sizeof *user);
    if (!user) {
        write_syslog(SYSLOG | ERRLOG, 0,
                "ERROR: Memory allocation failure in create_user().\n");
        return user;
    }
    memset(user, 0, (sizeof *user));
    /* Append object into linked list. */
    if (!user_first) {
        user_first = user;
        user->prev = NULL;
    } else {
        user_last->next = user;
        user->prev = user_last;
    }
    user->next = NULL;
    user_last = user;

    /* initialise user structure */
    user->type = USER_TYPE;
    user->socket = -1;
    user->attempts = 0;
    user->login = 0;
#ifdef WIZPORT
    user->wizport = 0;
#endif
    *user->site = '\0';
    *user->site_port = '\0';
    *user->name = '\0';
    reset_user(user);
    return user;
}

/*
 * reset the user variables
 */
void
reset_user(UR_OBJECT user)
{
    int i;

    strcpy(user->version, USERVER);
    *user->verify_code = '\0';
    *user->email = '\0';
    *user->homepage = '\0';
    *user->icq = '\0';
    *user->recap = '\0';
    *user->bw_recap = '\0';
    strcpy(user->desc, "is a newbie");
    strcpy(user->in_phrase, "enters");
    strcpy(user->out_phrase, "goes");
    *user->afk_mesg = '\0';
    *user->pass = '\0';
    *user->last_site = '\0';
    *user->page_file = '\0';
    *user->mail_to = '\0';
    *user->inpstr_old = '\0';
    *user->buff = '\0';
    *user->call = '\0';
    *user->samesite_check_store = '\0';
#ifdef GAMES
    *user->hang_word = '\0';
    *user->hang_word_show = '\0';
    *user->hang_guess = '\0';
#endif
    *user->invite_by = '\0';
    strcpy(user->logout_room, room_first->name);
    strcpy(user->date, (long_date(1)));
    for (i = 0; i < MAX_COPIES; ++i) {
        *user->copyto[i] = '\0';
    }
    for (i = 0; i < 10; ++i) {
        *user->macros[i] = '\0';
    }
#ifdef NETLINKS
    user->netlink = NULL;
    user->pot_netlink = NULL;
#endif
    user->room = NULL;
    user->invite_room = NULL;
    user->malloc_start = NULL;
    user->malloc_end = NULL;
    user->owner = NULL;
    user->wrap_room = NULL;
    user->read_mail = time(0);
    user->last_input = time(0);
    user->last_login = time(0);
    user->retired = 0;
    user->level = NEW;
    user->real_level = user->level;
    user->unarrest = NEW;
    user->arrestby = JAILED; /* FIXME: Use sentinel other JAILED */
    user->buffpos = 0;
    user->filepos = 0;
    user->command_mode = 0;
    user->vis = 1;
    user->ignall = 0;
    user->ignall_store = 0;
    user->ignshouts = 0;
    user->igntells = 0;
    user->muzzled = JAILED; /* FIXME: Use sentinel other JAILED */
    user->last_login_len = 0;
    user->total_login = 0;
    user->prompt = amsys->prompt_def;
    user->colour = amsys->colour_def;
    user->charmode_echo = amsys->charecho_def;
    user->show_pass = amsys->passwordecho_def;
    user->misc_op = 0;
    user->edit_op = 0;
    user->edit_line = 0;
    user->charcnt = 0;
    user->warned = 0;
    user->accreq = 0;
    user->afk = 0;
    user->clone_hear = CLONE_HEAR_ALL;
    user->wipe_to = 0;
    user->wipe_from = 0;
    user->wrap = 0;
    user->pager = 23;
    user->logons = 0;
    user->expire = 1;
    user->lroom = 0;
    user->monitor = 0;
    user->gender = NEUTER;
    user->age = 0;
    user->hideemail = 1;
    user->misses = 0;
    user->hits = 0;
    user->kills = 0;
    user->deaths = 0;
    user->bullets = 6;
    user->hps = 10;
    user->alert = 0;
    user->mail_verified = 0;
    user->autofwd = 0;
    user->ignpics = 0;
    user->ignlogons = 0;
    user->igngreets = 0;
    user->ignwiz = 0;
    user->ignbeeps = 0;
    user->samesite_all_store = 0;
    user->cmd_type = 0;
    user->hang_stage = -1;
    user->show_rdesc = 1;
    user->lmail_all = 0;
    user->lmail_lev = NUM_LEVELS;
    for (i = 0; i < MAX_REMINDERS; ++i) {
        user->reminder[i].date = -1;
        *user->reminder[i].msg = '\0';
    }
    user->reminder_pos = -1;
    for (i = 0; i < MAX_XCOMS; ++i) {
        user->xcoms[i] = -1;
    }
    for (i = 0; i < MAX_GCOMS; ++i) {
        user->gcoms[i] = -1;
    }
    for (i = 0; i < MAX_PAGES; ++i) {
        user->pages[i] = 0;
    }
    user->pagecnt = 0;
    user->login_prompt = 1;
    user->user_page_pos = 0;
    user->user_page_lev = NUM_LEVELS;
    user->pm_count = 0;
    user->pm_currcount = 0;
    user->universal_pager = 0;
    user->pm_current = NULL;
    user->pm_first = NULL;
    user->pm_last = NULL;
    user->fu_first = NULL;
    user->fu_last = NULL;
    user->rb_first = NULL;
    user->rb_last = NULL;
    user->money = DEFAULT_MONEY;
    user->bank = DEFAULT_BANK;
    user->inctime = 0;
    user->reverse_buffer = 0;
}

/*
 * Destruct an user object from linked list
 */
void
destruct_user(UR_OBJECT user)
{
    if (user == user_first) {
        user_first = user->next;
        if (user == user_last) {
            user_last = NULL;
        } else {
            user_first->prev = NULL;
        }
    } else {
        user->prev->next = user->next;
        if (user == user_last) {
            user_last = user->prev;
            user_last->next = NULL;
        } else {
            user->next->prev = user->prev;
        }
    }

    if (user) {
        memset(user, 0, (sizeof *user));
        free(user);
        user = NULL;
    }

    destructed = 1;
}

/*
 * Construct room object
 */
RM_OBJECT
create_room(void)
{
    RM_OBJECT room;
    int i;

    room = (RM_OBJECT) malloc(sizeof *room);
    if (!room) {
        fprintf(stderr, "Amnuts: Memory allocation failure in create_room().\n");
        boot_exit(1);
    }
    memset(room, 0, (sizeof *room));
    /* Append object into linked list. */
    if (!room_first) {
        room_first = room;
        room->prev = NULL;
    } else {
        room_last->next = room;
        room->prev = room_last;
    }
    room->next = NULL;
    room_last = room;
    *room->name = '\0';
    *room->label = '\0';
    *room->desc = '\0';
    *room->topic = '\0';
    *room->map = '\0';
    *room->show_name = '\0';
    *room->owner = '\0';
    room->access = -1;
    room->revline = 0;
    room->mesg_cnt = 0;
#ifdef NETLINKS
    room->inlink = 0;
    room->netlink = NULL;
    *room->netlink_name = '\0';
#endif
    room->next = NULL;
    for (i = 0; i < MAX_LINKS; ++i) {
        *room->link_label[i] = '\0';
        room->link[i] = NULL;
    }
    for (i = 0; i < REVIEW_LINES; ++i) {
        *room->revbuff[i] = '\0';
    }
    return room;
}

/*
 * Destruct a room object from the linked list.
 */
void
destruct_room(RM_OBJECT rm)
{
    /* Remove from linked list */
    if (rm == room_first) {
        room_first = rm->next;
        if (rm == room_last) {
            room_last = NULL;
        } else {
            room_first->prev = NULL;
        }
    } else {
        rm->prev->next = rm->next;
        if (rm == room_last) {
            room_last = rm->prev;
            room_last->next = NULL;
        } else {
            rm->next->prev = rm->prev;
        }
    }
    if (rm) {
        memset(rm, 0, (sizeof *rm));
        free(rm);
        rm = NULL;
    }
}

/*
 * add a command to the commands linked list.  Get which command via the passed id
 * int and the enum in the header file
 */
int
add_command(enum cmd_value cmd_id)
{
    CMD_OBJECT cmd, tmp;

    cmd = (CMD_OBJECT) malloc(sizeof *cmd);
    if (!cmd) {
        write_syslog(SYSLOG | ERRLOG, 0,
                "ERROR: Memory allocation failure in add_command().\n");
        return 0;
    }
    memset(cmd, 0, (sizeof *cmd));
    strcpy(cmd->name, command_table[cmd_id].name);
    cmd->id = cmd_id;
    strcpy(cmd->alias, command_table[cmd_id].alias);
    cmd->level = (enum lvl_value) command_table[cmd_id].level;
    cmd->function = command_table[cmd_id].function;
    cmd->count = 0;

    /*
       do an insertion sort on the linked list
       this could take a long time, but it only needs to be done once when booting, so it
       does not really matter
     */
    for (tmp = first_command; tmp; tmp = tmp->next) {
        if (strcmp(cmd->name, tmp->name) < 0) {
            break;
        }
    }
    if (!tmp) {
        if (tmp == first_command) {
            /* insert in an empty list */
            first_command = cmd;
            cmd->prev = NULL;
            cmd->next = NULL;
            last_command = cmd;
        } else {
            /* insert at the end of the list */
            last_command->next = cmd;
            cmd->prev = last_command;
            cmd->next = NULL;
            last_command = cmd;
        }
    } else {
        if (tmp == first_command) {
            /* insert as first item in the list */
            first_command->prev = cmd;
            cmd->prev = NULL;
            cmd->next = first_command;
            first_command = cmd;
        } else {
            /* insert in the middle of the list somewhere */
            tmp->prev->next = cmd;
            cmd->prev = tmp->prev;
            tmp->prev = cmd;
            cmd->next = tmp;
        }
    }
    return 1;
}

/*
 * destruct command nodes
 */
int
rem_command(enum cmd_value cmd_id)
{
    CMD_OBJECT cmd;

    /*
       as command object not being passed, first find what node we want to delete
       for the id integer that *is* passed.
     */
    for (cmd = first_command; cmd; cmd = cmd->next) {
        if ((enum cmd_value) cmd->id == cmd_id) {
            break;
        }
    }
    if (!cmd) {
        return 0;
    }
    if (cmd == first_command) {
        first_command = cmd->next;
        if (cmd == last_command) {
            last_command = NULL;
        } else {
            first_command->prev = NULL;
        }
    } else {
        cmd->prev->next = cmd->next;
        if (cmd == last_command) {
            last_command = cmd->prev;
            last_command->next = NULL;
        } else {
            cmd->next->prev = cmd->prev;
        }
    }
    memset(cmd, 0, (sizeof *cmd));
    free(cmd);
    return 1;
}

/*
 * add a user node to the user linked list
 */
int
add_user_node(const char *name, enum lvl_value level)
{
    UD_OBJECT entry;

    entry = (UD_OBJECT) malloc(sizeof *entry);
    if (!entry) {
        write_syslog(SYSLOG | ERRLOG, 0,
                "ERROR: Memory allocation failure in add_user_node().\n");
        return 0;
    }
    memset(entry, 0, (sizeof *entry));
    if (!first_user_entry) {
        first_user_entry = entry;
        entry->prev = NULL;
    } else {
        last_user_entry->next = entry;
        entry->prev = last_user_entry;
    }
    entry->next = NULL;
    last_user_entry = entry;
    strcpy(entry->name, name);
    entry->level = level;
    *entry->date = '\0';
    entry->retired = 0;
    ++amsys->user_count;
    ++amsys->level_count[entry->level];
    return 1;
}

/*
 * remove a user node from the user linked list
 */
int
rem_user_node(const char *name)
{
    UD_OBJECT entry;

    for (entry = first_user_entry; entry; entry = entry->next) {
        if (!strcasecmp(entry->name, name)) {
            break;
        }
    }
    if (!entry) {
        return 0;
    }
    --amsys->user_count;
    --amsys->level_count[entry->level];
    if (entry == first_user_entry) {
        first_user_entry = entry->next;
        if (entry == last_user_entry) {
            last_user_entry = NULL;
        } else {
            first_user_entry->prev = NULL;
        }
    } else {
        entry->prev->next = entry->next;
        if (entry == last_user_entry) {
            last_user_entry = entry->prev;
            last_user_entry->next = NULL;
        } else {
            entry->next->prev = entry->prev;
        }
    }
    memset(entry, 0, (sizeof *entry));
    free(entry);
    return 1;
}

/*
 * put a date string in a node in the directory linked list that
 * matches name
 */
void
add_user_date_node(const char *name, const char *date)
{
    UD_OBJECT entry;

    for (entry = first_user_entry; entry; entry = entry->next) {
        if (!strcasecmp(entry->name, name)) {
            break;
        }
    }
    if (entry) {
        strcpy(entry->date, date);
    }
    return;
}

/*
 * alter the level of a node in the user linked list
 */
int
user_list_level(const char *name, enum lvl_value lvl)
{
    UD_OBJECT entry;

    for (entry = first_user_entry; entry; entry = entry->next) {
        if (!strcmp(entry->name, name)) {
            break;
        }
    }
    if (entry) {
        --amsys->level_count[entry->level];
        entry->level = lvl;
        ++amsys->level_count[entry->level];
    }
    return !!entry;
}

/*
 * set a flag in the "flagged users" list (ignore, friend, etc)
 */
int
setbit_flagged_user_entry(UR_OBJECT user, char *name, unsigned flags)
{
    FU_OBJECT fu, next;
    int rc;

    /* check to see if the user is already in there */
    for (fu = user->fu_first; fu; fu = next) {
        next = fu->next;
        if (!strcasecmp(fu->name, name)) {
            break;
        }
    }
    if (!fu) {
        /* guess they are not */
        rc = create_flagged_user_entry(user, name, flags);
    } else {
        fu->flags |= flags;
        rc = 1;
    }
    /* finish up */
    save_flagged_users(user);
    return rc;
}

/*
 * unset a flag in the "flagged users" list (ignore, friend, etc)
 */
int
unsetbit_flagged_user_entry(UR_OBJECT user, char *name, unsigned flags)
{
    FU_OBJECT fu, next;
    int rc;

    /* check to see if the user is already in there */
    for (fu = user->fu_first; fu; fu = next) {
        next = fu->next;
        if (!strcasecmp(fu->name, name)) {
            break;
        }
    }
    if (!fu) {
        return 0;
    }
    fu->flags &= ~flags;
    /* check to see if we need to remove the entry */
    if (!fu->flags) {
        rc = destruct_flagged_user_entry(user, fu);
    } else {
        rc = 1;
    }
    /* finish up */
    save_flagged_users(user);
    return rc;
}

/*
 * unset all flagged users of a particular flag
 */
void
all_unsetbit_flagged_user_entry(UR_OBJECT user, unsigned flags)
{
    FU_OBJECT fu, next;

    /* check to see if the user is already in there */
    for (fu = user->fu_first; fu; fu = next) {
        next = fu->next;
        fu->flags &= ~flags;
        /* check to see if we need to remove the entry */
        if (!fu->flags) {
            destruct_flagged_user_entry(user, fu);
        }
    }
    /* finish up */
    save_flagged_users(user);
}

/*
 * create the flagged user object for insertion into the user object
 */
int
create_flagged_user_entry(UR_OBJECT user, char *name, unsigned flags)
{
    FU_OBJECT fu;

    fu = (FU_OBJECT) malloc(sizeof *fu);
    if (!fu) {
        return 0;
    }
    fu->name = (char *) malloc(1 + strlen(name));
    if (!fu->name) {
        free(fu);
        return 0;
    }
    strcpy(fu->name, name);
    fu->flags = 0 | flags;
    if (!user->fu_first) {
        user->fu_first = fu;
        fu->prev = NULL;
    } else {
        user->fu_last->next = fu;
        fu->prev = user->fu_last;
    }
    fu->next = NULL;
    user->fu_last = fu;
    return 1;
}

/*
 * destruct a node in the flagged user linked list
 */
int
destruct_flagged_user_entry(UR_OBJECT user, FU_OBJECT fu)
{
    if (!fu) {
        return 0;
    }
    if (fu->name) {
        memset(fu->name, 0, 1 + strlen(fu->name));
        free(fu->name);
    }
    if (fu == user->fu_first) {
        user->fu_first = fu->next;
        if (fu == user->fu_last || !user->fu_first) {
            user->fu_last = NULL;
        } else {
            user->fu_first->prev = NULL;
        }
    } else {
        fu->prev->next = fu->next;
        if (fu == user->fu_last) {
            user->fu_last = fu->prev;
            user->fu_last->next = NULL;
        } else {
            fu->next->prev = fu->prev;
        }
    }
    memset(fu, 0, (sizeof *fu));
    free(fu);
    return 1;
}

/*
 * destruct all nodes in flagged user linked list
 */
void
destruct_all_flagged_users(UR_OBJECT user)
{
    while (user->fu_first) {
        destruct_flagged_user_entry(user, user->fu_first);
    }
    user->fu_first = user->fu_last = NULL;
}

/*
 * read flagged users from file
 */
int
load_flagged_users(UR_OBJECT user)
{
    char filename[80], name[USER_NAME_LEN + 1];
    FILE *fp;
    int f;
    unsigned flags, errors;

    sprintf(filename, "%s/%s/%s.U", USERFILES, USERFLAGGED, user->name);
    fp = fopen(filename, "r");
    if (!fp) {
        return 1;
    }
    errors = 0;
    for (f = fscanf(fp, "%s %u\n", name, &flags); f == 2;
            f = fscanf(fp, "%s %u\n", name, &flags)) {
        if (!create_flagged_user_entry(user, name, flags)) {
            write_syslog(SYSLOG | ERRLOG, 1,
                    "ERROR: Cannot create flagged user object for %s\n",
                    user->name);
            ++errors;
        }
    }
    fclose(fp);
    return !errors;
}

/*
 * saved flagged user list to a file
 */
int
save_flagged_users(UR_OBJECT user)
{
    char filename[80];
    FILE *fp;
    FU_OBJECT fu;

    sprintf(filename, "%s/%s/%s.U", USERFILES, USERFLAGGED, user->name);
    if (!user->fu_first) {
        remove(filename);
        return 1;
    }
    fp = fopen(filename, "w");
    if (!fp) {
        write_syslog(SYSLOG | ERRLOG, 1,
                "ERROR: Cannot save flagged user list for %s\n", user->name);
        return 0;
    }
    for (fu = user->fu_first; fu; fu = fu->next) {
        fprintf(fp, "%s %u\n", fu->name, fu->flags);
    }
    fclose(fp);
    return 1;
}

/*
 * Get user from name
 */
UR_OBJECT
get_user(const char *name)
{
    UR_OBJECT u;

    /* Search for exact name */
    for (u = user_first; u; u = u->next) {
        if (u->login || u->type == CLONE_TYPE) {
            continue;
        }
        if (!strcasecmp(u->name, name)) {
            break;
        }
    }
    return u;
}

/*
 * Get user from abbreviated name
 */
UR_OBJECT
get_user_name(UR_OBJECT user, const char *name)
{
    UR_OBJECT u, last;
    int found;
    size_t len;

    len = strlen(name);
    last = NULL;
    found = 0;
    *text = '\0';
    for (u = user_first; u; u = u->next) {
        if (u->login || u->type == CLONE_TYPE) {
            continue;
        }
        if (!strncasecmp(u->name, name, len)) {
            if (strlen(u->name) == len) {
                break;
            }
            /* FIXME: Bounds checking */
            strcat(text, found++ % 8 ? "~RS  " : "\n  ");
            strcat(text, u->recap);
            last = u;
        }
    }
    if (u) {
        found = 1;
        last = u;
    }
    if (found > 1) {
        if (user) {
            strcat(text, found % 8 ? "\n\n" : "\n");
            vwrite_user(user,
                    "~FR~OLName is not unique. \"~FT%s~RS~FR~OL\" also matches:\n",
                    name);
            write_user(user, text);
        }
        last = NULL;
    }
    *text = '\0';
    return last;
}

/*
 * retrieve_user() and done_retrieve() by Ardant (ardant@ardant.net)
 * basically the above two functions rolled into one easy function
 * modified to allow for NULL user object
 */
UR_OBJECT
retrieve_user(UR_OBJECT user, const char *name)
{
    UR_OBJECT u;
    UD_OBJECT entry, last;
    int found;
    size_t len;

    len = strlen(name);
    last = NULL;
    found = 0;
    *text = '\0';
    for (entry = first_user_entry; entry; entry = entry->next) {
        if (!strncasecmp(entry->name, name, len)) {
            if (strlen(entry->name) == len) {
                break;
            }
            /* FIXME: Bounds checking */
            strcat(text, found++ % 8 ? "  " : "\n  ");
            strcat(text, entry->name);
            last = entry;
        }
    }
    if (entry) {
        found = 1;
        last = entry;
    }
    if (found > 1) {
        if (user) {
            vwrite_user(user,
                    "\n~FR~OLName is not unique. \"~FT%s~RS~FR~OL\" also matches:\n",
                    name);
            vwrite_user(user, "   %s\n\n", text);
        }
        retrieve_user_type = 0;
        *text = '\0';
        return NULL;
    }
    *text = '\0';
    if (!found) {
        if (user) {
            write_user(user, nosuchuser);
        }
        retrieve_user_type = 0;
        return NULL;
    }
    u = get_user(last->name);
    if (u) {
        retrieve_user_type = 1;
        return u;
    }
    u = create_user();
    if (!u) {
        sprintf(text, "%s: unable to create temporary user object.\n", syserror);
        if (user) {
            write_user(user, text);
        }
        write_syslog(SYSLOG, 1, "%s", text);
        *text = '\0';
        retrieve_user_type = 0;
        return NULL;
    }
    strcpy(u->name, last->name);
    if (!load_user_details(u)) {
        destruct_user(u);
        destructed = 0;
        if (user) {
            write_user(user, nosuchuser);
        }
        retrieve_user_type = 0;
        return NULL;
    }
    retrieve_user_type = 2;
    return u;
}

/*
 * clean up the user object created by the above function
 */
void
done_retrieve(UR_OBJECT user)
{
    switch (retrieve_user_type) {
    case 0:
        /* not supposed to happen */
        break;
    case 1:
        /* online user, do not do anything */
        break;
    case 2:
        /* offline user, save and have fun */
        user->socket = -2;
        strcpy(user->site, user->last_site);
        save_user_details(user, 0);
        destruct_user(user);
        destructed = 0;
        break;
    }
    return;
}

/*
 * Get room from abbreviated name
 */
RM_OBJECT
get_room(const char *name)
{
    RM_OBJECT rm;
    size_t len;

    /* FIXME: exact match not checked for; use length for partial */
    len = strlen(name);
    for (rm = room_first; rm; rm = rm->next) {
        if (!strncasecmp(rm->name, name, len)) {
            break;
        }
    }
    return rm;
}

/*
 * Get room from full name
 */
RM_OBJECT
get_room_full(const char *name)
{
    RM_OBJECT rm;

    for (rm = room_first; rm; rm = rm->next) {
        if (!strcmp(rm->name, name)) {
            break;
        }
    }
    return rm;
}

/*
 * Return level value based on level name
 */
enum lvl_value
get_level(const char *name)
{
    enum lvl_value lvl;

    for (lvl = JAILED; lvl < NUM_LEVELS; lvl = (enum lvl_value) (lvl + 1)) {
        if (!strcasecmp(user_level[lvl].name, name)) {
            break;
        }
    }
    return lvl;
}

/*
 * create a review buffer object for insertion into the user object
 */
int
create_review_buffer_entry(UR_OBJECT user, const char *name,
        const char *buffer, unsigned flags)
{
    RB_OBJECT rb;

    rb = (RB_OBJECT) malloc(sizeof *rb);
    if (!rb) {
        return 0;
    }
    rb->name = (char *) malloc(1 + strlen(name));
    if (!rb->name) {
        free(rb);
        return 0;
    }
    rb->buffer = (char *) malloc(1 + strlen(buffer));
    if (!rb->buffer) {
        free(rb->name);
        free(rb);
        return 0;
    }
    strcpy(rb->name, name);
    strcpy(rb->buffer, buffer);
    rb->flags = 0 | flags;
    if (!user->rb_first) {
        user->rb_first = rb;
        rb->prev = NULL;
    } else {
        user->rb_last->next = rb;
        rb->prev = user->rb_last;
    }
    rb->next = NULL;
    user->rb_last = rb;
    return 1;
}

/*
 * destruct a node in the review buffer linked list
 */
int
destruct_review_buffer_entry(UR_OBJECT user, RB_OBJECT rb)
{
    if (!rb) {
        return 0;
    }
    if (rb->name) {
        memset(rb->name, 0, 1 + strlen(rb->name));
        free(rb->name);
    }
    if (rb->buffer) {
        memset(rb->buffer, 0, 1 + strlen(rb->buffer));
        free(rb->buffer);
    }
    if (rb == user->rb_first) {
        user->rb_first = rb->next;
        if (rb == user->rb_last || !user->rb_first) {
            user->rb_last = NULL;
        } else {
            user->rb_first->prev = NULL;
        }
    } else {
        rb->prev->next = rb->next;
        if (rb == user->rb_last) {
            user->rb_last = rb->prev;
            user->rb_last->next = NULL;
        } else {
            rb->next->prev = rb->prev;
        }
    }
    memset(rb, 0, (sizeof *rb));
    free(rb);
    return 1;
}

/*
 * destruct all nodes in review buffer linked list
 */
void
destruct_all_review_buffer(UR_OBJECT user)
{
    while (user->rb_first) {
        destruct_review_buffer_entry(user, user->rb_first);
    }
    user->rb_first = user->rb_last = NULL;
}

/*
 * destruct all nodes of a certain type in review buffer linked list
 */
void
destruct_review_buffer_type(UR_OBJECT user, unsigned flags, int first)
{
    RB_OBJECT rb, next;

    if (!user->rb_first) {
        return;
    }
    for (rb = user->rb_first; rb; rb = next) {
        next = rb->next;
        if (rb->flags & flags) {
            destruct_review_buffer_entry(user, rb);
            if (first) {
                return;
            }
        }
    }
}
