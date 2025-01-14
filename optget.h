#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    OPTGET_ARGTYPE_NONE,
    OPTGET_ARGTYPE_OPTIONAL,
    OPTGET_ARGTYPE_REQUIRED,
} optget_argtype;

struct optget_option {
    optget_argtype argtype;
    char shortopt;
    char *longopt;
};

// We need this to return option index and arg
struct optget_optarg {
    size_t opt_ix;
    char *arg;
};

struct optget {
    int argc;
    char **argv;
    const struct optget_option *options;
    size_t options_len;
    struct optget_optarg optarg;
    bool nonopt_mode;
    int arg_ix;
    size_t shortopt_ix;
    bool error_flag;
    char err_msg[64];
};

void optget_init(struct optget *optget, int argc, char *argv[], const struct optget_option options[],
                 size_t options_len);
struct optget_optarg *optget_get(struct optget *optget);
char *optget_get_error(struct optget *optget);
