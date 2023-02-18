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
 * Attempt to get '\n' terminated line of input from a character
 * mode client else store data read so far in user buffer.
 */
int
get_charclient_line(UR_OBJECT user, char *inpstr, int len)
{
    size_t l;

    for (l = 0; l < (size_t) len; ++l) {
        /* see if delete entered */
        if (inpstr[l] == '\b' || inpstr[l] == '\x7f') {
            if (user->buffpos) {
                --user->buffpos;
                if (user->charmode_echo) {
                    write_user(user, "\b \b");
                }
            }
            continue;
        }
        user->buff[user->buffpos] = inpstr[l];
        /* See if end of line */
        if (iscntrl(inpstr[l]) || user->buffpos + 2 == ARR_SIZE) {
            terminate(user->buff);
            strcpy(inpstr, user->buff);
            if (user->charmode_echo) {
                write_user(user, "\n");
            }
            return 1;
        }
        ++user->buffpos;
    }
    if (user->charmode_echo
            && ((user->login != LOGIN_PASSWD && user->login != LOGIN_CONFIRM)
            || user->show_pass))
        send(user->socket, inpstr, l, 0);
    return 0;
}

/*
 * Terminate string at first control character
 */
void
terminate(char *str)
{
    size_t i;

    for (i = 0; i < ARR_SIZE; ++i) {
        if (iscntrl(str[i])) {
            break;
        }
    }
    if (i < ARR_SIZE) {
        str[i] = '\0';
        return;
    }
    str[ARR_SIZE - 1] = '\0';
}

/*
 * Get words from sentence. This function prevents the words in the
 * sentence from writing off the end of a word array element. This is
 * difficult to do with sscanf() hence I use this function instead.
 */
int
wordfind(const char *inpstr)
{
    const char *str;
    size_t wcnt, wpos;

    str = inpstr;
    for (wcnt = 0; wcnt < MAX_WORDS; ++wcnt) {
        for (; *str; ++str) {
            if (!isspace(*str)) {
                break;
            }
        }
        if (!*str) {
            break;
        }
        for (wpos = 0; wpos < WORD_LEN; ++wpos) {
            if (!*str || isspace(*str)) {
                break;
            }
            word[wcnt][wpos] = *str++;
        }
        word[wcnt][wpos] = '\0';
        for (; *str; ++str) {
            if (isspace(*str)) {
                break;
            }
        }
    }
    return wcnt;
}

/*
 * clear word array etc.
 */
void
clear_words(void)
{
    size_t w;

    for (w = 0; w < MAX_WORDS; ++w) {
        *word[w] = '\0';
    }
    word_count = 0;
}

/*
 * check to see if string given is YES or NO
 */
int
yn_check(const char *wd)
{
    if (!strcmp(wd, "YES")) {
        return 1;
    }
    if (!strcmp(wd, "NO")) {
        return 0;
    }
    return -1;
}

/*
 * check to see if string given is ON or OFF
 */
int
onoff_check(const char *wd)
{
    if (!strcmp(wd, "ON")) {
        return 1;
    }
    if (!strcmp(wd, "OFF")) {
        return 0;
    }
    return -1;
}

/*
 * check to see if string given is OFF, MIN or MAX
 */
int
minmax_check(const char *wd)
{
    if (!strcmp(wd, "OFF")) {
        return SBOFF;
    }
    if (!strcmp(wd, "MIN")) {
        return SBMIN;
    }
    if (!strcmp(wd, "MAX")) {
        return SBMAX;
    }
    return -1;
}

/*
 * check to see if string given is OFF, AUTO, MANUAL
 */
int
resolve_check(const char *wd)
{
    if (!strcmp(wd, "OFF")) {
        return 0;
    }
    if (!strcmp(wd, "AUTO")) {
        return 1;
    }
    if (!strcmp(wd, "MANUAL")) {
        return 2;
    }
    if (!strcmp(wd, "IDENTD")) {
        return 3;
    }
    return -1;
}

/*
 * Tell telnet not to echo characters - for password entry
 */
void
echo_off(UR_OBJECT user)
{
    if (user->show_pass) {
        return;
    }
    vwrite_user(user, "%c%c%c", '\xff', '\xfb', '\x01');
}

/*
 * Tell telnet to echo characters
 */
void
echo_on(UR_OBJECT user)
{
    if (user->show_pass) {
        return;
    }
    vwrite_user(user, "%c%c%c", '\xff', '\xfc', '\x01');
}

/*
 * Return position of second word in the given string
 */
