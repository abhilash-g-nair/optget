#include "optget.h"
#include <stdio.h>
#include <string.h>

static bool is_dashdash(const char *s) {
    return (s[0] == '-') && (s[1] == '-') && (s[2] == '\0');
}

static bool is_option(const char *s) {
    return (s[0] == '-') && (s[1] != '\0');
}

static char *find_char(const char *s, char c) {
    while (*s != '\0') {
        if (*s == c) return (char *)s;
        s++;
    }
    return NULL;
}

static void *optget_error(struct optget *optget) {
    optget->error_flag = true;
    optget->arg_ix = optget->argc;
    return NULL;
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
        if ((strlen(options[i].longopt) == longopt_len) &&
            (strncmp(options[i].longopt, longopt, longopt_len) == 0)) {
            return &options[i];
        }
    }
    return NULL;
}

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
    optget->error_flag = false;
}

struct optget_optarg *optget_get(struct optget *optget) {
    if (optget->arg_ix >= optget->argc) return NULL;

    char *current_arg = optget->argv[optget->arg_ix];

    if (!optget->nonopt_mode && is_dashdash(current_arg)) {
        optget->nonopt_mode = true;
        if (optget->arg_ix + 1 >= optget->argc) return NULL;
        current_arg = optget->argv[++optget->arg_ix];
    }

    if (optget->nonopt_mode || !is_option(current_arg)) {
        optget->arg_ix++;
        optget->optarg.opt_ix = optget->options_len;
        optget->optarg.arg = current_arg;
        return &optget->optarg;
    }

    if (current_arg[1] == '-') {
        // Long option case
        char *longopt = current_arg;
        char *arg_separator = find_char(longopt, '=');

        if ((arg_separator != NULL) && (arg_separator[1] == '\0')) {
            sprintf(optget->err_msg, "argument expected after = for %s", longopt);
            return optget_error(optget);
        }

        size_t longopt_len =
            (arg_separator == NULL) ? strlen(longopt + 2) : arg_separator - (longopt + 2);

        const struct optget_option *option = find_by_longopt_in_options(
            longopt + 2, longopt_len, optget->options, optget->options_len);

        if (option == NULL) {
            sprintf(optget->err_msg, "unknown option %s", longopt);
            return optget_error(optget);
        }

        optget->optarg.opt_ix = option - optget->options;

        if ((option->argtype == OPTGET_ARGTYPE_NONE) && (arg_separator != NULL)) {
            sprintf(optget->err_msg, "--%s does not accept arguments", option->longopt);
            return optget_error(optget);
        }

        switch (option->argtype) {
        case OPTGET_ARGTYPE_NONE:
            optget->optarg.arg = NULL;
            break;
        case OPTGET_ARGTYPE_OPTIONAL:
            optget->optarg.arg = (arg_separator != NULL) ? arg_separator + 1 : NULL;
            break;
        case OPTGET_ARGTYPE_REQUIRED:
        default:
            if (arg_separator != NULL) {
                optget->optarg.arg = arg_separator + 1;
            } else if (optget->arg_ix + 1 < optget->argc) {
                optget->optarg.arg = optget->argv[++optget->arg_ix];
            } else {
                sprintf(optget->err_msg, "%s requires an argument", longopt);
                return optget_error(optget);
            }
            break;
        }
        optget->arg_ix++;
        return &optget->optarg;

    } else {
        // short option case
        char shortopt = current_arg[1 + optget->shortopt_ix];
        char *rest = &current_arg[2 + optget->shortopt_ix];
        optget->shortopt_ix++;

        const struct optget_option *option =
            find_by_shortopt_in_options(shortopt, optget->options, optget->options_len);

        if (option == NULL) {
            sprintf(optget->err_msg, "unknown option '%c'", shortopt);
            return optget_error(optget);
        }

        optget->optarg.opt_ix = option - optget->options;

        switch (option->argtype) {
        case OPTGET_ARGTYPE_NONE:
            optget->optarg.arg = NULL;
            if (rest[0] == '\0') {
                optget->shortopt_ix = 0;
                optget->arg_ix++;
            }
            break;
        case OPTGET_ARGTYPE_OPTIONAL:
            optget->optarg.arg = (rest[0] != '\0') ? rest : NULL;
            optget->shortopt_ix = 0;
            optget->arg_ix++;
            break;
        case OPTGET_ARGTYPE_REQUIRED:
        default:
            if (rest[0] != '\0') {
                optget->optarg.arg = rest;
            } else if (optget->arg_ix + 1 < optget->argc) {
                optget->optarg.arg = optget->argv[++optget->arg_ix];
                optget->shortopt_ix = 0;
                optget->arg_ix++;
            } else {
                sprintf(optget->err_msg, "'%c' requires an argument", shortopt);
                return optget_error(optget);
            }
            optget->shortopt_ix = 0;
            optget->arg_ix++;
            break;
        }

        return &optget->optarg;
    }
}

char *optget_get_error(struct optget *optget) {
    return optget->error_flag ? optget->err_msg : NULL;
}
