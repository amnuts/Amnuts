/****************************************************************************
             Amnuts - Copyright (C) Andrew Collington, 1996-2023
                        Last update: Sometime in 2023

                   talker@amnuts.net - https://amnuts.net/

                                 based on

   NUTS version 3.3.3 (Triple Three :) - Copyright (C) Neil Robertson 1996
 ***************************************************************************/

#ifndef AMNUTS_PROTOTYPES_H
#define AMNUTS_PROTOTYPES_H

#include "../vendors/sds/sds.h"

#ifndef __attribute__
#if !defined __GNUC__ || __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 7)
#define __attribute__(x)
#else
#define __attribute__(x) __attribute__(x)
#endif
#endif

#ifndef _XOPEN_SOURCE
/*
 * external prototypes used:
 * comment these out should you get an error with them...but you shouldn't!
 */
#ifdef  __cplusplus
extern "C" {
#endif
    extern char *crypt(const char *, const char *);
#ifdef  __cplusplus
}
#endif
#endif
/*
 * functions in amnuts230.c
 */
int main(int, char **);
void check_directories(void);
int find_user_listed(const char *);
int user_logged_on(const char *);
int setup_readmask(fd_set *);
void accept_connection(int);
#ifdef MANDNS
char *resolve_ip(char *);
#endif
int socket_listen(const char *, const char *);
void load_and_parse_config(void);
void parse_init_section(void);
void parse_rooms_section(void);
void parse_topics_section(char *);
#ifdef NETLINKS
void parse_sites_section(void);
#endif
void init_signals(void);
void sig_handler(int);
void boot_exit(int)
__attribute__((__noreturn__));
void check_reboot_shutdown(void);
void check_idle_and_timeout(void);
void record_last_login(const char *);
void record_last_logout(const char *);
int load_user_details(UR_OBJECT);
int save_user_details(UR_OBJECT, int);
int load_user_details_old(UR_OBJECT);
int load_oldversion_user(UR_OBJECT, int);
void process_users(void);
void parse_commands(void);
void clean_files(char *);
int remove_top_bottom(char *, int);
int count_lines(char *);
void write_sock(int, const char *);
void vwrite_user(UR_OBJECT, const char *, ...)
__attribute__((__format__(__printf__, 2, 3)));
void write_user(UR_OBJECT, const char *);
void vwrite_level(enum lvl_value, int, int, UR_OBJECT, const char *, ...)
__attribute__((__format__(__printf__, 5, 6)));
void write_level(enum lvl_value, int, int, const char *, UR_OBJECT);
void vwrite_room(RM_OBJECT, const char *, ...)
__attribute__((__format__(__printf__, 2, 3)));
void write_room(RM_OBJECT, const char *);
void vwrite_room_except(RM_OBJECT, UR_OBJECT, const char *, ...)
__attribute__((__format__(__printf__, 3, 4)));
void write_room_except(RM_OBJECT, const char *, UR_OBJECT);
void vwrite_room_except_both(RM_OBJECT, UR_OBJECT, UR_OBJECT,
        const char *, ...)