char *
remove_first(char *inpstr)
{
    char *str;

    str = inpstr;
    for (; *str; ++str) {
        if (!isspace(*str)) {
            break;
        }
    }
    for (; *str; ++str) {
        if (isspace(*str)) {
            break;
        }
    }
    for (; *str; ++str) {
        if (!isspace(*str)) {
            break;
        }
    }
    return str;
}

/*
 * See if string contains any swearing
 */
int
contains_swearing(const char *str)
{
    char *s;
    int i;

    s = (char *) malloc(1 + strlen(str));
    if (!s) {
        write_syslog(SYSLOG, 0,
                "ERROR: Failed to allocate memory in contains_swearing().\n");
        return 0;
    }
    strcpy(s, str);
    strtolower(s);
    for (i = 0; swear_words[i]; ++i) {
        if (strstr(s, swear_words[i])) {
            break;
        }
    }
    memset(s, 0, 1 + strlen(s));
    free(s);
    return !!swear_words[i];
}

/*
 * go through the given string and replace any of the words found in the
 * swear_words array with the default swear censor, *swear_censor
 */
char *
censor_swear_words(char *has_swears)
{
    int i;
    char *clean;

    clean = NULL;
    for (i = 0; swear_words[i]; ++i) {
        while (has_swears) {
            clean = has_swears;
            has_swears = replace_string(clean, swear_words[i], swear_censor);
        }
        has_swears = clean;
    }
    return clean;
}

/*
 * Strip out colour commands from string for when we are sending strings
 * over a netlink to a talker that does not support them
 */
char *
colour_com_strip(const char *str)
{
    static char text2[ARR_SIZE];
    const char *s;
    char *t;
    size_t i;

    t = text2;
    for (s = str; *s; ++s) {
        if (*s == '~') {
            for (i = 0; colour_codes[i].txt_code; ++i) {
                if (!strncmp(s + 1, colour_codes[i].txt_code,
                        strlen(colour_codes[i].txt_code))) {
                    break;
                }
            }
            if (colour_codes[i].txt_code) {
                s += strlen(colour_codes[i].txt_code);
                continue;
            }
            if (s[1] == '~') {
                ++s;
            }
        } else if (*s == '^') {
            if (s[1] == '~') {
                ++s;
            }
        }
        if (t >= text2 + ARR_SIZE - 1) {
            break;
        }
        *t++ = *s;
    }
    *t = '\0';
    return text2;
}

/*
 * Convert string to upper case
 */
void
strtoupper(char *str)
{
    for (; *str; ++str) {
        *str = toupper(*str);
    }
}

/*
 * Convert string to lower case
 */
void
strtolower(char *str)
{
    for (; *str; ++str) {
        *str = tolower(*str);
    }
}

/*
 * Convert string to name (first char is upper, rest is lower)
 */
void
strtoname(char *str)
{
    *str = toupper(*str);
    ++str;
    for (; *str; ++str) {
        *str = tolower(*str);
    }
}

/*
 * Returns 1 if string is a positive number
 */
int
is_number(const char *str)
{
    for (; *str; ++str) {
        if (!isdigit(*str)) {
            break;
        }
    }
    return !*str;
}

/*
 * Performs the same as strstr, in that it returns a pointer to the
 * first occurrence of pat in str--except that this is performed
 * case-insensitive
 */
char *
istrstr(char *str, const char *pat)
{
    const char *pptr;
    char *sptr, *start;
    size_t slen, plen;

    slen = strlen(str);
    plen = strlen(pat);
    for (start = str, pptr = pat; slen >= plen; ++start, --slen) {
        /* find start of pattern in string */
        while (toupper(*start) != toupper(*pat)) {
            ++start;
            --slen;
            /* if pattern longer than string */
            if (slen < plen) {
                return NULL;
            }
        }
        sptr = start;
        pptr = pat;
        while (toupper(*sptr) == toupper(*pptr)) {
            ++sptr;
            ++pptr;
            /* if end of pattern then pattern was found */
            if (!*pptr) {
                return start;
            }
        }
    }
    return NULL;
}

/*
 * Take the string "inpstr" and replace any occurrence of "old_str" with
 * the string "new_str"
 */
char *
replace_string(char *inpstr, const char *old_str, const char *new_str)
{
    size_t old_len, new_len;
    char *x, *y;

    x = istrstr(inpstr, old_str);
    if (!x) {
        return x;
    }
    old_len = strlen(old_str);
    new_len = strlen(new_str);
    y = x + new_len;
    memmove(y, x + old_len, strlen(x + old_len) + 1);
    memcpy(x, new_str, new_len);
    return inpstr;
}

/*
 * Take a string and repeat is the given number of times
 */
