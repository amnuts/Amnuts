
#include "../includes/defines.h"
#include "../includes/globals.h"
#include "../includes/commands.h"
#include "../includes/prototypes.h"
#ifndef __SDS_H
#include "../vendors/sds/sds.h"
#endif

/*
 * see list of pictures available--file dictated in "go" script
 */
void
preview(UR_OBJECT user)
{
#if !!0
    static const char usage[] = "Usage: preview [<picture>]\n";
#endif
    sds filename, text;
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
        cnt = total = 0;
        text = sdsempty();
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
            text = sdscatprintf(text, "%-12.12s   ", dp->d_name);
            if (++cnt == 5) {
                text = sdscatfmt(sdsempty(), "%s\n", align_string(ALIGN_LEFT, 78, 1, "|", "  %s", text));
                write_user(user, text);
                text = sdsempty();
                cnt = 0;
            }
        }
        closedir(dirp);
        if (total) {
            if (cnt) {
                text = sdscatfmt(sdsempty(), "%s\n", align_string(ALIGN_LEFT, 78, 1, "|", "  %s", text));
            }
            write_user(user,
                    "+----------------------------------------------------------------------------+\n");
            write_user(user,
                    align_string(ALIGN_LEFT, 78, 1, "|", "  There are %d picture%s  ",
                    total, PLTEXT_S(total)));
            write_user(user,
                    "+----------------------------------------------------------------------------+\n\n");
        } else {
            write_user(user, "There are no pictures available to be viewed.\n");
        }

        sdsfree(text);
        return;
    }
    if (strpbrk(word[1], "./")) {
        write_user(user, "Sorry, there is no picture with that name.\n");
        return;
    }
    filename = sdscatfmt(sdsempty(), "%s/%s", PICTFILES, word[1]);
    fp = fopen(filename, "r");
    if (!fp) {
        write_user(user, "Sorry, there is no picture with that name.\n");
        sdsfree(filename);
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
    sdsfree(filename);
}
