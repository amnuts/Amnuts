#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Create an account for a user if new users from their site have been
 * banned, and they want to log on - you know they ain't a troublemaker, etc
 */
void
create_account(UR_OBJECT user)
{
    UR_OBJECT u;
    int i;

    if (word_count < 3) {
        write_user(user, "Usage: create <name> <passwd>\n");
        return;
    }
    if (find_user_listed(word[1])) {
        write_user(user,
                "You cannot create with the name of an existing user!\n");
        return;
    }
    for (i = 0; word[1][i]; ++i) {
        if (!isalpha(word[1][i])) {
            break;
        }
    }
    if (word[1][i]) {
        write_user(user,
                "You cannot have anything but letters in the name - account not created.\n\n");
        return;
    }
    if (i < USER_NAME_MIN) {
        write_user(user, "Name was too short--account not created.\n");
        return;
    }
    if (i > USER_NAME_LEN) {
        write_user(user, "Name was too long--account not created.\n");
        return;
    }
    if (contains_swearing(word[1])) {
        write_user(user,
                "You cannot use a name like that--account not created.\n\n");
        return;
    }
    i = strlen(word[2]);
    if (i < PASS_MIN) {
        write_user(user, "Password was too short--account not created.\n");
        return;
    }
    i = strlen(word[2]);
    /* Via use of crypt() */
    if (i > 8) {
        write_user(user,
                "WARNING: Only the first eight characters of password will be used!\n");
    }
    u = create_user();
    if (!u) {
        vwrite_user(user, "%s: unable to create temporary user session.\n",
                syserror);
        write_syslog(SYSLOG | ERRLOG, 0,
                "ERROR: Unable to create temporary user session in create_account().\n");
        return;
    }
    strtolower(word[1]);
    *word[1] = toupper(*word[1]);
    strcpy(u->name, word[1]);
    if (!load_user_details(u)) {
        strcpy(u->pass, crypt(word[2], crypt_salt));
        strcpy(u->recap, u->name);
        strcpy(u->desc, "is a newbie");
        strcpy(u->in_phrase, "wanders in.");
        strcpy(u->out_phrase, "wanders out");
        strcpy(u->last_site, "created_account");
        strcpy(u->site, u->last_site);
        strcpy(u->logout_room, "<none>");
        *u->verify_code = '\0';
        *u->email = '\0';
        *u->homepage = '\0';
        *u->icq = '\0';
        u->prompt = amsys->prompt_def;
        u->charmode_echo = 0;
        u->room = room_first;
        u->level = NEW;
        u->unarrest = NEW;
        save_user_details(u, 0);
        add_user_node(u->name, u->level);
        add_user_date_node(u->name, (long_date(1)));
        sprintf(text, "Was manually created by %s.\n", user->name);
        add_history(u->name, 1, "%s", text);
        vwrite_user(user,
                "You have created an account with the name \"~FC%s~RS\" and password \"~FG%s~RS\".\n",
                u->name, word[2]);
        write_syslog(SYSLOG, 1, "%s created a new account with the name \"%s\"\n",
                user->name, u->name);
        destruct_user(u);
        destructed = 0;
        return;
    }
    write_user(user,
            "You cannot create an account with the name of an existing user!\n");
    destruct_user(u);
    destructed = 0;
}