char *
repeat_string(const char *string, int times)
{
    size_t slen = strlen(string);
    char *dest = malloc(times*slen+1);

    int i; char *p;
    for ( i=0, p = dest; i < times; ++i, p += slen ) {
        memcpy(p, string, slen);
    }
    *p = '\0';
    return dest;
}

/*
 * Get ordinal value of a date and return the string
 */
const char *
ordinal_text(int num)
{
    static const char *const ords[] = {"th", "st", "nd", "rd"};

    num %= 100;
    if (num >= 10 && num < 20) {
        num = 0;
    }
    num %= 10;
    if (num >= 4) {
        num = 0;
    }
    return ords[num];
}

/*
 * Date string for board messages, mail, .who and .allclones, etc
 */
char *
long_date(int which)
{
    static const char *const full_month_name[] = {"January", "February", "March", "April", "May", "June", "July",
        "August", "September", "October", "November", "December"};
    static const char *const full_day_name[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday",
        "Saturday"};
    static char dstr[80];
    const struct tm *date;
    time_t now;
    int ap, hour;

    /* XXX: This is ugly; look at changing formats and using strftime()
     * Problems with using strftime() and the current formats include:
     * Day of month: cannot get it unpadded (zero or space)
     * Day of month: cannot get ordinal specifier (e.g., 1st, 2nd, 3rd, 4th, etc.)
     * Hour of Day: cannot get AM or PM value in lowercase
     * Recommend going to ISO 8601:2000 sortable date format: "%Y-%m-%d %H:%M:%S"
     */
    time(&now);
    date = localtime(&now);
    if (date->tm_hour >= 12) {
        hour = date->tm_hour > 12 ? date->tm_hour - 12 : 12;
        ap = 1;
    } else {
        hour = !date->tm_hour ? 12 : date->tm_hour;
        ap = 0;
    }
    if (which) {
        sprintf(dstr, "on %s %d%s %s %d at %.2d:%.2d%s",
                full_day_name[date->tm_wday], date->tm_mday,
                ordinal_text(date->tm_mday), full_month_name[date->tm_mon],
                (int) 1900 + date->tm_year, hour, (int) date->tm_min,
                !ap ? "am" : "pm");
    } else {
        sprintf(dstr, "[ %s %d%s %s %d at %.2d:%.2d%s ]",
                full_day_name[date->tm_wday], date->tm_mday,
                ordinal_text(date->tm_mday), full_month_name[date->tm_mon],
                (int) 1900 + date->tm_year, hour, (int) date->tm_min,
                !ap ? "am" : "pm");
    }
    return dstr;
}

/*
 * takes string str and determines what smiley type it should have.  The type is then
 * stored in "type".  The smiley type is determined by the last 2 characters in str.
 */
const char *
smiley_type(const char *str)
{

    struct emotions_struct {
        const char *name;
        const char *const *emoticons;
    };
    static const char *const glaze_emotions[] = {"8)", "(8", NULL};
    static const char *const wink_emotions[] = {";)", "(;", NULL};
    static const char *const frown_emotions[] = {");", ")=", "):", "=(", ":(", NULL};
    static const char *const smile_emotions[] = {"(=", "(:", "=)", ":)", NULL};
    static const char *const exclaim_emotions[] = {"!", NULL};
    static const char *const ask_emotions[] = {"?", NULL};
    static const struct emotions_struct emotions[] = {
        {"glaze", glaze_emotions},
        {"wink", wink_emotions},
        {"frown", frown_emotions},
        {"smile", smile_emotions},
        {"exclaim", exclaim_emotions},
        {"ask", ask_emotions},
        {NULL, NULL},
    };
    const char *const *emoticon;
    const struct emotions_struct *emotion;
    int len, elen;

    len = strlen(str);
    for (emotion = emotions; emotion->name; ++emotion) {
        for (emoticon = emotion->emoticons; *emoticon; ++emoticon) {
            elen = strlen(*emoticon);
            if (len >= elen && !strcmp(str + len - elen, *emoticon)) {
                break;
            }
        }
        if (*emoticon) {
            break;
        }
    }
    return emotion->name;
}

/*
 * This function allows you to align a string to the left, center or right.
 * returns to string length is bigger than format length
 * pos=0, align left.  pos=1, align centre.  pos=2, align right.
 * mark=0, no markers on the left and right sides.  mark=1, markers in place
 */
