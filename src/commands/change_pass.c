
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Change users password. Only GODs and above can change another users
 * password and they do this by specifying the user at the end. When this is
 * done the old password given can be anything, the wiz doesnt have to know it
 * in advance
 */
void
change_pass(UR_OBJECT user)
{
    UR_OBJECT u;
    const char *name;
    int i;

    if (word_count < 3) {
        if (user->level < GOD) {
            write_user(user, "Usage: passwd <old password> <new password>\n");
        } else {
            write_user(user,
                    "Usage: passwd <old password> <new password> [<user>]\n");
        }
        return;
    }
    i = strlen(word[2]);
    if (i < PASS_MIN) {
        write_user(user, "New password too short.\n");
        return;
    }
    /* Via use of crypt() */
    if (i > 8) {
        write_user(user,
                "WARNING: Only the first eight characters of password will be used!\n");
    }
    /* Change own password */
    if (word_count == 3) {
        if (strcmp(user->pass, crypt(word[1], user->pass))) {
            write_user(user, "Old password incorrect.\n");
            return;
        }
        if (!strcmp(word[1], word[2])) {
            write_user(user, "Old and new passwords are the same.\n");
            return;
        }
        strcpy(user->pass, crypt(word[2], crypt_salt));
        save_user_details(user, 0);
        cls(user);
        write_user(user, "Password changed.\n");
        add_history(user->name, 1, "Changed passwords.\n");
        return;
    }
    /* Change someone else's password */
    if (user->level < GOD) {
        write_user(user,
                "You are not a high enough level to use the user option.\n");
        return;
    }
    *word[3] = toupper(*word[3]);
    if (!strcmp(word[3], user->name)) {
        /*
           security feature  - prevents someone coming to a wizes terminal and
           changing their password since they won't have to know the old one
         */
        write_user(user,
                "You cannot change your own password using the <user> option.\n");
        return;
    }
    u = get_user(word[3]);
    if (u) {
#ifdef NETLINKS
        if (u->type == REMOTE_TYPE) {
            write_user(user,
                    "You cannot change the password of a user logged on remotely.\n");
            return;
        }
#endif
        if (u->level >= user->level) {
            write_user(user,
                    "You cannot change the password of a user of equal or higher level than yourself.\n");
            return;
        }
        strcpy(u->pass, crypt(word[2], crypt_salt));
        cls(user);
        vwrite_user(user, "%s~RS's password has been changed.\n", u->recap);
        name = user->vis ? user->name : invisname;
        vwrite_user(u, "~FR~OLYour password has been changed by %s!\n", name);
        write_syslog(SYSLOG, 1, "%s changed %s's password.\n", user->name,
                u->name);
        sprintf(text, "Forced password change by %s.\n", user->name);
        add_history(u->name, 0, "%s", text);
        return;
    }
    u = create_user();
    if (!u) {
        vwrite_user(user, "%s: unable to create temporary user object.\n",
                syserror);
        write_syslog(SYSLOG | ERRLOG, 0,
                "ERROR: Unable to create temporary user object in change_pass().\n");
        return;
    }
    strcpy(u->name, word[3]);
    if (!load_user_details(u)) {
        write_user(user, nosuchuser);
        destruct_user(u);
        destructed = 0;
        return;
    }
    if (u->level >= user->level) {
        write_user(user,
                "You cannot change the password of a user of equal or higher level than yourself.\n");
        destruct_user(u);
        destructed = 0;
        return;
    }
    strcpy(u->pass, crypt(word[2], crypt_salt));
    save_user_details(u, 0);
    destruct_user(u);
    destructed = 0;
    cls(user);
    vwrite_user(user, "%s's password changed to \"%s\".\n", word[3], word[2]);
    sprintf(text, "Forced password change by %s.\n", user->name);
    add_history(word[3], 0, "%s", text);
    write_syslog(SYSLOG, 1, "%s changed %s's password.\n", user->name, word[3]);
}