__attribute__((__format__(__printf__, 4, 5)));
void write_room_except_both(RM_OBJECT, const char *, UR_OBJECT, UR_OBJECT);
void vwrite_room_ignore(RM_OBJECT rm, UR_OBJECT user, const char *str, ...)
__attribute__((__format__(__printf__, 3, 4)));
void write_room_ignore(RM_OBJECT rm, UR_OBJECT user, const char *str);
void write_friends(UR_OBJECT, const char *, int);
void write_syslog(int, int, const char *, ...)
__attribute__((__format__(__printf__, 3, 4)));
int do_write_syslog(const char *);
void record_last_command(UR_OBJECT, CMD_OBJECT, size_t);
void dump_commands(int);
void write_monitor(UR_OBJECT, RM_OBJECT, int);
int more(UR_OBJECT, int, const char *);
int more_users(UR_OBJECT);
void add_history(char *, int, const char *, ...)
__attribute__((__format__(__printf__, 3, 4)));
void login(UR_OBJECT, char *);
void attempts(UR_OBJECT);
void show_login_info(UR_OBJECT);
void connect_user(UR_OBJECT);
void disconnect_user(UR_OBJECT);
int misc_ops(UR_OBJECT, char *);
int exec_com(UR_OBJECT, char *, enum cmd_value);
void look(UR_OBJECT);
void who(UR_OBJECT, int);
void login_who(UR_OBJECT);
void display_files(UR_OBJECT, int);
void help(UR_OBJECT);
void help_commands_level(UR_OBJECT);
void help_commands_function(UR_OBJECT);
void help_nuts_credits(UR_OBJECT);
void help_amnuts_credits(UR_OBJECT);
void status(UR_OBJECT);
void examine(UR_OBJECT);
void set_attributes(UR_OBJECT);
void show_attributes(UR_OBJECT);
void prompt(UR_OBJECT);
void toggle_prompt(UR_OBJECT);
void toggle_mode(UR_OBJECT);
void toggle_charecho(UR_OBJECT);
void set_desc(UR_OBJECT, char *);
void set_iophrase(UR_OBJECT, char *);
void enter_profile(UR_OBJECT, char *);
void account_request(UR_OBJECT, char *);
void afk(UR_OBJECT, char *);
void get_macros(UR_OBJECT);
void macros(UR_OBJECT);
void check_macros(UR_OBJECT, char *);
void visibility(UR_OBJECT, int);
void show_igusers(UR_OBJECT);
int check_igusers(UR_OBJECT, UR_OBJECT);
void set_igusers(UR_OBJECT);
void set_ignore(UR_OBJECT);
void show_ignlist(UR_OBJECT);
void toggle_ignall(UR_OBJECT);
void user_listen(UR_OBJECT);
void show_last_login(UR_OBJECT);
void display_colour(UR_OBJECT);
void show_ranks(UR_OBJECT);
void wiz_list(UR_OBJECT);
void get_time(UR_OBJECT);
void show_version(UR_OBJECT);
void show_command_counts(UR_OBJECT);
int count_friends(UR_OBJECT);
int user_is_friend(UR_OBJECT, UR_OBJECT);
void alert_friends(UR_OBJECT);
void friends(UR_OBJECT);
int is_leap(int);
long months_to_days(int);
long years_to_days(int);
long ymd_to_scalar(int, int, int);
void scalar_to_ymd(long, int *, int *, int *);
int is_ymd_today(int, int, int);
void show_calendar(UR_OBJECT);
int has_reminder(UR_OBJECT, int, int, int);
int has_reminder_today(UR_OBJECT);
int remove_old_reminders(UR_OBJECT);
int read_user_reminders(UR_OBJECT);
int write_user_reminders(UR_OBJECT);
void show_reminders(UR_OBJECT, int);
void show_flagged_users(UR_OBJECT);


/*
 * functions in admin.c
 */
void talker_shutdown(UR_OBJECT, const char *, int);
void shutdown_com(UR_OBJECT);
void reboot_com(UR_OBJECT);
void sreboot_com(UR_OBJECT);
#ifdef IDENTD
void start_ident(UR_OBJECT);
void resite(UR_OBJECT);
#endif
int site_banned(char *, int);
int login_port_flood(char *);
int user_banned(char *);
void listbans(UR_OBJECT);
void ban(UR_OBJECT);
void auto_ban_site(char *);
void ban_site(UR_OBJECT);
void ban_user(UR_OBJECT);
void ban_new(UR_OBJECT);
void unban(UR_OBJECT);
void unban_site(UR_OBJECT);
void unban_user(UR_OBJECT);
void unban_new(UR_OBJECT);
void make_invis(UR_OBJECT);
void make_vis(UR_OBJECT);
void create_clone(UR_OBJECT);
void destroy_user_clones(UR_OBJECT);
void destroy_clone(UR_OBJECT);
void myclones(UR_OBJECT);
void allclones(UR_OBJECT);
void clone_switch(UR_OBJECT);
void check_autopromote(UR_OBJECT, int);
void temporary_promote(UR_OBJECT);
void promote(UR_OBJECT);
void demote(UR_OBJECT);
void muzzle(UR_OBJECT);
void unmuzzle(UR_OBJECT);
void arrest(UR_OBJECT);
void unarrest(UR_OBJECT);
void change_pass(UR_OBJECT);
void kill_user(UR_OBJECT);
void suicide(UR_OBJECT);
void delete_user(UR_OBJECT, int);
int purge(int, char *, int);
void purge_users(UR_OBJECT);
void user_expires(UR_OBJECT);
void create_account(UR_OBJECT);
void force_save(UR_OBJECT);
void viewlog(UR_OBJECT);
void samesite(UR_OBJECT, int);
void site(UR_OBJECT);
void manual_history(UR_OBJECT, char *);
void user_history(UR_OBJECT);
void logging(UR_OBJECT);
void minlogin(UR_OBJECT);
void system_details(UR_OBJECT);
void clearline(UR_OBJECT);
void toggle_swearban(UR_OBJECT);
void retire_user(UR_OBJECT);
void unretire_user(UR_OBJECT);
int is_retired(const char *);
void add_retire_list(const char *);
void clean_retire_list(const char *);
void recount_users(UR_OBJECT, char *);
void grep_users(UR_OBJECT);
void set_command_level(UR_OBJECT);
void user_xcom(UR_OBJECT);
void user_gcom(UR_OBJECT);
int set_xgcom(UR_OBJECT, UR_OBJECT, int, int, int);
int get_xgcoms(UR_OBJECT);
int has_gcom(UR_OBJECT, int);
int has_xcom(UR_OBJECT, int);
void bring(UR_OBJECT);
void force(UR_OBJECT, char *);
void dump_to_file(UR_OBJECT);
void change_user_name(UR_OBJECT);
void shackle(UR_OBJECT);
void unshackle(UR_OBJECT);