char *
align_string(int pos, int cstrlen, int mark, const char *marker, const char *str, ...)
{
    va_list args;
    char text2[ARR_SIZE * 2];
    char vtext[ARR_SIZE];
    int len = 0, spc = 0, odd = 0;

    /* first build up the string */
    *vtext = '\0';
    va_start(args, str);
    vsnprintf(vtext, sizeof(vtext), str, args);
    va_end(args);
    /* get size */
    len = strlen(vtext) - teslen(vtext, 0);
    spc = (int) ((cstrlen / 2) - (len / 2));
    odd = ((spc + spc + len) - (cstrlen));
    /* if greater than size given then do not do anything except return */
    if (len > cstrlen) {
        return strdup(vtext);
    }
    switch (pos) {
        case ALIGN_LEFT:
            snprintf(text2, sizeof(text2), "%s%*.*s", vtext, (spc * 2) - odd, (spc * 2) - odd, "");
            break;
        case ALIGN_CENTRE:
            snprintf(text2, sizeof(text2), "%*.*s%s%*.*s", spc, spc, "", vtext, spc - odd, spc - odd, "");
            break;
        case ALIGN_RIGHT:
            snprintf(text2, sizeof(text2), "%*.*s%s", (spc * 2) - odd, (spc * 2) - odd, "", vtext);
            break;
    }
    strcpy(vtext, text2);
    /* if marked, then add spaces on the other side too */
    if (mark) {
        /* if markers cannot be placed without over-writing text then return */
        if (len > (cstrlen - 2)) {
            return strdup(vtext);
        }
        /* if they forgot to pass a marker, use a default one */
        if (!marker) {
            marker = "|";
        }
        *vtext = *marker;
        int index = 0;
        if (strlen(vtext) > 0) {
            index = strlen(vtext) - 1;
        }
        vtext[index] = *marker;
    }
    strcat(vtext, "\n");
    return strdup(vtext);
}

/*
 * Check to see if the pattern "pat" appears in the string "str".
 * Uses recursion to achieve this
 */
int
pattern_match(char *str, char *pat)
{
    int i, slraw;

    /* if end of both, strings match */
    if (!*pat && !*str) {
        return 1;
    }
    if (!*pat) {
        return 0;
    }
    if (*pat == '*') {
        if (!pat[1]) {
            return 1;
        }
        for (i = 0, slraw = strlen(str); i <= slraw; ++i) {
            if (str[i] == pat[1] || pat[1] == '?') {
                if (pattern_match(str + i + 1, pat + 2) == 1) {
                    return 1;
                }
            }
        }
    } else {
        if (!*str) {
            return 0;
        }
        if (*pat == '?' || *pat == *str) {
            if (pattern_match(str + 1, pat + 1) == 1) {
                return 1;
            }
        }
    }
    return 0;
}

/*
 * check to see if a given email has the correct format
 */
int
validate_email(char *email)
{
    char *p;
    int subdomains;

    if (!*email) {
        return 0;
    }
    for (p = email; *p; ++p) {
        if (!isalnum(*p) && !strchr("._-", *p)) {
            break;
        }
    }
    if (*p != '@') {
        return 0;
    }
    subdomains = 0;
    for (++p; *p; ++p) {
        if (!isalpha(*p)) {
            break;
        }
        for (++p; *p; ++p) {
            if (!isalnum(*p) && !strchr("_-", *p)) {
                break;
            }
        }
        if (*p != '.') {
            break;
        }
        ++subdomains;
    }
    if (*p) {
        return 0;
    }
    return subdomains > 0;
}

/*
 * control what happens when a command shortcut is used.  Whether it needs
 * to have word recount, shortcut parsed, or whatever.
 */
char *
process_input_string(char *inpstr, CMD_OBJECT defaultcmd)
{
    CMD_OBJECT cmd;

    /* FIXME: Stupid special case "call tell" */
    if (*inpstr == ',') {
        if (!isspace(inpstr[1])) {
            split_command_string(inpstr);
        }
        strcpy(word[0], "tell");
        /* "," needs to remain in inpstr for quick call check/hack in the tell() */
        return inpstr;
    }
    for (cmd = first_command; cmd; cmd = cmd->next) {
        if (strchr(cmd->alias, *inpstr)) {
            break;
        }
    }
    if (cmd) {
        if (!isspace(inpstr[1])) {
            split_command_string(inpstr);
        }
        strcpy(word[0], cmd->name);
        return remove_first(inpstr);
#if !!0
        return inpstr + 1;
#endif
    }
    if (*inpstr == '.') {
#if !!0
        /* FIXME: Check direction or use memmove() */
        strcpy(word[0], word[0] + 1);
#endif
        return remove_first(inpstr);
    }
    if (defaultcmd) {
        strcpy(word[0], defaultcmd->name);
        return inpstr;
    }
    return remove_first(inpstr);
}

