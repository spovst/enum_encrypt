#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#include "common.h"
#include "args.h"

#define EE_TO_STR_HELPER(arg) #arg
#define EE_TO_STR(arg) EE_TO_STR_HELPER(arg)

#define EE_MODE_DEFAULT EE_MODE_ENCRYPT
#define EE_MODE_DEFAULT_STR "encrypt"

#define EE_SIGMA_DEFAULT 8
#define EE_SIGMA_DEFAULT_STR EE_TO_STR(EE_SIGMA_DEFAULT)
#define EE_SIGMA_MIN 1
#define EE_SIGMA_MAX 16

#define EE_MU_DEFAULT 0
#define EE_MU_DEFAULT_STR EE_TO_STR(EE_MU_DEFAULT)
#define EE_MU_MIN 0
#define EE_MU_MAX 255

#define EE_OUTPUT_FILE_DEFAULT "a.out"
#define EE_OUTPUT_FILE_DEFAULT_STR EE_OUTPUT_FILE_DEFAULT

#define EE_SEE_HELP_MSG "see --help for more information"
#define EE_SEE_HELP(prog) \
        do { \
            fprintf(stderr, "%s: %s\n", (prog), EE_SEE_HELP_MSG); \
        } while (0)

#define EE_CHECK_OPTARG(prog, option, status, label) \
        do { \
            if (NULL == optarg) { \
                fprintf(stderr, "%s: option %s required argument\n", (prog), \
                        (option)); \
                fprintf(stderr, EE_SEE_HELP_MSG); \
                status = EE_FAILURE; \
                goto label; \
            } \
        } while (0)

#define EE_OPTION_REQUIRED(prog, option, status, label) \
        do { \
            fprintf(stderr, "%s: %s required\n", (prog), (option)); \
            EE_SEE_HELP((prog)); \
            (status) = EE_FAILURE; \
            goto label; \
        } while (0)

#define EE_USED_DEFAULT_VALUE(prog, option, value) \
        do { \
            printf("%s: %s does not specified; default value '%s' is used\n", \
                    (prog), (option), (value)); \
        } while (0)

static void
ee_print_help_msg_s(void);

ee_int_t
ee_args_parse(ee_args_t *args, int argc, char *argv[])
{
    static const char *opts = "m:s:u:dpo:k:h";
    static const struct option lopts[] = {
        { "mode",         required_argument, NULL, 'm' },
        { "sigma",        required_argument, NULL, 's' },
        { "mu",           required_argument, NULL, 'u' },
        { "dump-sources", no_argument,       NULL, 'd' },
        { "part",         no_argument,       NULL, 'p' },
        { "output",       required_argument, NULL, 'o' },
        { "key",          required_argument, NULL, 'k' },
        { "help",         no_argument,       NULL, 'h' },
        { NULL,           0,                 NULL, 0   }
    };

    ee_int_t status = EE_SUCCESS;

    ee_bool_t mode_specified = EE_FALSE;
    ee_bool_t sigma_specified = EE_FALSE;
    ee_bool_t mu_specified = EE_FALSE;
    ee_bool_t dump_sources_specified = EE_FALSE;
    ee_bool_t output_specified = EE_FALSE;

    args->mode = EE_MODE_DEFAULT;
    args->sigma = EE_SIGMA_DEFAULT;
    args->mu = EE_MU_DEFAULT;
    args->dump_sources = EE_FALSE;
    args->part = EE_FALSE;
    args->key = NULL;
    args->input_file = NULL;
    args->output_file = EE_OUTPUT_FILE_DEFAULT;

    int c;
    while (-1 != (c = getopt_long(argc, argv, opts, lopts, NULL))) {
        ee_size_t arg_len;
        switch (c) {
        case 'm':
            EE_CHECK_OPTARG(argv[0], "'--mode'", status, end);
            arg_len = strlen(optarg);
            if (0 == strncmp(optarg, "encrypt", arg_len)) {
                args->mode = EE_MODE_ENCRYPT;
            } else if (0 == strncmp(optarg, "decrypt", arg_len)) {
                args->mode = EE_MODE_DECRYPT;
            } else {
                fprintf(stderr, "%s: '--mode' requires 'encrypt' or 'decrypt' "
                        "(or their reduction)\n", argv[0]);
                EE_SEE_HELP(argv[0]);
                status = EE_FAILURE;
                goto end;
            }

            mode_specified = EE_TRUE;
            break;
        case 's':
            EE_CHECK_OPTARG(argv[0], "'--sigma'", status, end);
            args->sigma = atoi(optarg);
            if (0 == args->sigma && '0' != optarg[0]) {
                fprintf(stderr, "%s: '--sigma' must be an integer value\n",
                        argv[0]);
                EE_SEE_HELP(argv[0]);
                status = EE_FAILURE;
                goto end;
            }

            if (EE_SIGMA_MIN > args->sigma || EE_SIGMA_MAX < args->sigma) {
                fprintf(stderr, "%s: '--sigma' must be in range [%d; %d]\n",
                        argv[0], EE_SIGMA_MIN, EE_SIGMA_MAX);
                EE_SEE_HELP(argv[0]);
                status = EE_FAILURE;
                goto end;
            }

            sigma_specified = EE_TRUE;
            break;
        case 'u':
            EE_CHECK_OPTARG(argv[0], "'--mu'", status, end);
            args->mu = atoi(optarg);
            if (0 == args->mu && '0' != optarg[0]) {
                fprintf(stderr, "%s: '--mu' must be an integer value\n",
                        argv[0]);
                EE_SEE_HELP(argv[0]);
                status = EE_FAILURE;
                goto end;
            }

            if (EE_MU_MIN > args->mu || EE_MU_MAX < args->mu) {
                fprintf(stderr, "%s: '--mu' must be in range [%d; %d]\n",
                        argv[0], EE_MU_MIN, EE_MU_MAX);
                EE_SEE_HELP(argv[0]);
                status = EE_FAILURE;
                goto end;
            }

            mu_specified = EE_TRUE;
            break;
        case 'd':
            args->dump_sources = EE_TRUE;
            dump_sources_specified = EE_TRUE;
            break;
        case 'p':
            args->part = EE_TRUE;
            break;
        case 'o':
            EE_CHECK_OPTARG(argv[0], "'--output'", status, end);
            args->output_file = optarg;
            output_specified = EE_TRUE;
            break;
        case 'k':
            EE_CHECK_OPTARG(argv[0], "'--key'", status, end);
            args->key = optarg;
            break;
        case 'h':
            ee_print_help_msg_s();
            status = EE_FAILURE;
            goto end;
        default:
            status = EE_FAILURE;
            goto end;
        }
    }

    if (optind == argc) {
        EE_OPTION_REQUIRED(argv[0], "input file", status, end);
    }

    if (NULL == args->key) {
        EE_OPTION_REQUIRED(argv[0], "'--key'", status, end);
    }

    args->input_file = argv[optind];

    if (EE_FALSE == mode_specified) {
        EE_USED_DEFAULT_VALUE(argv[0], "'--mode'", EE_MODE_DEFAULT_STR);
    }

    if (EE_FALSE == sigma_specified) {
        EE_USED_DEFAULT_VALUE(argv[0], "'--sigma'", EE_SIGMA_DEFAULT_STR);
    }

    if (EE_FALSE == mu_specified) {
        EE_USED_DEFAULT_VALUE(argv[0], "'--mu'", EE_MU_DEFAULT_STR);
    }

    if (EE_TRUE == dump_sources_specified && EE_MODE_DECRYPT == args->mode) {
        printf("%s: '--dump-sources' has no effect in decryption mode\n",
                argv[0]);
    }

    if (EE_FALSE == output_specified) {
        EE_USED_DEFAULT_VALUE(argv[0], "'--output'", EE_OUTPUT_FILE_DEFAULT_STR);
    }

end:
    return status;
}

