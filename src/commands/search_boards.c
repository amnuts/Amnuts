
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Search all the boards for the words given in the list. Rooms fixed to
 * private will be ignore if the users level is less than gatecrash_level
 */
void
search_boards(UR_OBJECT user)
{
    char filename[80], line[82], buff[(MAX_LINES + 1) * 82], w1[81], *s;
    FILE *fp;
    RM_OBJECT rm;
    int w, cnt, message, yes, room_given;

    if (word_count < 2) {
        write_user(user, "Usage: search <word list>\n");
        return;
    }
    /* Go through rooms */
    cnt = 0;
    for (rm = room_first; rm; rm = rm->next) {
        if (!has_room_access(user, rm)) {
            continue;
        }
        if (is_personal_room(rm)) {
            sprintf(filename, "%s/%s/%s.B", USERFILES, USERROOMS, rm->owner);
        } else {
            sprintf(filename, "%s/%s.B", DATAFILES, rm->name);
        }
        fp = fopen(filename, "r");
        if (!fp) {
            continue;
        }
        /* Go through file */
        *buff = '\0';
        yes = message = room_given = 0;
        for (s = fgets(line, 81, fp); s; s = fgets(line, 81, fp)) {
            if (*s == '\n') {
                if (yes) {
                    strcat(buff, "\n");
                    write_user(user, buff);
                }
                message = yes = 0;
                *buff = '\0';
            }
            if (!message) {
                *w1 = '\0';
                sscanf(s, "%s", w1);
                if (!strcmp(w1, "PT:")) {
                    message = 1;
                    strcpy(buff, remove_first(remove_first(s)));
                }
            } else {
                strcat(buff, s);
            }
            for (w = 1; w < word_count; ++w) {
                if (!yes && strstr(s, word[w])) {
                    if (!room_given) {
                        vwrite_user(user, "~BB*** %s ***\n\n", rm->name);
                        room_given = 1;
                    }
                    yes = 1;
                    ++cnt;
                }
            }
        }
        if (yes) {
            strcat(buff, "\n");
            write_user(user, buff);
        }
        fclose(fp);
    }
    if (cnt) {
        vwrite_user(user, "Total of %d matching message%s.\n\n", cnt,
                PLTEXT_S(cnt));
    } else {
        write_user(user, "No occurences found.\n");
    }
}
