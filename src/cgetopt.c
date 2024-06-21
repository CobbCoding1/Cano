// Reimplementation of GNU getopt by proh14 and cobbcoding

#include "./cgetopt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *optarg = NULL;
int optind = 1, opterr = 0, optopt = '\0';
static int end_of_args = 0;

#define EXCHANGE(coropt)                                                       \
  do {                                                                         \
    int i = (coropt);                                                          \
    int j = (coropt)-1;                                                        \
    while (j >= 0 && argv[j][0] != '-') {                                      \
      getopt_exchange(argv, i, j);                                             \
      i--;                                                                     \
      j--;                                                                     \
    }                                                                          \
  } while (0)

static void getopt_printerr(const char *msg) {
  if (opterr) {
    fprintf(stderr, "%s", msg);
  }
}

static int getopt_in(char d, const char *str) {
  int i = 0;
  while (str[i] != '\0') {
    if (d == str[i] && str[i] != ':') {
      return i;
    }
    i++;
  }
  return -1;
}

static void getopt_exchange(char *argv[], int i, int j) {
  char *tmp = argv[i];
  argv[i] = argv[j];
  argv[j] = tmp;
}

int cgetopt(int argc, char *argv[], const char *optstring) {
  int c;
  static char *nextchar = NULL;
  static int coropt = 1;
  if (!coropt) {
    coropt = 1;
  }

  if (coropt >= argc || argv[coropt] == NULL) {
    return -1;
  }

  while (argv[coropt] && argv[coropt][0] != '-') {
    coropt++;
  }

  if (coropt >= argc) {
    return -1;
  }

  nextchar = &argv[coropt][1];

  if (strcmp(argv[coropt], "--") == 0) {
    EXCHANGE(coropt);
    end_of_args = 1;
    optind++;
    return -1;
  }

  int idx = idx = getopt_in(*nextchar, optstring);
  if (!(idx >= 0)) {
    getopt_printerr("invalid option\n");
    optopt = (unsigned)*nextchar;
    if (*(nextchar + 1) == '\0') {
      nextchar = NULL;
      optind++;
    }
    optind++;
    c = '?';
    goto exit;
  }

  c = (unsigned)*nextchar++;
  if (*nextchar == '\0') {
    coropt++;
    optind++;
  }

  if (optstring[idx + 1] != ':') {
    coropt--;
    goto exit;
  }
  EXCHANGE(coropt - 1);

  if (nextchar != NULL && *nextchar != '\0') {
    coropt++;
  }

  if (coropt >= argc || argv[coropt][0] == '-') {
    getopt_printerr("option requires an argument\n");
    optopt = (unsigned)*nextchar;
    c = '?';
    optind++;
    goto exit;
  }

  optarg = argv[coropt];
  optind++;

exit: { EXCHANGE(coropt); }
  return c;
}

int cgetopt_long(int argc, char *argv[], const char *optstring,
                const struct option *longopts, int *longindex) {
  int c;
  static char *nextchar = NULL;
  static int coropt = 1;
  if (!coropt) {
    coropt = 1;
  }

  if (coropt >= argc || argv[coropt] == NULL) {
    return -1;
  }

  while (argv[coropt] && argv[coropt][0] != '-') {
    coropt++;
  }

  if (nextchar == NULL || *nextchar == '\0') {
    if (coropt >= argc) {
      return -1;
    }
    nextchar = &argv[coropt][1];
  }

  if (*nextchar == '-') {
    nextchar++;
    const struct option *curlong = longopts;
    while (curlong->name != NULL) {
      size_t name_len = strlen(curlong->name);
      if (strncmp(curlong->name, nextchar, name_len) == 0) {
        if (longindex != NULL)
          *longindex = curlong - longopts;
        optind++;
        if (longopts->flag) {
          *curlong->flag = curlong->val;
          c = 0;
        } else {
          c = curlong->val;
        }
        if (curlong->has_arg == optional_argument &&
            ((coropt + 1) >= argc || argv[(coropt + 1)][0] == '-'))
          goto exit;
        if (curlong->has_arg) {
          if (*(nextchar + name_len) != '=') {
            nextchar = NULL;
            EXCHANGE(coropt);
            coropt++;
            if (coropt >= argc || argv[coropt][0] == '-') {
              getopt_printerr("option requires an argument\n");
              optopt = curlong->val;
              c = '?';
              goto exit;
            }
            optarg = argv[coropt];
            optind++;
          } else {
            optarg = nextchar + (name_len + 1);
          }
        }
        nextchar = NULL;
        goto exit;
      }
      curlong++;
    }
    optind++;
    optopt = longopts->val;
    nextchar = NULL;
    getopt_printerr("invalid option\n");
    c = '?';
    goto exit;
  }

  int idx = getopt_in(*nextchar, optstring);
  if (!(idx >= 0)) {
    getopt_printerr("invalid option\n");
    optopt = (unsigned)*nextchar;
    if (*(nextchar + 1) == '\0') {
      nextchar = NULL;
      optind++;
    }
    optind++;
    c = '?';
    goto exit;
  }

  c = (unsigned)*nextchar++;
  if (*nextchar == '\0') {
    coropt++;
    optind++;
  }

  if (optstring[idx + 1] != ':') {
    coropt--;
    goto exit;
  }
  EXCHANGE(coropt - 1);

  if (nextchar != NULL && *nextchar != '\0') {
    coropt++;
  }

  if (coropt >= argc || argv[coropt][0] == '-') {
    getopt_printerr("option requires an argument\n");
    optopt = (unsigned)*nextchar;
    optind++;
    c = '?';
    goto exit;
  }

  optarg = argv[coropt];
  optind++;

exit: { EXCHANGE(coropt); }
  return c;
}
