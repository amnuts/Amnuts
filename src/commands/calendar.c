#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * display the calendar to the user
 */
void
show_calendar(UR_OBJECT user)
{
    static const char *const short_month_name[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct",
        "Nov", "Dec"
    };
    static const char *const short_day_name[] = {
        "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
    };
    static const int num_of_days[] = {
        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
    };
    char temp[ARR_SIZE];
    const struct tm *date;
    time_t now;
    int dow, numdays;
    int yr, mo, dy;

    time(&now);
    date = localtime(&now);
    yr = word_count < 3 ? 1900 + date->tm_year : atoi(word[2]);
    mo = word_count < 2 ? 1 + date->tm_mon : atoi(word[1]);
    if (yr == 99) {
        yr += 1900;
    } else if (yr < 99) {
        yr += 2000;
    }
    if (word_count > 3 || yr > 3000 || yr < 1800 || mo < 1 || mo > 12) {
        write_user(user, "Usage: calendar [<m> [<y>]]\n");
        write_user(user, "where: <m> = month from 1 to 12\n");
        write_user(user, "       <y> = year from 1 to 99, or 1800 to 3000\n");
        return;
    }
    /* show calendar */
    write_user(user, "\n+-----------------------------------+\n");
    write_user(user,
            align_string(ALIGN_CENTRE, 37, 1, "|", "~OL~FC%.4d %s~RS", yr,
            short_month_name[mo - 1]));
    write_user(user, "+-----------------------------------+\n");
    *temp = '\0';
    *text = '\0';
    for (dow = 0; dow < 7; ++dow) {
#ifdef ISOWEEKS
        sprintf(temp, "  ~OL~FY%s~RS", short_day_name[(1 + dow) % 7]);
#else
        sprintf(temp, "  ~OL~FY%s~RS", short_day_name[dow]);
#endif
        strcat(text, temp);
    }
    strcat(text, "\n+-----------------------------------+\n");
    numdays = num_of_days[mo - 1];
    if (2 == mo && is_leap(yr)) {
        ++numdays;
    }
#ifdef ISOWEEKS
    dow = (ymd_to_scalar(yr, mo, 1) - 1) % 7L;
#else
    dow = ymd_to_scalar(yr, mo, 1) % 7L;
#endif
    for (dy = 0; dy < dow; ++dy) {
        strcat(text, "     ");
    }
    for (dy = 1; dy <= numdays; ++dy, ++dow, dow %= 7) {
        if (!dow && 1 != dy) {
            strcat(text, "\n\n");
        }
        if (has_reminder(user, dy, mo, yr)) {
            sprintf(temp, " ~OL~FR%3d~RS ", dy);
            strcat(text, temp);
        } else if (is_ymd_today(yr, mo, dy)) {
            sprintf(temp, " ~OL~FG%3d~RS ", dy);
            strcat(text, temp);
        } else {
            sprintf(temp, " %3d ", dy);
            strcat(text, temp);
        }
    }
    for (; dow; ++dow, dow %= 7) {
        strcat(text, "      ");
    }
    strcat(text, "\n+-----------------------------------+\n\n");
    write_user(user, text);
}
