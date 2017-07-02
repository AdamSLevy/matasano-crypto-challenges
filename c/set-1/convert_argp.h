#ifndef CONVERT_ARGP_H_
#define CONVERT_ARGP_H_

#include <argp.h>
#include <stdbool.h>

extern const struct argp_option options[];
extern const struct argp args;

typedef enum {
    ENCODE_CHAR,
    ENCODE_HEX,
    ENCODE_BASE64,
} encoding_t;

typedef enum {
    SOURCE_ARGS,
    SOURCE_FILE,
    SOURCE_STDIN,
} source_t;

typedef enum {
    DEST_FILE,
    DEST_STDOUT,
} dest_t;

typedef struct {
    encoding_t input;
    encoding_t output;
    source_t source;
    dest_t dest;
    char *output_file;
    int argc;
    char **argv;
    bool binary;
    int pad;
} convert_opts_t;

#define CONVERT_OPTS_INIT (convert_opts_t){ \
    ENCODE_CHAR, \
    ENCODE_CHAR, \
    SOURCE_STDIN, \
    DEST_STDOUT, \
    NULL, \
    0, \
    NULL, \
    false, \
    0, \
}

int parse_opt(int key, char *arg, struct argp_state *state);

#endif // CONVERT_ARGP_H_
