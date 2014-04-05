#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#include "common.h"
#include "args.h"

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
    static const char *opts = "m:s:u:o:k:h";
    static const struct option lopts[] = {
        { "mode",   required_argument, NULL, 'm' },
        { "sigma",  required_argument, NULL, 's' },
        { "mu",     required_argument, NULL, 'u' },
        { "output", required_argument, NULL, 'o' },
        { "key",    required_argument, NULL, 'k' },
        { "help",   no_argument,       NULL, 'h' },
        { NULL,     0,                 NULL, 0   }
    };
    
    ee_int_t status = EE_SUCCESS;
    
    ee_bool_t mode_specified = EE_FALSE;
    ee_bool_t sigma_specified = EE_FALSE;
    ee_bool_t mu_specified = EE_FALSE;
    ee_bool_t output_specified = EE_FALSE;
    
    args->mode = EE_MODE_ENCRYPT;
    args->sigma = 8;
    args->mu = 0;
    args->key = NULL;
    args->input_file = NULL;
    args->output_file = "a.out";
    
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
            
            if (1 > args->sigma || 16 < args->sigma) {
                fprintf(stderr, "%s: '--sigma' must be in range [1; 16]\n",
                        argv[0]);
                EE_SEE_HELP(argv[0]);
                status = EE_FAILURE;
                goto end;
            }
            
            sigma_specified = EE_TRUE;
            break;
        case 'u':
            EE_CHECK_OPTARG(argv[0], "'--mu'", status, end);
            args->mu = atoi(optarg);
            if (0 == args->sigma && '0' != optarg[0]) {
                fprintf(stderr, "%s: '--mu' must be an integer value\n",
                        argv[0]);
                EE_SEE_HELP(argv[0]);
                status = EE_FAILURE;
                goto end;
            }
            
            if (255 < args->mu) {
                fprintf(stderr, "%s: '--mu' must be in range [0; 255]\n",
                        argv[0]);
                EE_SEE_HELP(argv[0]);
                status = EE_FAILURE;
                goto end;
            }
            
            mu_specified = EE_TRUE;
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
            break;
        default:
            status = EE_FAILURE;
            goto end;
            break;
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
        EE_USED_DEFAULT_VALUE(argv[0], "'--mode'", "encrypt");
    }
    
    if (EE_FALSE == sigma_specified) {
        EE_USED_DEFAULT_VALUE(argv[0], "'--sigma'", "8");
    }
    
    if (EE_FALSE == mu_specified) {
        EE_USED_DEFAULT_VALUE(argv[0], "'--mu'", "0");
    }
    
    if (EE_FALSE == output_specified) {
        EE_USED_DEFAULT_VALUE(argv[0], "'--output'", args->output_file);
    }
    
end:
    return status;
}

static void
ee_print_help_msg_s(void)
{
    printf("Usage: ee [options] file\n");
    printf("\n");
    printf("where possible options include:\n");
    printf("\t-m, --mode=[encrypt|decrypt] \tspecifies the mode; 'encrypt' for encrypting the source\n"
           "\t                             \tinput file end writing result to output file and 'decrypt'\n"
           "\t                             \tfor decrypt the encrypted input file and writing result\n"
           "\t                             \tto output file; also allowed the reduction;\n"
           "\t                             \t'encrypt' by default\n");
    printf("\t-s, --sigma=[VALUE]          \tspecifies the block size is calculated as 2^VALUE;\n"
           "\t                             \tthe valid values in range [1; 16]; '8' by default\n");
    printf("\t-u, --mu=[VALUE]             \tspecifies the memory size in initial message source;\n"
           "\t                             \tif this value is 0 then initial message source considered\n"
           "\t                             \tto message source without memory; the value must be in\n"
           "\t                             \trange [0; 255]; '0' by default\n");
    printf("\t-o, --output=[FILE]          \tspecifies the output file to which to write the result\n"
           "\t                             \tof the encryption or decryption (depending on the --mode);\n"
           "\t                             \t'a.out' by default\n");
    printf("\t-k, --key=[KEY]              \tspecifies the secret key for encryption or decryption\n"
           "\t                             \tthe message\n");
    printf("\t-h, --help                   \tshow this help message\n");
    printf("\n");
}
