#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * read all the user files to check if a user exists
 */
void
recount_users(UR_OBJECT user, char *inpstr)
{
    char filename[80], *s;
    DIR *dirp;
    FILE *fp;
    struct dirent *dp;
    UD_OBJECT entry, next;
    UR_OBJECT u;
    int incorrect, correct, added, removed;

    if (!user->misc_op) {
        user->misc_op = 17;
        write_user(user,
                "~OL~FRWARNING:~RS This process may take some time if you have a lot of user accounts.\n");
        write_user(user,
                "         This should only be done if there are no, or minimal, users currently\n         logged on.\n");
        write_user(user, "\nDo you wish to continue (y|n)? ");
        return;
    }
    user->misc_op = 0;
    if (tolower(*inpstr) != 'y') {
        return;
    }
    write_user(user,
            "\n+----------------------------------------------------------------------------+\n");
    incorrect = correct = added = removed = 0;
    write_user(user, "~OLRecounting all of the users...\n");
    /* First process the files to see if there are any to add to the directory listing */
    write_user(user, "Processing users to add...");
    u = create_user();
    if (!u) {
        write_user(user, "ERROR: Cannot create user object.\n");
        write_syslog(SYSLOG | ERRLOG, 1,
                "ERROR: Cannot create user object in recount_users().\n");
        return;
    }
    /* open the directory file up */
    dirp = opendir(USERFILES);
    if (!dirp) {
        write_user(user, "ERROR: Failed to open userfile directory.\n");
        write_syslog(SYSLOG | ERRLOG, 1,
                "ERROR: Directory open failure in recount_users().\n");
        return;
    }
    /* count up how many files in the directory - this include . and .. */
    for (dp = readdir(dirp); dp; dp = readdir(dirp)) {
        s = strchr(dp->d_name, '.');
        if (!s || strcmp(s, ".D")) {
            continue;
        }
        *u->name = '\0';
        strncat(u->name, dp->d_name, (size_t) (s - dp->d_name));
        for (entry = first_user_entry; entry; entry = next) {
            next = entry->next;
            if (!strcmp(u->name, entry->name)) {
                break;
            }
        }
        if (!entry) {
            if (load_user_details(u)) {
                add_user_node(u->name, u->level);
                write_syslog(SYSLOG, 0,
                        "Added new user node for existing user \"%s\"\n",
                        u->name);
                ++added;
                reset_user(u);
            }
            /* FIXME: Probably ought to warn about this case */
        } else {
            ++correct;
        }
    }
    closedir(dirp);
    destruct_user(u);
    /*
     * Now process any nodes to remove the directory listing.  This may
     * not be optimal to do one loop to add and then one to remove, but
     * it is the best way I can think of doing it right now at 4:27am!
     */
    write_user(user, "\nProcessing users to remove...");
    for (entry = first_user_entry; entry; entry = next) {
        next = entry->next;
        sprintf(filename, "%s/%s.D", USERFILES, entry->name);
        fp = fopen(filename, "r");
        if (!fp) {
            ++removed;
            --correct;
            write_syslog(SYSLOG, 0,
                    "Removed user node for \"%s\" - user file does not exist.\n",
                    entry->name);
            rem_user_node(entry->name);
        } else {
            fclose(fp);
        }
    }
    write_user(user,
            "\n+----------------------------------------------------------------------------+\n");
    vwrite_user(user,
            "Checked ~OL%d~RS user%s.  ~OL%d~RS node%s %s added, and ~OL%d~RS node%s %s removed.\n",
            added + removed + correct, PLTEXT_S(added + removed + correct),
            added, PLTEXT_S(added), PLTEXT_WAS(added), removed,
            PLTEXT_S(removed), PLTEXT_WAS(removed));
    if (incorrect) {
        write_user(user, "See the system log for further details.\n");
    }
    write_user(user,
            "+----------------------------------------------------------------------------+\n");
}
