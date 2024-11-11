#include "defines.h"
#include "globals.h"
#include "commands.h"
#include "prototypes.h"

/*
 * allows a user to email specific messages to themselves
 */
void
forward_specific_mail(UR_OBJECT user)
{
    char w1[ARR_SIZE], line[ARR_SIZE], filenamei[80], filenameo[80],
            subject[80], *s;
    FILE *fpi, *fpo;
    int valid, cnt, total, smail_number, tmp1, tmp2;

    if (word_count < 2) {
        write_user(user, "Usage: fmail all|<mail number>\n");
        return;
    }
    total = mail_sizes(user->name, 0);
    if (!total) {
        write_user(user, "You currently have no mail.\n");
        return;
    }
    if (!user->mail_verified) {
        write_user(user, "You have not yet verified your email address.\n");
        return;
    }
    sprintf(subject, "Manual forwarding of smail (%s)", user->name);
    /* send all smail */
    if (!strcasecmp(word[1], "all")) {
        sprintf(filenameo, "%s/%s.FWD", MAILSPOOL, user->name);
        fpo = fopen(filenameo, "w");
        if (!fpo) {
            write_syslog(SYSLOG, 0,
                    "Unable to open forward mail file in forward_specific_mail()\n");
            write_user(user, "Sorry, could not forward any mail to you.\n");
            return;
        }
        sprintf(filenamei, "%s/%s/%s.M", USERFILES, USERMAILS, user->name);
        fpi = fopen(filenamei, "r");
        if (!fpi) {
            write_user(user, "Sorry, could not forward any mail to you.\n");
            write_syslog(SYSLOG, 0,
                    "Unable to open %s's mailbox in forward_specific_mail()\n",
                    user->name);
            fclose(fpo);
            return;
        }
        fprintf(fpo, "From: %s\n", TALKER_NAME);
        fprintf(fpo, "To: %s <%s>\n\n", user->name, user->email);
        fscanf(fpi, "%d %d\r", &tmp1, &tmp2);
        for (s = fgets(line, ARR_SIZE - 1, fpi); s;
                s = fgets(line, ARR_SIZE - 1, fpi)) {
            fprintf(fpo, "%s", colour_com_strip(s));
        }
        fputs(talker_signature, fpo);
        fclose(fpi);
        fclose(fpo);
        send_forward_email(user->email, filenameo, subject);
        write_user(user,
                "You have now sent ~OL~FRall~RS your smails to your email account.\n");
        return;
    }
    /* send just a specific smail */
    smail_number = atoi(word[1]);
    if (!smail_number) {
        write_user(user, "Usage: fmail all/<mail number>\n");
        return;
    }
    if (smail_number > total) {
        vwrite_user(user, "You only have %d message%s in your mailbox.\n", total,
                PLTEXT_S(total));
        return;
    }
    sprintf(filenameo, "%s/%s.FWD", MAILSPOOL, user->name);
    fpo = fopen(filenameo, "w");
    if (!fpo) {
        write_syslog(SYSLOG, 0,
                "Unable to open forward mail file in forward_specific_mail()\n");
        write_user(user, "Sorry, could not forward any mail to you.\n");
        return;
    }
    sprintf(filenamei, "%s/%s/%s.M", USERFILES, USERMAILS, user->name);
    fpi = fopen(filenamei, "r");
    if (!fpi) {
        write_user(user, "Sorry, could not forward any mail to you.\n");
        write_syslog(SYSLOG, 0,
                "Unable to open %s's mailbox in forward_specific_mail()\n",
                user->name);
        fclose(fpo);
        return;
    }
    fprintf(fpo, "From: %s\n", TALKER_NAME);
    fprintf(fpo, "To: %s <%s>\n\n", user->name, user->email);
    valid = cnt = 1;
    fscanf(fpi, "%d %d\r", &tmp1, &tmp2);
    for (s = fgets(line, ARR_SIZE - 1, fpi); s;
            s = fgets(line, ARR_SIZE - 1, fpi)) {
        if (*s == '\n') {
            valid = 1;
        }
        sscanf(s, "%s", w1);
        if (valid && !strcmp(colour_com_strip(w1), "From:")) {
            valid = 0;
            if (smail_number == cnt++) {
                break;
            }
        }
    }
    for (; s; s = fgets(line, ARR_SIZE - 1, fpi)) {
        if (*s == '\n') {
            break;
        }
        fprintf(fpo, "%s", colour_com_strip(s));
    }
    fputs(talker_signature, fpo);
    fclose(fpi);
    fclose(fpo);
    send_forward_email(user->email, filenameo, subject);
    vwrite_user(user,
            "You have now sent smail number ~FM~OL%d~RS to your email account.\n",
            smail_number);
}
