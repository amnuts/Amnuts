#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Display some files to the user.  This was once intergrated with the ".help" command,
 * but due to the new processing that goes through it was moved into its own command.
 * The files listed are now stored in MISCFILES rather than HELPFILES as they may not
 * necessarily be for any help commands.
 */
void
display_files(UR_OBJECT user, int admins)
{
    char filename[128];
    int ret;

    if (word_count < 2) {
        if (!admins) {
            sprintf(filename, "%s/%s", TEXTFILES, SHOWFILES);
        } else {
            sprintf(filename, "%s/%s/%s", TEXTFILES, ADMINFILES, SHOWFILES);
        }
        ret = more(user, user->socket, filename);
        if (!ret) {
            if (!admins) {
                write_user(user, "There is no list of files at the moment.\n");
            } else {
                write_user(user, "There is no list of admin files at the moment.\n");
            }
            return;
        }
        if (ret == 1) {
            user->misc_op = 2;
        }
        return;
    }
    /* check for any illegal characters */
    if (strpbrk(word[1], "./")) {
        if (!admins) {
            write_user(user, "Sorry, there are no files with that name.\n");
        } else {
            write_user(user, "Sorry, there are no admin files with that name.\n");
        }
        return;
    }
    /* show the file */
    if (!admins) {
        sprintf(filename, "%s/%s", TEXTFILES, word[1]);
    } else {
        sprintf(filename, "%s/%s/%s", TEXTFILES, ADMINFILES, word[1]);
    }
    ret = more(user, user->socket, filename);
    if (!ret) {
        if (!admins) {
            write_user(user, "Sorry, there are no files with that name.\n");
        } else {
            write_user(user, "Sorry, there are no admin files with that name.\n");
        }
        return;
    }
    if (ret == 1) {
        user->misc_op = 2;
    }
    return;
}

