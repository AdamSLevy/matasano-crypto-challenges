#include <string.h>
#include <stdlib.h>
#include "convert_argp.h"

const char * argp_program_version = "v1.1.0";
const char * argp_program_bug_address = "<adam@aslevy.com>";

const struct argp args = {
    options,
    parse_opt,
    "[INPUT1 [INPUT2 [...]]]",
    "Convert Hex, Base64, Char (raw)\v"
        "Input (--decode) and output (--encode) encoding default to CHAR. "
        "If no INPUTs are passed then input is read from STDIN. ",
    0,
    0,
    0,
};

const struct argp_option options[] = {
    {.doc = "Set input and output encodings", .group = 1},
    {
        "encode",
        'e',
        "HEX|B64|CHAR",
        0,
        "Set output encoding",
        1
    },
    {
        "decode",
        'd',
        "HEX|B64|CHAR",
        0,
        "Set input encoding",
        1
    },
    {.doc = "Parse options", .group = 2},
    {
        "binary",
        'b',
        0,
        0,
        "Parse files and output data as binary (includes '\\n' and '\\0' bytes). "
            "only meaningful when reading or writing to a file with char encoding",
        2,
    },
    {
        "pad",
        'p',
        "BLOCKSIZE",
        OPTION_ARG_OPTIONAL,
        "Pad data to BLOCKSIZE according to PKCS#7. BLOCKSIZE defaults to 16",
        2,
    },
    {.doc = "Set input source and output destination", .group = 3},
    {
        "file",
        'f',
        0,
        0,
        "INPUTs are parsed as list of files. "
            "Newlines in files are ignored.",
        3,
    },
    {
        "output",
        'o',
        "FILE",
        0,
        "Output to FILE",
        3,
    },
    {0,0,0,0,0,0}
};

int parse_opt(int key, char *arg, struct argp_state *state)
{
    convert_opts_t *opts = (convert_opts_t *)state->input;
    size_t arg_len;
    switch (key) {
        case 'd':
            arg_len = strlen(arg);
            if (!strncasecmp(arg, "char", arg_len) ||
                    !strncasecmp(arg, "raw", arg_len) ||
                    !strncasecmp(arg, "ascii", arg_len)) {
                    opts->input = ENCODE_CHAR;
            } else if (!strncasecmp(arg, "64", arg_len) ||
                    !strncasecmp(arg, "b64", arg_len) ||
                    !strncasecmp(arg, "base64", arg_len)) {
                    opts->input = ENCODE_BASE64;
            } else if (!strncasecmp(arg, "hex", arg_len) ||
                    !strncasecmp(arg, "16", arg_len) ||
                    !strncasecmp(arg, "0x", arg_len) ||
                    !strncasecmp(arg, "x", arg_len)) {
                    opts->input = ENCODE_HEX;
            } else {
                argp_error(state, "Could not parse encoding %s", arg);
            }
            break;
        case 'e':
            arg_len = strlen(arg);
            if (!strncasecmp(arg, "char", arg_len) ||
                    !strncasecmp(arg, "raw", arg_len) ||
                    !strncasecmp(arg, "ascii", arg_len)) {
                    opts->output = ENCODE_CHAR;
            } else if (!strncasecmp(arg, "64", arg_len) ||
                    !strncasecmp(arg, "b64", arg_len) ||
                    !strncasecmp(arg, "base64", arg_len)) {
                    opts->output = ENCODE_BASE64;
            } else if (!strncasecmp(arg, "hex", arg_len) ||
                    !strncasecmp(arg, "base16", arg_len) ||
                    !strncasecmp(arg, "b16", arg_len) ||
                    !strncasecmp(arg, "16", arg_len) ||
                    !strncasecmp(arg, "0x", arg_len) ||
                    !strncasecmp(arg, "x", arg_len)) {
                    opts->output = ENCODE_HEX;
            } else {
                argp_error(state, "Could not parse encoding %s", arg);
            }
            break;
        case 'f':
            opts->source = SOURCE_FILE;
            break;
        case 'o':
            opts->dest = DEST_FILE;
            opts->output_file = arg;
            break;
        case 'b':
            opts->binary = true;
            break;
        case 'p':
            if (NULL == arg) {
                opts->pad = 16;
            } else {
                errno = 0;
                char * end = NULL;
                int pad = strtol(arg, &end, 10);
                if (errno == 0 && *end == '\0') {
                    if (2 > pad || pad >= 256) {
                        argp_error(state, "Invalid padding BLOCKSIZE");
                    }
                    opts->pad = pad;
                } else {
                    argp_error(state, "Could not parse padding BLOCKSIZE");
                }
            }
            break;
        case ARGP_KEY_ARG:
            if (SOURCE_FILE != opts->source) {
                opts->source = SOURCE_ARGS;
            }
            opts->argc = state->argc - (state->next - 1);
            opts->argv = &state->argv[state->next - 1];
            state->next = state->argc;
            break;
        case ARGP_KEY_NO_ARGS:
            if (SOURCE_FILE == opts->source) {
                argp_error(state, "At least one file must be specified with --file");
            }
            opts->source = SOURCE_STDIN;
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}