#ifdef GAMES
/*
 * functions in games.c
 */
void play_hangman(UR_OBJECT);
void get_hang_word(char *);
void guess_hangman(UR_OBJECT);
void shoot_user(UR_OBJECT);
void reload_gun(UR_OBJECT);
void bank_money(UR_OBJECT);
void donate_cash(UR_OBJECT);
void show_money(UR_OBJECT);
void check_credit_updates(void);
void global_money(UR_OBJECT);
#endif


/*
 * functions in messages.c
 */
void count_suggestions(void);
int count_motds(int);
int get_motd_num(int);
void check_messages(UR_OBJECT, int);
int send_mail(UR_OBJECT, char *, char *, int);
void smail(UR_OBJECT, char *);
void rmail(UR_OBJECT);
void read_specific_mail(UR_OBJECT);
void read_new_mail(UR_OBJECT);
void dmail(UR_OBJECT);
void mail_from(UR_OBJECT);
void copies_to(UR_OBJECT);
void send_copies(UR_OBJECT, char *);
void level_mail(UR_OBJECT, char *);
int send_broadcast_mail(UR_OBJECT, char *, enum lvl_value, int);
int mail_sizes(char *, int);
int reset_mail_counts(UR_OBJECT);
void set_forward_email(UR_OBJECT);
void verify_email(UR_OBJECT);
void forward_email(char *, char *, char *);
int send_forward_email(char *, char *, char *);
#ifdef DOUBLEFORK
int double_fork(void);
#endif
void forward_specific_mail(UR_OBJECT);
void read_board(UR_OBJECT);
void read_board_specific(UR_OBJECT, RM_OBJECT, int);
void write_board(UR_OBJECT, char *);
void wipe_board(UR_OBJECT);
int check_board_wipe(UR_OBJECT);
void board_from(UR_OBJECT);
void search_boards(UR_OBJECT);
void suggestions(UR_OBJECT, char *);
void delete_suggestions(UR_OBJECT);
void suggestions_from(UR_OBJECT);
int get_wipe_parameters(UR_OBJECT);
int wipe_messages(char *, int, int, int);
void friend_smail(UR_OBJECT, char *);
void editor(UR_OBJECT, char *);
void editor_done(UR_OBJECT);


#ifdef NETLINKS
/*
 * functions in netlinks.c
 */
NL_OBJECT create_netlink(void);
void destruct_netlink(NL_OBJECT nl);
void init_connections(void);
int socket_connect(const char *, const char *);
void check_nethangs_send_keepalives(void);
void accept_server_connection(int sock);
void exec_netcom(NL_OBJECT nl, char *inpstr);
void nl_transfer(NL_OBJECT nl, char *name, char *pass,
        enum lvl_value lvl, char *inpstr);
void nl_release(NL_OBJECT nl, char *name);
void nl_action(NL_OBJECT nl, char *name, char *inpstr);
void nl_granted(NL_OBJECT nl, char *name);
void nl_denied(NL_OBJECT nl, char *name, char *inpstr);
void nl_mesg(NL_OBJECT nl, char *name);
void nl_prompt(NL_OBJECT nl, char *name);
void nl_verification(NL_OBJECT nl, char *w2, char *w3, int com);
void nl_removed(NL_OBJECT nl, char *name);
void nl_error(NL_OBJECT nl);
void nl_checkexist(NL_OBJECT nl, char *to, char *from);
void nl_user_notexist(NL_OBJECT nl, char *to, char *from);
void nl_user_exist(NL_OBJECT nl, char *to, char *from);
void nl_mail(NL_OBJECT nl, char *to, char *from);
void nl_endmail(NL_OBJECT nl);
void nl_mailerror(NL_OBJECT nl, char *to, char *from);
void nl_rstat(NL_OBJECT nl, char *to);
void shutdown_netlink(NL_OBJECT nl);
int transfer_nl(UR_OBJECT user);
int release_nl(UR_OBJECT user);
int action_nl(UR_OBJECT user, const char *command, const char *inpstr);
int message_nl(UR_OBJECT user, const char *str);
int prompt_nl(UR_OBJECT user);
int remove_nl(UR_OBJECT user);
int mail_nl(UR_OBJECT user, char *to, const char *mesg);
void home(UR_OBJECT user);
void netstat(UR_OBJECT user);
void netdata(UR_OBJECT user);
void connect_netlink(UR_OBJECT user);
void disconnect_netlink(UR_OBJECT user);
void remote_stat(UR_OBJECT user);
#endif


