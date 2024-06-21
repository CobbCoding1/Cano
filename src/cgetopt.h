// Reimplementation of GNU getopt by proh14 and cobbcoding
#ifndef _CGETOPT_H_
#define _CGETOPT_H_

#define no_argument 0
#define required_argument 1
#define optional_argument 2

struct option {
  const char *name;
  int has_arg;
  int *flag;
  int val;
};

extern char *optarg;
extern int optind, opterr, optopt;

int cgetopt(int argc, char **argv, const char *optstring);

int cgetopt_long(int argc, char *argv[], const char *optstring,
                const struct option *longopts, int *longindex);

int cgetopt_long_only(int argc, char *argv[], const char *optstring,
                     const struct option *longopts, int *longindex);

#endif // _GETOPT_H_
