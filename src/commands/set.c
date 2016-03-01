
#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * Set the user attributes
 */
void
set_attributes(UR_OBJECT user)
{
    char name[USER_NAME_LEN + 1], *recname;
    int i, tmp;

    if (word_count < 2) {
        i = -1;
    } else {
        /* FIXME: Add abbreviated matching like help() does */
        for (i = 0; setstr[i].type; ++i) {
            if (!strcasecmp(setstr[i].type, word[1])) {
                break;
            }
        }
        if (!setstr[i].type) {
            i = -1;
        }
    }
    if (i == -1) {
        write_user(user, "Attributes you can set are:\n\n");
        for (i = 0; setstr[i].type; ++i) {
            vwrite_user(user, "~FC%s~RS : %s\n", setstr[i].type, setstr[i].desc);
        }
        write_user(user, "\n");
        return;
    }
    write_user(user, "\n");
    switch (i) {
    case SETSHOW:
        show_attributes(user);
        return;
    case SETGEND:
        *word[2] = tolower(*word[2]);
        if (*word[2] == 'm' || *word[2] == 'f' || *word[2] == 'n') {
            switch (*word[2]) {
            case 'm':
                user->gender = MALE;
                write_user(user, "Gender set to Male\n");
                break;
            case 'f':
                user->gender = FEMALE;
                write_user(user, "Gender set to Female\n");
                break;
            case 'n':
                user->gender = NEUTER;
                write_user(user, "Gender set to Unset\n");
                break;
            }
            check_autopromote(user, 1);
            return;
        }
        write_user(user, "Usage: set gender m|f|n\n");
        return;
    case SETAGE:
        if (word_count < 3) {
            write_user(user, "Usage: set age <age>\n");
            return;
        }
        tmp = atoi(word[2]);
        if (tmp < 0 || tmp > 200) {
            write_user(user,
                    "You can only set your age range between 0 (unset) and 200.\n");
            return;
        }
        user->age = tmp;
        vwrite_user(user, "Age now set to: %d\n", user->age);
        return;
    case SETWRAP:
        switch (user->wrap) {
        case 0:
            user->wrap = 1;
            write_user(user, "Word wrap now ON\n");
            break;
        case 1:
            user->wrap = 0;
            write_user(user, "Word wrap now OFF\n");
            break;
        }
        return;
    case SETEMAIL:
        strcpy(word[2], colour_com_strip(word[2]));
        if (strlen(word[2]) > 80) {
            write_user(user,
                    "The maximum email length you can have is 80 characters.\n");
            return;
        }
        if (!validate_email(word[2])) {
            write_user(user,
                    "That email address format is incorrect.  Correct format: user@network.net\n");
            return;
        }
        strcpy(user->email, word[2]);
        if (!*user->email) {
            write_user(user, "Email set to : ~FRunset\n");
        } else {
            vwrite_user(user, "Email set to : ~FC%s\n", user->email);
        }
        set_forward_email(user);
        return;
    case SETHOMEP:
        strcpy(word[2], colour_com_strip(word[2]));
        if (strlen(word[2]) > 80) {
            write_user(user,
                    "The maximum homepage length you can have is 80 characters.\n");
            return;
        }
        strcpy(user->homepage, word[2]);
        if (!*user->homepage) {
            write_user(user, "Homepage set to : ~FRunset\n");
        } else {
            vwrite_user(user, "Homepage set to : ~FC%s\n", user->homepage);
        }
        return;
    case SETHIDEEMAIL:
        switch (user->hideemail) {
        case 0:
            user->hideemail = 1;
            write_user(user, "Email now showing to only the admins.\n");
            break;
        case 1:
            user->hideemail = 0;
            write_user(user, "Email now showing to everyone.\n");
            break;
        }
        return;
    case SETCOLOUR:
        switch (user->colour) {
        case 0:
            user->colour = 1;
            write_user(user, "~FCColour now on\n");
            break;
        case 1:
            user->colour = 0;
            write_user(user, "Colour now off\n");
            break;
        }
        return;
    case SETPAGER:
        if (word_count < 3) {
            write_user(user, "Usage: set pager <length>\n");
            return;
        }
        user->pager = atoi(word[2]);
        if (user->pager < MAX_LINES || user->pager > 99) {
            vwrite_user(user,
                    "Pager can only be set between %d and 99 - setting to default\n",
                    MAX_LINES);
            user->pager = 23;
        }
        vwrite_user(user, "Pager length now set to: %d\n", user->pager);
        return;
    case SETROOM:
        switch (user->lroom) {
        case 0:
            user->lroom = 1;
            write_user(user, "You will log on into the room you left from.\n");
            break;
        case 1:
            user->lroom = 0;
            write_user(user, "You will log on into the main room.\n");
            break;
        }
        return;
    case SETFWD:
        if (!*user->email) {
            write_user(user,
                    "You have not yet set your email address - autofwd cannot be used until you do.\n");
            return;
        }
        if (!user->mail_verified) {
            write_user(user,
                    "You have not yet verified your email - autofwd cannot be used until you do.\n");
            return;
        }
        switch (user->autofwd) {
        case 0:
            user->autofwd = 1;
            write_user(user, "You will also receive smails via email.\n");
            break;
        case 1:
            user->autofwd = 0;
            write_user(user, "You will no longer receive smails via email.\n");
            break;
        }
        return;
    case SETPASSWD:
        switch (user->show_pass) {
        case 0:
            user->show_pass = 1;
            write_user(user,
                    "You will now see your password when entering it at login.\n");
            break;
        case 1:
            user->show_pass = 0;
            write_user(user,
                    "You will no longer see your password when entering it at login.\n");
            break;
        }
        return;
    case SETRDESC:
        switch (user->show_rdesc) {
        case 0:
            user->show_rdesc = 1;
            write_user(user, "You will now see the room descriptions.\n");
            break;
        case 1:
            user->show_rdesc = 0;
            write_user(user, "You will no longer see the room descriptions.\n");
            break;
        }
        return;
    case SETCOMMAND:
        switch (user->cmd_type) {
        case 0:
            user->cmd_type = 1;
            write_user(user,
                    "You will now see commands listed by functionality.\n");
            break;
        case 1:
            user->cmd_type = 0;
            write_user(user, "You will now see commands listed by level.\n");
            break;
        }
        return;
    case SETRECAP:
        if (!amsys->allow_recaps) {
            write_user(user,
                    "Sorry, names cannot be recapped at this present time.\n");
            return;
        }
        if (word_count < 3) {
            write_user(user, "Usage: set recap <name as you would like it>\n");
            return;
        }
        if (strlen(word[2]) > RECAP_NAME_LEN - 3) {
            write_user(user,
                    "The recapped name length is too long - try using fewer colour codes.\n");
            return;
        }
        recname = colour_com_strip(word[2]);
        if (strlen(recname) > USER_NAME_LEN) {
            write_user(user,
                    "The recapped name still has to match your proper name.\n");
            return;
        }
        strcpy(name, recname);
        strtolower(name);
        *name = toupper(*name);
        if (strcmp(user->name, name)) {
            write_user(user,
                    "The recapped name still has to match your proper name.\n");
            return;
        }
        strcpy(user->recap, word[2]);
        strcat(user->recap, "~RS"); /* user->recap is allways escaped with a reset to its colours... */
        strcpy(user->bw_recap, recname);
        vwrite_user(user,
                "Your name will now appear as \"%s~RS\" on the \"who\", \"examine\", tells, etc.\n",
                user->recap);
        return;
    case SETICQ:
        strcpy(word[2], colour_com_strip(word[2]));
        if (strlen(word[2]) > ICQ_LEN) {
            vwrite_user(user,
                    "The maximum ICQ UIN length you can have is %d characters.\n",
                    ICQ_LEN);
            return;
        }
        strcpy(user->icq, word[2]);
        if (!*user->icq) {
            write_user(user, "ICQ number set to : ~FRunset\n");
        } else {
            vwrite_user(user, "ICQ number set to : ~FC%s\n", user->icq);
        }
        return;
    case SETALERT:
        switch (user->alert) {
        case 0:
            user->alert = 1;
            write_user(user,
                    "You will now be alerted if anyone on your friends list logs on.\n");
            break;
        case 1:
            user->alert = 0;
            write_user(user,
                    "You will no longer be alerted if anyone on your friends list logs on.\n");
            break;
        }
        return;
    case SETREVBUF:
        switch (user->reverse_buffer) {
        case 0:
            user->reverse_buffer = 1;
            write_user(user, "~FCBuffers are now reversed\n");
            break;
        case 1:
            user->reverse_buffer = 0;
            write_user(user, "Buffers now not reversed\n");
            break;
        }
        return;
    }
}