/*
 * functions in objects.c
 */
void create_system(void);
UR_OBJECT create_user(void);
void reset_user(UR_OBJECT);
void destruct_user(UR_OBJECT);
RM_OBJECT create_room(void);
void destruct_room(RM_OBJECT);
int add_command(enum cmd_value);
int rem_command(enum cmd_value);
int add_user_node(const char *, enum lvl_value);
int rem_user_node(const char *);
void add_user_date_node(const char *, const char *);
int user_list_level(const char *, enum lvl_value);
int setbit_flagged_user_entry(UR_OBJECT, char *, unsigned int);
int unsetbit_flagged_user_entry(UR_OBJECT, char *, unsigned int);
void all_unsetbit_flagged_user_entry(UR_OBJECT, unsigned int);
int create_flagged_user_entry(UR_OBJECT, char *, unsigned int);
int destruct_flagged_user_entry(UR_OBJECT, FU_OBJECT);
void destruct_all_flagged_users(UR_OBJECT);
int load_flagged_users(UR_OBJECT);
int save_flagged_users(UR_OBJECT);
UR_OBJECT get_user(const char *);
UR_OBJECT get_user_name(UR_OBJECT, const char *);
UR_OBJECT retrieve_user(UR_OBJECT, const char *);
void done_retrieve(UR_OBJECT);
RM_OBJECT get_room(const char *);
RM_OBJECT get_room_full(const char *);
enum lvl_value get_level(const char *);
int create_review_buffer_entry(UR_OBJECT, const char *, const char *,
        unsigned int);
int destruct_review_buffer_entry(UR_OBJECT, RB_OBJECT);
void destruct_all_review_buffer(UR_OBJECT);
void destruct_review_buffer_type(UR_OBJECT, unsigned int, int);


/*
 * functions in pager.c
 */
void start_pager(UR_OBJECT);
void stop_pager(UR_OBJECT);
void end_pager(UR_OBJECT);
PM_OBJECT create_buf_mesg(const char *);
int destruct_pag_mesg(UR_OBJECT, PM_OBJECT);
void add_pag_mesg_user(UR_OBJECT, PM_OBJECT);
int add_pm(UR_OBJECT, const char *);
int display_pm(UR_OBJECT);
int rewind_pager(UR_OBJECT, int);
void write_apager(UR_OBJECT);


/*
 * functions in reboot.c
 */
int build_sysinfo(UR_OBJECT);
int build_loggedin_users_list(UR_OBJECT);
int build_loggedin_users_info(UR_OBJECT);
int build_pager_info(UR_OBJECT);
int build_room_info(UR_OBJECT);
#ifdef IDENTD
int build_ident_info(void);
#endif
int build_flagged_user_info(UR_OBJECT);
int build_review_buffer_info(UR_OBJECT);
void close_fds(void);
void do_sreboot(UR_OBJECT);
UR_OBJECT create_user_template(UR_OBJECT);
RM_OBJECT create_room_template(RM_OBJECT);
int retrieve_sysinfo(void);
void retrieve_rooms(void);
void retrieve_users(void);
#ifdef IDENTD
void retrieve_ident(void);
#endif
int possibly_reboot(void);


/*
 * functions in rooms.c
 */
