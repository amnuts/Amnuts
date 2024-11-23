
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Display all the people logged on from the same site as user
 */
void
samesite(UR_OBJECT user, int stage)
{
    UR_OBJECT u, u_loop;
    UD_OBJECT entry;
    int found, cnt, same, on;

    on = 0;
    if (!stage) {
        if (word_count < 2) {
            write_user(user, "Usage: samesite user|site [all]\n");
            return;
        }
        if (word_count == 3 && !strcasecmp(word[2], "all")) {
            user->samesite_all_store = 1;
        } else {
            user->samesite_all_store = 0;
        }
        if (!strcasecmp(word[1], "user")) {
            write_user(user, "Enter the name of the user to be checked against: ");
            user->misc_op = 12;
            return;
        }
        if (!strcasecmp(word[1], "site")) {
            write_user(user,
                    "~OL~FRNOTE:~RS Wildcards \"*\" and \"?\" can be used.\n");
            write_user(user, "Enter the site to be checked against: ");
            user->misc_op = 13;
            return;
        }
        write_user(user, "Usage: samesite user|site [all]\n");
        return;
    }
    /* check for users of same site - user supplied */
    if (stage == 1) {
        /* check just those logged on */
        if (!user->samesite_all_store) {
            found = cnt = same = 0;
            u = get_user(user->samesite_check_store);
            if (!u) {
                write_user(user, notloggedon);
                return;
            }
            for (u_loop = user_first; u_loop; u_loop = u_loop->next) {
                ++cnt;
                if (u_loop == u) {
                    continue;
                }
                if (!strcmp(u->site, u_loop->site)) {
                    ++same;
                    if (!found++) {
                        vwrite_user(user,
                                "\n~BB~FG*** Users logged on from the same site as ~OL%s~RS~BB~FG ***\n\n",
                                u->name);
                    }
                    sprintf(text, "    %s %s\n", u_loop->name, u_loop->desc);
                    if (u_loop->type == REMOTE_TYPE) {
                        text[2] = '@';
                    }
                    if (!u_loop->vis) {
                        text[3] = '*';
                    }
                    write_user(user, text);
                }
            }
            if (!found) {
                vwrite_user(user,
                        "No users currently logged on have the same site as %s.\n",
                        u->name);
            } else {
                vwrite_user(user,
                        "\nChecked ~FM~OL%d~RS users, ~FM~OL%d~RS had the site as ~FG~OL%s~RS ~FG(%s)\n\n",
                        cnt, same, u->name, u->site);
            }
            return;
        }
        /* check all the users..  First, load the name given */
        u = get_user(user->samesite_check_store);
        on = !!u;
        if (!on) {
            u = create_user();
            if (!u) {
                vwrite_user(user, "%s: unable to create temporary user session.\n",
                        syserror);
                write_syslog(SYSLOG | ERRLOG, 0,
                        "ERROR: Unable to create temporary user session in samesite() - stage 1 of all.\n");
                return;
            }
            strcpy(u->name, user->samesite_check_store);
            if (!load_user_details(u)) {
                destruct_user(u);
                destructed = 0;
                vwrite_user(user, "Sorry, unable to load user file for %s.\n",
                        user->samesite_check_store);
                write_syslog(SYSLOG | ERRLOG, 0,
                        "ERROR: Unable to load user details in samesite() - stage 1 of all.\n");
                return;
            }
        }
        /* read userlist and check against all users */
        found = cnt = same = 0;
        for (entry = first_user_entry; entry; entry = entry->next) {
            *entry->name = toupper(*entry->name); /* just in case */
            /* create a user object if user not already logged on */
            u_loop = create_user();
            if (!u_loop) {
                write_syslog(SYSLOG | ERRLOG, 0,
                        "ERROR: Unable to create temporary user session in samesite().\n");
                continue;
            }
            strcpy(u_loop->name, entry->name);
            if (!load_user_details(u_loop)) {
                destruct_user(u_loop);
                destructed = 0;
                continue;
            }
            ++cnt;
            if ((on && !strcmp(u->site, u_loop->last_site))
                    || (!on && !strcmp(u->last_site, u_loop->last_site))) {
                ++same;
                if (!found++) {
                    vwrite_user(user,
                            "\n~BB~FG*** All users from the same site as ~OL%s~RS~BB~FG ***\n\n",
                            u->name);
                }
                vwrite_user(user, "    %s %s\n", u_loop->name, u_loop->desc);
                destruct_user(u_loop);
                destructed = 0;
                continue;
            }
            destruct_user(u_loop);
            destructed = 0;
        }
        if (!found) {
            vwrite_user(user, "No users have the same site as %s.\n", u->name);
        } else {
            if (!on) {
                vwrite_user(user,
                        "\nChecked ~FM~OL%d~RS users, ~FM~OL%d~RS had the site as ~FG~OL%s~RS ~FG(%s)\n\n",
                        cnt, same, u->name, u->last_site);
            } else {
                vwrite_user(user,
                        "\nChecked ~FM~OL%d~RS users, ~FM~OL%d~RS had the site as ~FG~OL%s~RS ~FG(%s)\n\n",
                        cnt, same, u->name, u->site);
            }
        }
        if (!on) {
            destruct_user(u);
            destructed = 0;
        }
        return;
    }
    /* check for users of same site - site supplied */
    if (stage == 2) {
        /* check any wildcards are correct */
        if (strstr(user->samesite_check_store, "**")) {
            write_user(user, "You cannot have ** in your site to check.\n");
            return;
        }
        if (strstr(user->samesite_check_store, "?*")) {
            write_user(user, "You cannot have ?* in your site to check.\n");
            return;
        }
        if (strstr(user->samesite_check_store, "*?")) {
            write_user(user, "You cannot have *? in your site to check.\n");
            return;
        }
        /* check just those logged on */
        if (!user->samesite_all_store) {
            found = cnt = same = 0;
            for (u = user_first; u; u = u->next) {
                ++cnt;
                if (!pattern_match(u->site, user->samesite_check_store)) {
                    continue;
                }
                ++same;
                if (!found++) {
                    vwrite_user(user,
                            "\n~BB~FG*** Users logged on from the same site as ~OL%s~RS~BB~FG ***\n\n",
                            user->samesite_check_store);
                }
                sprintf(text, "    %s %s\n", u->name, u->desc);
                if (u->type == REMOTE_TYPE) {
                    text[2] = '@';
                }
                if (!u->vis) {
                    text[3] = '*';
                }
                write_user(user, text);
            }
            if (!found) {
                vwrite_user(user,
                        "No users currently logged on have that same site as %s.\n",
                        user->samesite_check_store);
            } else {
                vwrite_user(user,
                        "\nChecked ~FM~OL%d~RS users, ~FM~OL%d~RS had the site as ~FG~OL%s\n\n",
                        cnt, same, user->samesite_check_store);
            }
            return;
        }
        /* check all the users.. */
        /* open userlist to check against all users */
        found = cnt = same = 0;
        for (entry = first_user_entry; entry; entry = entry->next) {
            *entry->name = toupper(*entry->name);
            /* create a user object if user not already logged on */
            u_loop = create_user();
            if (!u_loop) {
                write_syslog(SYSLOG | ERRLOG, 0,
                        "ERROR: Unable to create temporary user session in samesite().\n");
                continue;
            }
            strcpy(u_loop->name, entry->name);
            if (!load_user_details(u_loop)) {
                destruct_user(u_loop);
                destructed = 0;
                continue;
            }
            ++cnt;
            if ((pattern_match(u_loop->last_site, user->samesite_check_store))) {
                ++same;
                if (!found++) {
                    vwrite_user(user,
                            "\n~BB~FG*** All users that have the site ~OL%s~RS~BB~FG ***\n\n",
                            user->samesite_check_store);
                }
                vwrite_user(user, "    %s %s\n", u_loop->name, u_loop->desc);
            }
            destruct_user(u_loop);
            destructed = 0;
        }
        if (!found) {
            vwrite_user(user, "No users have the same site as %s.\n",
                    user->samesite_check_store);
        } else {
            if (!on) {
                vwrite_user(user,
                        "\nChecked ~FM~OL%d~RS users, ~FM~OL%d~RS had the site as ~FG~OL%s\n\n",
                        cnt, same, user->samesite_check_store);
            } else {
                vwrite_user(user,
                        "\nChecked ~FM~OL%d~RS users, ~FM~OL%d~RS had the site as ~FG~OL%s\n\n",
                        cnt, same, user->samesite_check_store);
            }
        }
        return;
    }
}

