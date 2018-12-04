
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Remove any expired messages from boards unless force = 2 in which case
 * just do a recount.
 */
void
check_messages(UR_OBJECT user, int chforce)
{
    char id[82], filename[80], line[82], *s;
    FILE *infp, *outfp;
    RM_OBJECT rm;
    int valid, pt, write_rest, board_cnt, old_cnt, bad_cnt, tmp;

    infp = outfp = NULL;
    switch (chforce) {
    case 0:
        break;
    case 1:
        printf("Checking boards...\n");
        break;
    case 2:
        if (word_count >= 2) {
            strtolower(word[1]);
            if (strcmp(word[1], "motds")) {
                write_user(user, "Usage: recount [motds]\n");
                return;
            }
            if (!count_motds(1)) {
                write_user(user,
                        "Sorry, could not recount the motds at this time.\n");
                write_syslog(SYSLOG | ERRLOG, 1,
                        "ERROR: Could not recount motds in check_messages().\n");
                return;
            }
            vwrite_user(user, "There %s %d login motd%s and %d post-login motd%s\n",
                    PLTEXT_WAS(amsys->motd1_cnt), amsys->motd1_cnt,
                    PLTEXT_S(amsys->motd1_cnt), amsys->motd2_cnt,
                    PLTEXT_S(amsys->motd2_cnt));
            write_syslog(SYSLOG, 1, "%s recounted the MOTDS.\n", user->name);
            return;
        }
        break;
    }
    board_cnt = 0;
    old_cnt = 0;
    bad_cnt = 0;
    for (rm = room_first; rm; rm = rm->next) {
        tmp = rm->mesg_cnt;
        rm->mesg_cnt = 0;
        if (is_personal_room(rm)) {
            sprintf(filename, "%s/%s/%s.B", USERFILES, USERROOMS, rm->owner);
        } else {
            sprintf(filename, "%s/%s.B", DATAFILES, rm->name);
        }
        infp = fopen(filename, "r");
        if (!infp) {
            continue;
        }
        if (chforce < 2) {
            outfp = fopen("tempfile", "w");
            if (!outfp) {
                if (chforce) {
                    fprintf(stderr, "Amnuts: Cannot open tempfile.\n");
                }
                write_syslog(SYSLOG | ERRLOG, 0,
                        "ERROR: Cannot open tempfile in check_messages().\n");
                fclose(infp);
                return;
            }
        }
        ++board_cnt;
        /*
           We assume that once 1 in date message is encountered all the others
           will be in date too , hence write_rest once set to 1 is never set to
           0 again
         */
        valid = 1;
        write_rest = 0;
        /* max of 80+newline+terminator = 82 */
        for (s = fgets(line, 82, infp); s; s = fgets(line, 82, infp)) {
            if (*s == '\n') {
                valid = 1;
            }
            sscanf(s, "%s %d", id, &pt);
            if (!write_rest) {
                if (valid && !strcmp(id, "PT:")) {
                    if (chforce == 2) {
                        ++rm->mesg_cnt;
                    } else {
                        /* 86400 = num. of secs in a day */
                        if ((int) time(0) - pt < amsys->mesg_life * 86400) {
                            fputs(s, outfp);
                            ++rm->mesg_cnt;
                            write_rest = 1;
                        } else {
                            ++old_cnt;
                        }
                    }
                    valid = 0;
                }
            } else {
                fputs(s, outfp);
                if (valid && !strcmp(id, "PT:")) {
                    ++rm->mesg_cnt;
                    valid = 0;
                }
            }
        }
        fclose(infp);
        if (chforce < 2) {
            fclose(outfp);
            remove(filename);
            if (!write_rest) {
                remove("tempfile");
            } else {
                rename("tempfile", filename);
            }
        }
        if (rm->mesg_cnt != tmp) {
            ++bad_cnt;
        }
    }
    switch (chforce) {
    case 0:
        if (bad_cnt) {
            write_syslog(SYSLOG, 1,
                    "CHECK_MESSAGES: %d file%s checked, %d had an incorrect message count, %d message%s deleted.\n",
                    board_cnt, PLTEXT_S(board_cnt), bad_cnt, old_cnt,
                    PLTEXT_S(old_cnt));
        } else {
            write_syslog(SYSLOG, 1,
                    "CHECK_MESSAGES: %d file%s checked, %d message%s deleted.\n",
                    board_cnt, PLTEXT_S(board_cnt), old_cnt,
                    PLTEXT_S(old_cnt));
        }
        break;
    case 1:
        printf("  %d board file%s checked, %d out of date message%s found.\n",
                board_cnt, PLTEXT_S(board_cnt), old_cnt, PLTEXT_S(old_cnt));
        break;
    case 2:
        vwrite_user(user,
                "%d board file%s checked, %d had an incorrect message count.\n",
                board_cnt, PLTEXT_S(board_cnt), bad_cnt);
        write_syslog(SYSLOG, 1, "%s forced a recount of the message boards.\n",
                user->name);
        break;
    }
}
