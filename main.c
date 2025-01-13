// So single char options are called short options, maybe they can be called shortopts, like -a -b
// And they can be chained (or grouped), so it can be -ab
// Short options can take arguments -a arga -b argb, also no need of space, so it can be -aarga
// -bargb And optional arguments as long as space is omitted We can use -- to demarcate options from
// non options Long options start with -- They can take options after space or with = Optional
// arguments are possible after = for long options Don't forget to handle non options
//
//
// Concepts
//- Short opts
//- Long opts
//- Arguments
//- Optional arguments

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

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
};

void optget_init(struct optget *optget, int argc, char *argv[],
                 const struct optget_option options[], size_t options_len) {
    optget->argc = argc;
    optget->argv = argv;

    optget->options = options;
    optget->options_len = options_len;

    optget->optarg.opt_ix = 0;
    optget->optarg.arg = NULL;

    optget->nonopt_mode = false;
    optget->arg_ix = 1;

    optget->shortopt_ix = 0;
}

static bool is_dashdash(const char *s) {
    return (s[0] == '-') && (s[1] == '-') && (s[2] == '\0');
}

static bool is_option(const char *s) {
    return (s[0] == '-') && (s[1] != '\0');
}

static const struct optget_option *find_by_shortopt_in_options(char shortopt,
                                                               const struct optget_option options[],
                                                               size_t options_len) {
    for (size_t i = 0; i < options_len; i++) {
        if (options[i].shortopt == shortopt) return &options[i];
    }
    return NULL;
}

static const struct optget_option *find_by_longopt_in_options(char *longopt, size_t longopt_len,
                                                              const struct optget_option options[],
                                                              size_t options_len) {
    for (size_t i = 0; i < options_len; i++) {
        if (memcmp(options[i].longopt, longopt, longopt_len) == 0) { return &options[i]; }
    }
    return NULL;
}

struct optget_optarg *optget_get(struct optget *optget) {
    if (optget->arg_ix >= optget->argc) return NULL;

    char **argv = optget->argv;

	const char *current_arg = optget->argv[optget->arg_ix++];

    if (!optget->nonopt_mode && is_dashdash(argv[optget->arg_ix])) {
        optget->nonopt_mode = true;
        optget->arg_ix++;
        if (optget->arg_ix >= optget->argc) return NULL;
    }

    if (optget->nonopt_mode || !is_option(argv[optget->arg_ix])) {
        optget->optarg.opt_ix = optget->options_len;
        optget->optarg.arg = argv[optget->arg_ix++];
        return &optget->optarg;
    }

    if (argv[optget->arg_ix][1] == '-') {
        // Long option case
        char *longopt = &argv[optget->arg_ix][2];

        char *has_arg = strchr(longopt, '=');
        if ((has_arg != NULL) && (has_arg[1] == '\0')) {
            fprintf(stderr, "Invalid argument for option %s\n", longopt);
            optget->arg_ix = optget->argc;
            return NULL;
        }

        size_t longopt_len = (has_arg == NULL) ? strlen(longopt) + 1 : has_arg - longopt;

        const struct optget_option *option =
            find_by_longopt_in_options(longopt, longopt_len, optget->options, optget->options_len);

        if (option == NULL) {
            printf("Unknown option: %s\n", longopt);
            optget->arg_ix = optget->argc;
            return NULL;
        }

        optget->optarg.opt_ix = option - optget->options;

        if (option->argtype == OPTGET_ARGTYPE_NONE) {
            if (has_arg != NULL) {
                fprintf(stderr, "--%s does not accept arguments\n", longopt);
                optget->arg_ix = optget->argc;
                return NULL;
            }
            optget->optarg.arg = NULL;
            optget->arg_ix++;
        } else if (option->argtype == OPTGET_ARGTYPE_OPTIONAL) {
            optget->optarg.arg = (has_arg != NULL) ? &has_arg[1] : NULL;
            optget->arg_ix++;
        } else if (option->argtype == OPTGET_ARGTYPE_REQUIRED) {
            if (has_arg != NULL) {
                optget->optarg.arg = &has_arg[1];
                optget->arg_ix++;
            } else if (optget->arg_ix + 1 < optget->argc) {
                optget->optarg.arg = argv[++optget->arg_ix];
                optget->arg_ix++;
            } else {
                fprintf(stderr, "--%s requires an argument\n", longopt);
                optget->arg_ix = optget->argc;
                return NULL;
            }
        }
    } else {
        // short option case
        char shortopt = argv[optget->arg_ix][1 + optget->shortopt_ix++];
        char *rest = &argv[optget->arg_ix][1 + optget->shortopt_ix];

        const struct optget_option *option =
            find_by_shortopt_in_options(shortopt, optget->options, optget->options_len);

        // TODO Do better error msgs and handling
        if (option == NULL) {
            printf("Unknown option: %c\n", shortopt);
            optget->arg_ix = optget->argc;
            return NULL;
        }

        optget->optarg.opt_ix = option - optget->options;

        if (option->argtype == OPTGET_ARGTYPE_NONE) {
            optget->optarg.arg = NULL;
            if (rest[0] == '\0') {
                optget->shortopt_ix = 0;
                optget->arg_ix++;
            }
        } else if (option->argtype == OPTGET_ARGTYPE_OPTIONAL) {
            optget->optarg.arg = (rest[0] != '\0') ? rest : NULL;
            optget->shortopt_ix = 0;
            optget->arg_ix++;
        } else if (option->argtype == OPTGET_ARGTYPE_REQUIRED) {
            if (rest[0] != '\0') {
                optget->optarg.arg = rest;
            } else {
                optget->arg_ix++;
                if (optget->arg_ix >= optget->argc) {
                    printf("Option %c requrires an argument\n", shortopt);
                    optget->arg_ix = optget->argc;
                    return NULL;
                }
                optget->optarg.arg = argv[optget->arg_ix];
            }
            optget->shortopt_ix = 0;
            optget->arg_ix++;
        }
    }

    return &optget->optarg;
}

int main(int argc, char *argv[]) {
    const struct optget_option options[] = {
        {.shortopt = 'a', .longopt = "aaa", .argtype = OPTGET_ARGTYPE_NONE},
        {.shortopt = 'b', .longopt = "bbb", .argtype = OPTGET_ARGTYPE_OPTIONAL},
        {.shortopt = 'c', .longopt = "ccc", .argtype = OPTGET_ARGTYPE_REQUIRED},
        {.shortopt = 'd', .longopt = "ddd", .argtype = OPTGET_ARGTYPE_NONE},
    };
    const size_t options_len = sizeof(options) / sizeof(options[0]);

    struct optget optget;
    optget_init(&optget, argc, argv, options, options_len);

    struct optget_optarg *optarg;

    while ((optarg = optget_get(&optget)) != NULL) {
        printf("option_ix: %zu arg: %s\n", optarg->opt_ix, optarg->arg);
    }

    return 0;
}
