/*****************************************************************************
             Amncat version 1.1.0 - Copyright (C) Andrew Collington
                        Last update: 2003-03-20

      email: amnuts@talker.com   homepage: http://amnuts.talker.com/
 *****************************************************************************/


#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#define USER_NAME_LEN 12
#define DEFAULT_NAME "Snarfo"
#define ARR_SIZE 1000
#define OUT_BUFF_SIZE 1000

/* colour code values */
struct colour_codes_struct
{
  const char *txt_code;
  const char *esc_code;
};
static const struct colour_codes_struct colour_codes;
static const struct colour_codes_struct colour_codes[] = {
  /* Standard stuff */
  {"RS", "\033[0m"},            /* reset */
  {"OL", "\033[1m"},            /* bold */
  {"UL", "\033[4m"},            /* underline */
  {"LI", "\033[5m"},            /* blink */
  {"RV", "\033[7m"},            /* reverse */
  /* Foreground colour */
  {"FK", "\033[30m"},           /* black */
  {"FR", "\033[31m"},           /* red */
  {"FG", "\033[32m"},           /* green */
  {"FY", "\033[33m"},           /* yellow */
  {"FB", "\033[34m"},           /* blue */
  {"FM", "\033[35m"},           /* magenta */
  {"FC", "\033[36m"},           /* cyan */
  {"FW", "\033[37m"},           /* white */
  /* Background colour */
  {"BK", "\033[40m"},           /* black */
  {"BR", "\033[41m"},           /* red */
  {"BG", "\033[42m"},           /* green */
  {"BY", "\033[43m"},           /* yellow */
  {"BB", "\033[44m"},           /* blue */
  {"BM", "\033[45m"},           /* magenta */
  {"BC", "\033[46m"},           /* cyan */
  {"BW", "\033[47m"},           /* white */
  /* Some compatibility names */
  {"FT", "\033[36m"},           /* cyan AKA turquoise */
  {"BT", "\033[46m"},           /* cyan AKA turquoise */
  {NULL, NULL}
};

/* prototype */
static int cat(const char *filename);



/* main routine */
void
main(int argc, char *argv[])
{
  if (argc < 2) {
    printf("\nUsage: %s <filename>\n\n", argv[0]);
    exit(1);
  }
  if (!cat(argv[1])) {
    printf("\n%s: file not found\n\n", argv[0]);
    exit(1);
  }
  exit(0);
}


/* cat routine */
static int
cat(const char *filename)
{
  char text[ARR_SIZE * 2];
  char buff[OUT_BUFF_SIZE];
  const char *str;
  const char *s;
  FILE *fp;
  size_t i, buffpos;

  /* check if file exists */
  fp = fopen(filename, "r");
  if (!fp) {
    return 0;
  }
  /* Go through file */
  buffpos = 0;
  for (fgets(text, sizeof(text) - 1, fp);
       !feof(fp); fgets(text, sizeof(text) - 1, fp)) {
    str = text;
    /* Process line from file */
    for (s = str; *s; ++s) {
      if (buffpos > OUT_BUFF_SIZE - (6 < USER_NAME_LEN ? USER_NAME_LEN : 6)) {
        buff[buffpos] = '\0';
        printf("%s", buff);
        buffpos = 0;
      }
      if (*s == '\n') {
        /* Reset terminal before every newline */
        memcpy(buff + buffpos, colour_codes[0].esc_code,
               strlen(colour_codes[0].esc_code));
        buffpos += strlen(colour_codes[0].esc_code);
        buff[buffpos++] = '\r';
        buff[buffpos++] = '\n';
        continue;
      } else if (*s == '~') {
        /* process if colour variable */
        for (i = 0; i < colour_codes[i].txt_code; ++i) {
          if (!strncmp(s + 1, colour_codes[i].txt_code,
                       strlen(colour_codes[i].txt_code))) {
            break;
          }
        }
        if (colour_codes[i].txt_code) {
          memcpy(buffpos + buff, colour_codes[i].esc_code,
                 strlen(colour_codes[i].esc_code));
          buffpos += strlen(colour_codes[i].esc_code);
          s += strlen(colour_codes[i].txt_code);
          continue;
        }
        /* process if user name variable */
        if (s[1] == '$') {
          memcpy(buff + buffpos, DEFAULT_NAME, strlen(DEFAULT_NAME));
          buffpos += strlen(DEFAULT_NAME);
          ++s;
          continue;
        }
        if (s[1] == '~') {
          ++s;
        }
      } else if (*s == '^') {
        if (s[1] == '~') {
          ++s;
        }
      }
      buff[buffpos++] = *s;
    }
  }
  if (buffpos) {
    buff[buffpos] = '\0';
    printf("%s", buff);
  }
  fclose(fp);
  return 1;
}

/****************************************************************************/