int is_personal_room(RM_OBJECT);
int is_fixed_room(RM_OBJECT);
int is_private_room(RM_OBJECT);
int is_my_room(UR_OBJECT, RM_OBJECT);
int room_visitor_count(RM_OBJECT);
int has_room_key(const char *, RM_OBJECT);
void parse_user_rooms(void);
void personal_room(UR_OBJECT);
void personal_room_lock(UR_OBJECT);
void personal_room_visit(UR_OBJECT);
void personal_room_decorate(UR_OBJECT, char *);
void personal_room_bgone(UR_OBJECT);
int personal_room_store(const char *, int, RM_OBJECT);
void personal_room_admin(UR_OBJECT);
void personal_room_key(UR_OBJECT);
int personal_key_add(UR_OBJECT, char *);
int personal_key_remove(UR_OBJECT, char *);
void personal_room_rename(UR_OBJECT, char *);
void go(UR_OBJECT);
void move_user(UR_OBJECT, RM_OBJECT, int);
void move(UR_OBJECT);
void set_room_access(UR_OBJECT, int);
void change_room_fix(UR_OBJECT, int);
void invite(UR_OBJECT);
void uninvite(UR_OBJECT);
void letmein(UR_OBJECT);
void rooms(UR_OBJECT, int, int);
void clear_topic(UR_OBJECT);
void join(UR_OBJECT);
void set_topic(UR_OBJECT, char *);
void reload_room_description(UR_OBJECT);
void reset_access(RM_OBJECT);
int has_room_access(UR_OBJECT, RM_OBJECT);
int check_start_room(UR_OBJECT);


/*
 * functions in speech.c
 */
void say(UR_OBJECT, char *);
void say_to(UR_OBJECT, char *);
void shout(UR_OBJECT, char *);
void sto(UR_OBJECT, char *);
void tell_user(UR_OBJECT, char *);
void emote(UR_OBJECT, char *);
void semote(UR_OBJECT, char *);
void pemote(UR_OBJECT, char *);
void echo(UR_OBJECT, char *);
void mutter(UR_OBJECT, char *);
void plead(UR_OBJECT, char *);
void wizshout(UR_OBJECT, char *);
void wizemote(UR_OBJECT, char *);
void picture_tell(UR_OBJECT);
void preview(UR_OBJECT);
void picture_all(UR_OBJECT);
void greet(UR_OBJECT, char *);
void think_it(UR_OBJECT, char *);
void sing_it(UR_OBJECT, char *);
void bcast(UR_OBJECT, char *, int);
void wake(UR_OBJECT);
void beep(UR_OBJECT, char *);
void quick_call(UR_OBJECT);
void revtell(UR_OBJECT);
void revedit(UR_OBJECT);
void revafk(UR_OBJECT);
void revshout(UR_OBJECT);
void review(UR_OBJECT);
int review_buffer(UR_OBJECT, unsigned int);
void record(RM_OBJECT, char *);
void record_shout(const char *);
void record_tell(UR_OBJECT, UR_OBJECT, const char *);
void record_afk(UR_OBJECT, UR_OBJECT, const char *);
void record_edit(UR_OBJECT, UR_OBJECT, const char *);
void revclr(UR_OBJECT);
void clear_revbuff(RM_OBJECT);
void clear_tells(UR_OBJECT);
void clear_shouts(void);
void clear_afk(UR_OBJECT);
void clear_edit(UR_OBJECT);
int has_review(UR_OBJECT, unsigned int);
void cls(UR_OBJECT);
void clone_say(UR_OBJECT, char *);
void clone_emote(UR_OBJECT, char *);
void clone_hear(UR_OBJECT);
void show(UR_OBJECT, char *);
void friend_say(UR_OBJECT, char *);
void friend_emote(UR_OBJECT, char *);


/*
 * functions in spodlist.c
 */
int find_spodlist_position(char *);
int people_in_spodlist(void);
void add_name_to_spodlist(char *, int);
void delete_spodlist(void);
void calc_spodlist(void);
void show_spodlist(UR_OBJECT);


/*
 * functions in strings.h
 */
int get_charclient_line(UR_OBJECT, char *, int);
void terminate(char *);
int wordfind(const char *);
void clear_words(void);
int yn_check(const char *);
int onoff_check(const char *);
int minmax_check(const char *);
int resolve_check(const char *);
void echo_off(UR_OBJECT);
void echo_on(UR_OBJECT);
char *remove_first(char *);
int contains_swearing(const char *);
char *censor_swear_words(char *);
char *colour_com_strip(const char *);
void strtoupper(char *);
void strtolower(char *);
void strtoname(char *);
int is_number(const char *);
char *istrstr(char *, const char *);
char *replace_string(char *, const char *, const char *);
char *repeat_string(const char *, int times);
const char *ordinal_text(int);
char *long_date(int);
const char *smiley_type(const char *);
char *align_string(int, int, int, const char *, const char *, ...)
__attribute__((__format__(__printf__, 5, 6)));
int pattern_match(char *, char *);
int validate_email(char *);
char *process_input_string(char *, CMD_OBJECT);
void split_command_string(char *);
size_t teslen(const char *, size_t);
void get_soundex(const char *, char *);
char *word_time(int);

#endif
