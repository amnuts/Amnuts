#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Display the reminders a user has to them
 * login is used to show information at the login prompt, otherwise user
 * is just using the .reminder command.  stage is used for inputting of a new reminder
 */
void
show_reminders(UR_OBJECT user, int stage)
{
    char temp[ARR_SIZE];
    const struct tm *date;
    time_t now;
    int yr, mo, dy;
    int dd, mm, yy;
    int i, total, count, del;

    time(&now);
    date = localtime(&now);
    /* display manually */
    if (!stage) {
        if (word_count < 2) {
            write_user(user,
                    "~OLTo view:\nUsage: reminder all\n       reminder today\n");
            write_user(user, "       reminder <d> [<m> [<y>]]\n");
            write_user(user,
                    "~OLTo manage:\nUsage: reminder set\n       reminder del <number>\n");
            return;
        }
        /* display all the reminders a user has set */
        if (!strcasecmp("all", word[1])) {
            write_user(user,
                    "+----------------------------------------------------------------------------+\n");
            write_user(user,
                    "| ~OL~FCAll your reminders~RS                                                         |\n");
            write_user(user,
                    "+----------------------------------------------------------------------------+\n\n");
            total = 0;
            count = 0;
            for (i = 0; i < MAX_REMINDERS; ++i) {
                /* no msg set, then no reminder */
                if (!*user->reminder[i].msg) {
                    continue;
                }
                ++total;
                scalar_to_ymd(user->reminder[i].date, &yy, &mm, &dd);
                ++count;
                vwrite_user(user, "~OL%2d)~RS ~OL~FC%.4d-%.2d-%.2d~RS %s\n", total,
                        yy, mm, dd, user->reminder[i].msg);
            }
            if (!count) {
                write_user(user, "You do not have reminders set.\n");
            }
            write_user(user,
                    "\n+----------------------------------------------------------------------------+\n\n");
            return;
        }
        /* display all the reminders a user has for today */
        if (!strcasecmp("today", word[1])) {
            dy = date->tm_mday;
            mo = 1 + date->tm_mon;
            yr = 1900 + date->tm_year;
            write_user(user,
                    "+----------------------------------------------------------------------------+\n");
            write_user(user,
                    "| ~OL~FCYour reminders for today are~RS                                               |\n");
            write_user(user,
                    "+----------------------------------------------------------------------------+\n\n");
            total = 0;
            count = 0;
            for (i = 0; i < MAX_REMINDERS; ++i) {
                if (!*user->reminder[i].msg) {
                    continue;
                }
                ++total;
                scalar_to_ymd(user->reminder[i].date, &yy, &mm, &dd);
                if (dd == dy && mm == mo && yy == yr) {
                    ++count;
                    vwrite_user(user, "~OL%2d)~RS ~OL~FC%.4d-%.2d-%.2d~RS %s\n", total,
                            yy, mm, dd, user->reminder[i].msg);
                }
            }
            if (!count) {
                write_user(user, "You do not have reminders set for today.\n");
            }
            write_user(user,
                    "\n+----------------------------------------------------------------------------+\n\n");
            return;
        }
        /* allow a user to set a reminder */
        if (!strcasecmp("set", word[1])) {
            /* check to see if there is enough space to add another reminder */
            for (i = 0; i < MAX_REMINDERS; ++i) {
                if (!*user->reminder[i].msg) {
                    break;
                }
            }
            if (i >= MAX_REMINDERS) {
                write_user(user,
                        "You already have the maximum amount of reminders set.\n");
                return;
            }
            user->reminder_pos = i;
            write_user(user, "Please enter a date for the reminder (1-31): ");
            user->misc_op = 20;
            return;
        }
        /* allow a user to delete one of their reminders */
        if (!strcasecmp("del", word[1])) {
            if (word_count < 3) {
                write_user(user, "Usage: reminder del <number>\n");
                write_user(user,
                        "where: <number> can be taken from \"reminder all\"\n");
                return;
            }
            del = atoi(word[2]);
            total = 0;
            for (i = 0; i < MAX_REMINDERS; ++i) {
                if (!*user->reminder[i].msg) {
                    continue;
                }
                ++total;
                if (total == del) {
                    break;
                }
            }
            if (i >= MAX_REMINDERS) {
                vwrite_user(user,
                        "Sorry, could not delete reminder number ~OL%d~RS.\n",
                        del);
                return;
            }
            user->reminder[i].date = -1;
            *user->reminder[i].msg = '\0';
            vwrite_user(user, "You have now deleted reminder number ~OL%d~RS.\n",
                    del);
            write_user_reminders(user);
            return;
        }
        /* view reminders for a particular day */
        yr = word_count < 4 ? 1900 + date->tm_year : atoi(word[3]);
        mo = word_count < 3 ? 1 + date->tm_mon : atoi(word[2]);
        dy = word_count < 2 ? date->tm_mday : atoi(word[1]);
        /* assume that year give xx is y2k if xx!=99 */
        if (yr == 99) {
            yr += 1900;
        } else if (yr < 99) {
            yr += 2000;
        }
        if (word_count > 4 || yr > 3000 || yr < 1800 || mo < 1 || mo > 12
                || dy < 1 || dy > 31) {
            write_user(user, "Usage: reminder <d> [<m> [<y>]]\n");
            write_user(user, "where: <d> = day from 1 to 31\n");
            write_user(user, "       <m> = month from 1 to 12\n");
            write_user(user, "       <y> = year from 1 to 99, or 1800 to 3000\n");
            return;
        }
        write_user(user,
                "+----------------------------------------------------------------------------+\n");
        sprintf(temp, "Your reminders for %.4d-%.2d-%.2d", yr, mo, dy);
        vwrite_user(user, "| ~OL~FC%-74.74s~RS |\n", temp);
        write_user(user,
                "+----------------------------------------------------------------------------+\n\n");
        total = 0;
        count = 0;
        for (i = 0; i < MAX_REMINDERS; ++i) {
            if (!*user->reminder[i].msg) {
                continue;
            }
            ++total;
            scalar_to_ymd(user->reminder[i].date, &yy, &mm, &dd);
            if (dd == dy && mm == mo && yy == yr) {
                ++count;
                vwrite_user(user, "~OL%2d)~RS ~OL~FC%.4d-%.2d-%.2d~RS %s\n", total,
                        yy, mm, dd, user->reminder[i].msg);
            }
        }
        if (!count) {
            vwrite_user(user, "You have no reminders set for %.4d-%.2d-%.2d\n", yr,
                    mo, dy);
        }
        write_user(user,
                "\n+----------------------------------------------------------------------------+\n\n");
        return;
    }
    /* next stages of asking for a new reminder */
    switch (stage) {
    case 1:
        yy = 1900 + date->tm_year;
        mm = 1 + date->tm_mon;
        dd = !word_count ? -1 : atoi(word[0]);
        if (dd < 1 || dd > 31) {
            write_user(user,
                    "The day for the reminder must be between 1 and 31.\n");
            user->reminder_pos = -1;
            user->misc_op = 0;
            return;
        }
        write_user(user, "Please enter a month (1-12): ");
        user->reminder[user->reminder_pos].date = ymd_to_scalar(yy, mm, dd);
        user->misc_op = 21;
        return;
    case 2:
        scalar_to_ymd(user->reminder[user->reminder_pos].date, &yy, &mm, &dd);
        mm = !word_count ? -1 : atoi(word[0]);
        if (mm < 1 || mm > 12) {
            write_user(user,
                    "The month for the reminder must be between 1 and 12.\n");
            user->reminder[user->reminder_pos].date = -1;
            user->reminder_pos = -1;
            user->misc_op = 0;
            return;
        }
        write_user(user,
                "Please enter a year (xx or 19xx or 20xx, etc, <return> for this year): ");
        user->reminder[user->reminder_pos].date = ymd_to_scalar(yy, mm, dd);
        user->misc_op = 22;
        return;
    case 3:
        scalar_to_ymd(user->reminder[user->reminder_pos].date, &yy, &mm, &dd);
        yy = !word_count ? -1 : atoi(word[0]);
        if (yy == -1) {
            yy = 1900 + date->tm_year;
        }
        /* assume that year give xx is y2k if xx!=99 */
        if (yy == 99) {
            yy += 1900;
        } else if (yy < 99) {
            yy += 2000;
        }
        if (yy > 3000 || yy < 1800) {
            write_user(user,
                    "The year for the reminder must be between 1-99 or 1800-3000.\n");
            user->reminder[user->reminder_pos].date = -1;
            user->reminder_pos = -1;
            user->misc_op = 0;
            return;
        }
        /* check to see if date has past - why put in a remind for a date that has? */
        user->reminder[user->reminder_pos].date = ymd_to_scalar(yy, mm, dd);
        if (user->reminder[user->reminder_pos].date <
                ymd_to_scalar(1900 + date->tm_year, 1 + date->tm_mon,
                date->tm_mday)) {
            write_user(user,
                    "That date has already passed so there is no point setting a reminder for it.\n");
            user->reminder[user->reminder_pos].date = -1;
            user->reminder_pos = -1;
            user->misc_op = 0;
            return;
        }
        write_user(user, "Please enter reminder message:\n~FG>>~RS");
        user->misc_op = 23;
        return;
    case 4:
        /* tell them they MUST enter a message */
        if (!*user->reminder[user->reminder_pos].msg) {
            write_user(user, "Please enter reminder message:\n~FG>>~RS");
            user->misc_op = 23;
            return;
        }
        write_user(user, "You have set the following reminder:\n");
        total = 0;
        for (i = 0; i < MAX_REMINDERS; ++i) {
            if (!*user->reminder[i].msg) {
                continue;
            }
            ++total;
            if (i == user->reminder_pos) {
                break;
            }
        }
        user->reminder_pos = -1;
        scalar_to_ymd(user->reminder[i].date, &yy, &mm, &dd);
        vwrite_user(user, "~OL%2d)~RS ~OL~FC%.4d-%.2d-%.2d~RS %s\n", total, yy,
                mm, dd, user->reminder[i].msg);
        write_user_reminders(user);
        user->misc_op = 0;
        return;
    }
}
