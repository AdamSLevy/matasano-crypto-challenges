#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <argp.h>

#include "raw.h"
#include "convert_argp.h"

#define BUF_SIZE (128)

#define PRINT_LINE() \
    fprintf(stderr,"%10s:%-3d %s%-2s", __FILE__, __LINE__, __func__,"\n")

int main(int argc, char **argv)
{
    convert_opts_t opts = CONVERT_OPTS_INIT;
    if (argp_parse(&args, argc, argv, 0, NULL, &opts)) {
        return EXIT_FAILURE;
    }

    int num_inputs;
    raw_t *input = NULL;
    if (SOURCE_ARGS == opts.source) {
        num_inputs = opts.argc;
        input = (raw_t *)calloc(num_inputs, sizeof(raw_t));
        for (int i = 0; i < num_inputs; i++) {
            size_t len = strlen(opts.argv[i]);
            if (init_raw(&input[i], len)) {
                fprintf(stderr, "Error: init_raw()\n");
                return EXIT_FAILURE;
            }
            memcpy(input[i].data, opts.argv[i], len);
        }
    } else {
        int num_files;
        num_inputs = 0;
        if (SOURCE_FILE == opts.source) {
            num_files = opts.argc;
        } else {
            num_files = 1;
        }
        FILE *ifile = stdin;
        for (int i = 0; i < num_files; i++) {
            if (SOURCE_FILE == opts.source) {
                ifile = fopen(opts.argv[i], "r");
                if (NULL == ifile) {
                    int err = errno;
                    fprintf(stderr, "Error: fopen() %d, '%s' '%s'\n",
                            err, strerror(err), opts.argv[i]);
                    return EXIT_FAILURE;
                }
            }

            if (NULL == (input = (raw_t *)realloc(input,
                            (num_inputs + 1) * sizeof(raw_t)))) {
                int err = errno;
                fprintf(stderr, "Error: realloc() %d, '%s'",
                        err, strerror(err));
                return EXIT_FAILURE;
            }
            input[num_inputs] = INIT_RAW;

            if (ENCODE_CHAR == opts.input && opts.binary) {
                size_t buf_len = BUF_SIZE;
                size_t num_read = 0;
                do {
                    if (init_raw(&input[num_inputs], buf_len)) {
                        fprintf(stderr, "Error: init_raw()\n");
                        return EXIT_FAILURE;
                    }

                    size_t read = fread(&input[num_inputs].data[num_read],
                            1, BUF_SIZE, ifile);
                    num_read += read;
                    if (read == BUF_SIZE) {
                        buf_len += BUF_SIZE;
                    }
                } while (!feof(ifile));
                if (init_raw(&input[num_inputs], num_read)) {
                    fprintf(stderr, "Error: init_raw()\n");
                    return EXIT_FAILURE;
                }
                num_inputs++;
            } else {
                char *line = NULL;
                size_t line_len = 0;
                size_t input_len = 0;
                int r;
                while (-1 != (r = getline(&line, &line_len, ifile))) {
                    if (opts.newline) {
                        if (NULL == (input = (raw_t *)realloc(input,
                                        (num_inputs + 1) * sizeof(raw_t)))) {
                            int err = errno;
                            fprintf(stderr, "Error: realloc() %d, '%s'",
                                    err, strerror(err));
                            return EXIT_FAILURE;
                        }
                        input[num_inputs].data = (uint8_t *)line;
                        if (init_raw(&input[num_inputs], r - 1)) {
                            fprintf(stderr, "Error: init_raw()\n");
                            return EXIT_FAILURE;
                        }
                        line = NULL;
                        line_len = 0;
                        num_inputs++;
                    } else {
                        if (init_raw(&input[num_inputs], input_len + r - 1)) {
                            fprintf(stderr, "Error: init_raw()\n");
                            return EXIT_FAILURE;
                        }
                        memcpy(&input[num_inputs].data[input_len], line, r - 1);
                        input_len += r - 1;
                    }
                }
                if (errno) {
                    int err = errno;
                    fprintf(stderr, "Error: getline() %d, '%s'", err, strerror(err));
                    return EXIT_FAILURE;
                }
                if (!opts.newline) {
                    num_inputs++;
                }
            }
            if (fclose(ifile)) {
                int err = errno;
                fprintf(stderr, "Error: fclose() %d, '%s'", err, strerror(err));
                return EXIT_FAILURE;
            }
        }
    }

    raw_t *decoded;
    if (ENCODE_CHAR != opts.input) {
        decoded = (raw_t *)calloc(num_inputs, sizeof(raw_t));
    }
    switch (opts.input) {
        case ENCODE_CHAR:
            decoded = input;
            break;
        case ENCODE_HEX:
            for (int i = 0; i < num_inputs; i++) {
                size_t hex_len = input[i].len;

                if (0 == hex_len) {
                    fprintf(stderr, "Error: HEX-ENCODED-STRING is empty\n");
                    return EXIT_FAILURE;
                }

                if (hex_len % 2) {
                    fprintf(stderr, "Error: HEX-ENCODED-STRING is not of even length\n");
                    return EXIT_FAILURE;
                }

                int r;
                if ((r = init_raw(&decoded[i], hex_len/2))) {
                    fprintf(stderr, "Error: init_raw() returned %d\n", r);
                    return EXIT_FAILURE;
                }

                if ((r = hex_to_raw((char *)input[i].data, decoded[i]))) {
                    fprintf(stderr, "Error: hex_to_raw() returned %d\n", r);
                    return EXIT_FAILURE;
                }
            }
            break;
        case ENCODE_BASE64:
            for (int i = 0; i < num_inputs; i++) {
                size_t b64_len = input[i].len;
                if (0 == b64_len) {
                    fprintf(stderr, "Error: BASE64-ENCODED-STRING is empty\n");
                    return EXIT_FAILURE;
                }
                size_t data_len = 3 * (b64_len / 4);
                switch (b64_len % 4) {
                    case 0:
                        for (int e = 0; e < 2; e++) {
                            if ('=' == input[i].data[b64_len - (e + 1)]) {
                                data_len--;
                            }
                        }
                        break;
                    case 1:
                        fprintf(stderr, "Error: BASE64-ENCODED-STRING improper length\n");
                        return EXIT_FAILURE;
                    case 2:
                        data_len++;
                        break;
                    case 3:
                        data_len += 2;
                        break;
                }

                if (init_raw(&decoded[i], data_len)) {
                    fprintf(stderr, "Error: init_raw()\n");
                    return EXIT_FAILURE;
                }

                if (b64_to_raw((char *)input[i].data, decoded[i])) {
                    fprintf(stderr, "Error: b64_to_raw()\n");
                    return EXIT_FAILURE;
                }
            }
            break;
        default:
            fprintf(stderr, "Error: Invalid encode_t input\n");
            return EXIT_FAILURE;
    }

    if (opts.pad) {
        for (int i = 0; i < num_inputs; i++) {
            size_t len = decoded[i].len;
            uint8_t add_pad = opts.pad - (len % opts.pad);
            if (init_raw(&decoded[i], len + add_pad)) {
                fprintf(stderr, "Error: init_raw()\n");
                return EXIT_FAILURE;
            }
            for (int j = 0; j < add_pad; j++) {
                decoded[i].data[len + j] = add_pad;
            }
        }
    }

    char **output = (char **)malloc(num_inputs * sizeof(char **));
    switch (opts.output) {
        case ENCODE_CHAR:
            for (int i = 0; i < num_inputs; i++) {
                output[i] = (char *)decoded[i].data;
            }
            break;
        case ENCODE_HEX:
            for (int i = 0; i < num_inputs; i++) {
                output[i] = (char *)malloc(decoded[i].len * 2 + 1);
                raw_to_hex(decoded[i], output[i]);
            }
            break;
        case ENCODE_BASE64:
            for (int i = 0; i < num_inputs; i++) {
                size_t b64_len = 4 * (decoded[i].len / 3) +
                    4 * (bool)(decoded[i].len % 3);
                output[i] = (char *)malloc(b64_len + 1);
                raw_to_b64(decoded[i], output[i]);
            }
            break;
        default:
            fprintf(stderr, "Error: Invalid encode_t input\n");
            return EXIT_FAILURE;
    }

    FILE *ofile = NULL;
    switch (opts.dest) {
        case DEST_STDOUT:
            ofile = stdout;
            break;
        case DEST_FILE:
            ofile = fopen(opts.output_file, "w");
            if (NULL == ofile) {
                int err = errno;
                fprintf(stderr, "Error: fopen() %d, '%s' '%s'\n",
                        err, strerror(err), opts.output_file);
                return EXIT_FAILURE;
            }
            break;
        default:
            fprintf(stderr, "Error: Invalid dest_t dest\n");
            return EXIT_FAILURE;
    }

    for (int i = 0; i < num_inputs; i++) {
        if (ENCODE_CHAR == opts.output && opts.binary) {
            fwrite(output[i], decoded[i].len, sizeof(uint8_t), ofile);
        } else {
            fputs(output[i], ofile);
            if (!opts.concat ||
                    (DEST_STDOUT == opts.dest && i == (num_inputs - 1))) {
               fputc('\n', ofile);
            }
        }
    }

    return EXIT_SUCCESS;
}
