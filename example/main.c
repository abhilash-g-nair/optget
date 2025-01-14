#include "optget.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
    const struct optget_option options[] = {
        {.shortopt = 'a', .longopt = "aaa", .argtype = OPTGET_ARGTYPE_NONE},
        {.shortopt = 'b', .longopt = "bbb", .argtype = OPTGET_ARGTYPE_OPTIONAL},
        {.shortopt = 'c', .longopt = "ccc", .argtype = OPTGET_ARGTYPE_REQUIRED},
        {.shortopt = 'd', .longopt = "dddd", .argtype = OPTGET_ARGTYPE_NONE},
    };
    const size_t options_len = sizeof(options) / sizeof(options[0]);

    struct optget optget;
    optget_init(&optget, argc, argv, options, options_len);

    struct optget_optarg *optarg;

    while ((optarg = optget_get(&optget)) != NULL) {
        printf("option_ix: %zu arg: %s\n", optarg->opt_ix, optarg->arg);
    }

    if (optget_get_error(&optget) != NULL) {
        fprintf(stderr, "%s: %s\n", argv[0], optget_get_error(&optget));
        return -1;
    }

    return 0;
}
