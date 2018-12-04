
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * see list of pictures availiable--file dictated in "go" script
 */
void
preview(UR_OBJECT user)
{
#if !!0
    static const char usage[] = "Usage: preview [<picture>]\n";
#endif
    char filename[80], line[100];
    FILE *fp;
    DIR *dirp;
    struct dirent *dp;
    int cnt, total;

    if (word_count < 2) {
        /* open the directory file up */
        dirp = opendir(PICTFILES);
        if (!dirp) {
            write_user(user, "No list of the picture files is availiable.\n");
            return;
        }
        *line = '\0';
        cnt = total = 0;
        /* go through directory and list files */
        for (dp = readdir(dirp); dp; dp = readdir(dirp)) {
            if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, "..")) {
                continue;
            }
            if (!total++) {
                write_user(user,
                        "+----------------------------------------------------------------------------+\n");
                write_user(user,
                        "| ~OL~FCPictures available to view~RS                                                 |\n");
                write_user(user,
                        "+----------------------------------------------------------------------------+\n");
            }
            sprintf(text, "%-12.12s   ", dp->d_name);
            strcat(line, text);
            if (++cnt == 5) {
                write_user(user, align_string(0, 78, 1, "|", "  %s", line));
                *line = '\0';
                cnt = 0;
            }
        }
        closedir(dirp);
        if (total) {
            if (cnt) {
                write_user(user, align_string(0, 78, 1, "|", "  %s", line));
            }
            write_user(user,
                    "+----------------------------------------------------------------------------+\n");
            write_user(user,
                    align_string(0, 78, 1, "|", "  There are %d picture%s  ",
                    total, PLTEXT_S(total)));
            write_user(user,
                    "+----------------------------------------------------------------------------+\n\n");
        } else {
            write_user(user, "There are no pictures available to be viewed.\n");
        }
        return;
    }
    if (strpbrk(word[1], "./")) {
        write_user(user, "Sorry, there is no picture with that name.\n");
        return;
    }
    sprintf(filename, "%s/%s", PICTFILES, word[1]);
    fp = fopen(filename, "r");
    if (!fp) {
        write_user(user, "Sorry, there is no picture with that name.\n");
        return;
    }
    fclose(fp);
    write_user(user, "You ~OL~FGpreview the following picture...\n\n");
    switch (more(user, user->socket, filename)) {
    case 0:
        break;
    case 1:
        user->misc_op = 2;
        break;
    }
}
