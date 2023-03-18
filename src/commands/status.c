
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Show some user stats
 */
void
status(UR_OBJECT user)
{
    char ir[ROOM_NAME_LEN + 1], text2[ARR_SIZE], text3[ARR_SIZE], rm[4],
            qcall[USER_NAME_LEN];
    char email[82], nm[5], muzlev[20], arrlev[20];
    UR_OBJECT u;
    int days, hours, mins, hs, on, cnt, newmail;
    time_t t_expire;

    if (word_count < 2) {
        u = user;
        on = 1;
    } else {
        u = retrieve_user(user, word[1]);
        if (!u) {
            return;
        }
        on = retrieve_user_type == 1;
    }
    write_user(user, "\n\n");
    if (!on) {
        write_user(user,
                "+----- ~FCUser Info~RS -- ~FB(not currently logged on)~RS -------------------------------+\n");
    } else {
        write_user(user,
                "+----- ~FCUser Info~RS -- ~OL~FY(currently logged on)~RS -----------------------------------+\n");
    }
    sprintf(text2, "%s~RS %s", u->recap, u->desc);
    cnt = 45 + teslen(text2, 45);
    vwrite_user(user, "Name   : %-*.*s~RS Level : %s\n", cnt, cnt, text2,
            user_level[u->level].name);
    mins = (int) (time(0) - u->last_login) / 60;
    days = u->total_login / 86400;
    hours = (u->total_login % 86400) / 3600;
    if (!u->invite_room) {
        strcpy(ir, "<nowhere>");
    } else {
        strcpy(ir, u->invite_room->name);
    }
    if (!u->age) {
        sprintf(text2, "Unknown");
    } else {
        sprintf(text2, "%d", u->age);
    }
    if (!on || u->login) {
        *text3 = '\0';
    } else {
        sprintf(text3, "            Online for : %d min%s", mins, PLTEXT_S(mins));
    }
    vwrite_user(user, "Gender : %-8s      Age : %-8s %s\n", sex[u->gender],
            text2, text3);
    if (!*u->email) {
        strcpy(email, "Currently unset");
    } else {
        strcpy(email, u->email);
        if (u->mail_verified) {
            strcat(email, " ~FB~OL(VERIFIED)~RS");
        }
        if (u->hideemail) {
            if (user->level >= WIZ || u == user) {
                strcat(email, " ~FB~OL(HIDDEN)~RS");
            } else {
                strcpy(email, "Currently only on view to the Wizzes");
            }
        }
    }
    vwrite_user(user, "Email Address : %s\n", email);
    vwrite_user(user, "Homepage URL  : %s\n",
            !*u->homepage ? "Currently unset" : u->homepage);
    vwrite_user(user, "ICQ Number    : %s\n",
            !*u->icq ? "Currently unset" : u->icq);
    mins = (u->total_login % 3600) / 60;
    vwrite_user(user,
            "Total Logins  : %-9d  Total login : %d day%s, %d hour%s, %d minute%s\n",
            u->logons, days, PLTEXT_S(days), hours, PLTEXT_S(hours), mins,
            PLTEXT_S(mins));
    write_user(user,
            "+----- ~FCGeneral Info~RS ---------------------------------------------------------+\n");
    vwrite_user(user, "Enter Msg     : %s~RS %s\n", u->recap, u->in_phrase);
    vwrite_user(user, "Exit Msg      : %s~RS %s~RS to the...\n", u->recap,
            u->out_phrase);
    newmail = mail_sizes(u->name, 1);
    if (!newmail) {
        sprintf(nm, "NO");
    } else {
        sprintf(nm, "%d", newmail);
    }
    /* FIXME: Use sentinel other JAILED */
    if (!on || u->login) {
        vwrite_user(user, "New Mail      : %-13.13s  Muzzled : %-13.13s\n", nm,
                noyes[(u->muzzled != JAILED)]);
    } else {
        vwrite_user(user,
                "Invited to    : %-13.13s  Muzzled : %-13.13s  Ignoring : %-13.13s\n",
                ir, noyes[(u->muzzled != JAILED)], noyes[u->ignall]);
#ifdef NETLINKS
        if (u->type == REMOTE_TYPE || !u->room) {
            hs = 0;
            sprintf(ir, "<off site>");
        } else {
#endif
            hs = 1;
            sprintf(ir, "%s", u->room->name);
#ifdef NETLINKS
        }
#endif
        vwrite_user(user,
                "In Area       : %-13.13s  At home : %-13.13s  New Mail : %-13.13s\n",
                ir, noyes[hs], nm);
    }
    vwrite_user(user,
            "Killed %d people, and died %d times.  Energy : %d, Bullets : %d\n",
            u->kills, u->deaths, u->hps, u->bullets);
    if (u == user || user->level >= WIZ) {
        write_user(user,
                "+----- ~FCUser Only Info~RS -------------------------------------------------------+\n");
        vwrite_user(user,
                "Char echo     : %-13.13s  Wrap    : %-13.13s  Monitor  : %-13.13s\n",
                noyes[u->charmode_echo], noyes[u->wrap], noyes[u->monitor]);
        if (u->lroom == 2) {
            strcpy(rm, "YES");
        } else {
            strcpy(rm, noyes[u->lroom]);
        }
        vwrite_user(user,
                "Colours       : %-13.13s  Pager   : %-13d  Logon rm : %-13.13s\n",
                noyes[u->colour], u->pager, rm);
        if (!*u->call) {
            strcpy(qcall, "<no one>");
        } else {
            strcpy(qcall, u->call);
        }
        vwrite_user(user,
                "Quick call to : %-13.13s  Autofwd : %-13.13s  Verified : %-13.13s\n",
                qcall, noyes[u->autofwd], noyes[u->mail_verified]);
        if (on && !u->login) {
            if (u == user && user->level < WIZ) {
                vwrite_user(user, "On from site  : %s\n", u->site);
            } else {
                vwrite_user(user, "On from site  : %-42.42s  Port : %s\n", u->site,
                        u->site_port);
            }
        }
    }
    if (user->level >= WIZ) {
        write_user(user,
                "+----- ~OL~FCWiz Only Info~RS --------------------------------------------------------+\n");
        /* FIXME: Use sentinel other JAILED */
        if (u->muzzled == JAILED) {
            strcpy(muzlev, "Unmuzzled");
        } else {
            strcpy(muzlev, user_level[u->muzzled].name);
        }
        /* FIXME: Use sentinel other JAILED */
        if (u->arrestby == JAILED) {
            strcpy(arrlev, "Unarrested");
        } else {
            strcpy(arrlev, user_level[u->arrestby].name);
        }
        vwrite_user(user,
                "Unarrest Lev  : %-13.13s  Arr lev : %-13.13s  Muz Lev  : %-13.13s\n",
                user_level[u->unarrest].name, arrlev, muzlev);
        if (u->lroom == 2) {
            sprintf(rm, "YES");
        } else {
            sprintf(rm, "NO");
        }
        vwrite_user(user, "Logon room    : %-38.38s  Shackled : %s\n",
                u->logout_room, rm);
        vwrite_user(user, "Last site     : %s\n", u->last_site);
        t_expire =
                u->last_login + 86400 * (u->level ==
                NEW ? NEWBIE_EXPIRES : USER_EXPIRES);
        strftime(text2, ARR_SIZE, "%a %Y-%m-%d %H:%M:%S", localtime(&t_expire));
        vwrite_user(user, "User Expires  : %-13.13s  On date : %s\n",
                noyes[u->expire], text2);
    }
    write_user(user,
            "+----------------------------------------------------------------------------+\n\n");
    if (u != user) {
        done_retrieve(u);
    }
}
