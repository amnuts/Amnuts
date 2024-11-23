
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Show list of people your mail is from without seeing the whole lot
 */
void
mail_from(UR_OBJECT user)
{
    char w1[ARR_SIZE], line[ARR_SIZE], filename[80], *s;
    FILE *fp;
    int valid, cnt, tmp1, tmp2, nmail;

    sprintf(filename, "%s/%s/%s.M", USERFILES, USERMAILS, user->name);
    fp = fopen(filename, "r");
    if (!fp) {
        write_user(user, "You have no mail.\n");
        return;
    }
    write_user(user, "\n~BB*** Mail from ***\n\n");
    valid = 1;
    cnt = 0;
    fscanf(fp, "%d %d\r", &tmp1, &tmp2);
    for (s = fgets(line, ARR_SIZE - 1, fp); s;
            s = fgets(line, ARR_SIZE - 1, fp)) {
        if (*s == '\n') {
            valid = 1;
        }
        sscanf(s, "%s", w1);
        if (valid && !strcmp(colour_com_strip(w1), "From:")) {
            vwrite_user(user, "~FC%2d)~RS %s", ++cnt, remove_first(s));
            valid = 0;
        }
    }
    fclose(fp);
    nmail = mail_sizes(user->name, 1);
    vwrite_user(user,
            "\nTotal of ~OL%d~RS message%s, ~OL%d~RS of which %s new.\n\n",
            cnt, PLTEXT_S(cnt), nmail, PLTEXT_IS(nmail));
}