static void
ee_print_help_msg_s(void)
{
    printf("Usage: ee [options] INPUT\n");
    printf("\n");
    printf("where possible options include:\n");
    printf("\t-m, --mode=[encrypt|decrypt] \tspecifies the mode; 'encrypt' for encrypting the source\n"
           "\t                             \tinput file and writing result to output file, and 'decrypt'\n"
           "\t                             \tfor decrypt the encrypted input file and writing result\n"
           "\t                             \tto output file; also allowed the reduction;\n"
           "\t                             \t'%s' by default\n", EE_MODE_DEFAULT_STR);
    printf("\t-s, --sigma=[VALUE]          \tspecifies the block size is calculated as 2^VALUE;\n"
           "\t                             \tthe valid values in range [%d; %d]; '%d' by default\n",
           EE_SIGMA_MIN, EE_SIGMA_MAX, EE_SIGMA_DEFAULT);
    printf("\t-u, --mu=[VALUE]             \tspecifies the memory size in initial message source;\n"
           "\t                             \tif this value is 0 then initial message source considered\n"
           "\t                             \tto message source without memory; the value must be in\n"
           "\t                             \trange [%d; %d]; '%d' by default\n",
           EE_MU_MIN, EE_MU_MAX, EE_MU_DEFAULT);
    printf("\t-d, --dump-sources           \tin encryption mode creates file 'sources.dump' with result of\n"
           "\t                             \tsource splitting; in decryption mode has no effect\n");
    printf("\t-p, --part                   \tin encryption mode splits output into two files - FILE.pri and\n"
           "\t                             \tFILE.pub which contains private data (encrypted subnumbers) and\n"
           "\t                             \tpublic data (statistics, prefixes, etc.); in decryption mode\n"
           "\t                             \tspecifies that public and private data need to take from two\n"
           "\t                             \tdifferent files INPUT.pub and INPUT.pri files\n");
    printf("\t-o, --output=[FILE]          \tspecifies the output file to which to write the result\n"
           "\t                             \tof the encryption or decryption (depending on the --mode);\n"
           "\t                             \t'%s' by default\n", EE_OUTPUT_FILE_DEFAULT);
    printf("\t-k, --key=[KEY]              \tspecifies the secret key for encryption or decryption\n"
           "\t                             \tthe message\n");
    printf("\t-h, --help                   \tshow this help message\n");
    printf("\n");
}