/*
 * split up command abbreviations
 */
void
split_command_string(char *inpstr)
{
    char tmp[ARR_SIZE + 2];

    strcpy(tmp, &inpstr[1]);
    inpstr[1] = ' ';
    strcpy(&inpstr[2], tmp);
    word_count = wordfind(inpstr);
}

/*
 * Terminal escape string length
 *
 * This returns the number of non-printable characters (including escapes)
 * found within the first <len> number of printable characters from <str>.
 * <len> equals zero implies no length restraint; count all non-print chars.
 * It is most useful for formatting, truncation, and padding.
 */
size_t
teslen(const char *str, size_t len)
{
    const char *s;
    size_t n, i;

    if (!str) {
        return 0;
    }
    n = 0;
    for (s = str; *s; ++s) {
        if (*s == '~') {
            for (i = 0; colour_codes[i].txt_code; ++i) {
                if (!strncmp(s + 1, colour_codes[i].txt_code,
                        strlen(colour_codes[i].txt_code))) {
                    break;
                }
            }
            if (colour_codes[i].txt_code) {
                s += strlen(colour_codes[i].txt_code);
                continue;
            }
            if (s[1] == '~') {
                ++s;
            }
        } else if (*s == '^') {
            if (s[1] == '~') {
                ++s;
            }
        }
        if (len && n >= len) {
            break;
        }
        ++n;
    }
    return s - str - n;
}

/*
 * Simple soundex algorithm as described by Knuth in TAOCP, vol 3
 * Based on the PHP soundex code by Bjorn Borud
 */
void
get_soundex(const char *checkword, char *soundex)
{
    static const char soundex_table[26] = {0, '1', '2', '3', 0, '1', '2', 0, 0, '2', '2', '4', '5', '5', 0, '1', '2', '6', '2', '3', 0, '1', 0, '2', 0, '2'}; /* A-Z */
    int i, small, len, code, last;

    len = strlen(checkword);
    if (!len) {
        *soundex = '\0';
        return;
    }
    /* build soundex string */
    last = -1;
    for (i = 0, small = 0; i < len && small < 4; ++i) {
        code = toupper(checkword[i]);
        if (code >= 'A' && code <= 'Z') {
            if (!small) {
                soundex[small++] = code;
                last = soundex_table[code - 'A'];
            } else {
                code = soundex_table[code - 'A'];
                if (code != last) {
                    if (code) {
                        soundex[small++] = code;
                    }
                    last = code;
                }
            }
        }
    }
    /* pad with zeros and terminate with NUL */
    while (small < 4) {
        soundex[small++] = '0';
    }
    soundex[small] = '\0';
}

/*
 * Converts unix time into words
 * "borrowed" from PG+ - Thanks to Richard Lawrence (Silver)
 * (not that he knows I borrowed this, until he looks at the code ;)
 */
char *
word_time(int t)
{
    static char time_string[100];
    char *fill = time_string;
    int neg, days, hrs, mins, secs;

    if (!t) {
        strcpy(fill, "no time at all");
        return time_string;
    }
    neg = t < 0;
    if (neg) {
        t = 0 - t;
        strcpy(fill, "negative ");
        while (*fill) {
            ++fill;
        }
    }
    days = t / 86400;
    hrs = (t / 3600) % 24;
    mins = (t / 60) % 60;
    secs = t % 60;
    if (days) {
        sprintf(fill, "%d day", days);
        while (*fill) {
            ++fill;
        }
        if (days != 1) {
            *fill++ = 's';
        }
        if (hrs || mins || secs) {
            *fill++ = ',';
            *fill++ = ' ';
        }
    }
    if (hrs) {
        sprintf(fill, "%d hour", hrs);
        while (*fill) {
            ++fill;
        }
        if (hrs != 1) {
            *fill++ = 's';
        }
        if (mins && secs) {
            *fill++ = ',';
            *fill++ = ' ';
        }
        if ((mins && !secs) || (!mins && secs)) {
            strcpy(fill, " and ");
            while (*fill) {
                ++fill;
            }
        }
    }
    if (mins) {
        sprintf(fill, "%d min", mins);
        while (*fill) {
            ++fill;
        }
        if (mins != 1) {
            *fill++ = 's';
        }
        if (secs) {
            strcpy(fill, " and ");
            while (*fill) {
                ++fill;
            }
        }
    }
    if (secs) {
        sprintf(fill, "%d sec", secs);
        while (*fill) {
            ++fill;
        }
        if (secs != 1) {
            *fill++ = 's';
        }
    }
    *fill++ = '\0';
    return time_string;
}
