#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <argp.h>

#include "raw.h"
#include "convert_argp.h"

int main(int argc, char **argv)
{
    convert_opts_t opts = CONVERT_OPTS_INIT;
    int argv_id = 0;
    int r = 0;
    if ((r = argp_parse(&args, argc, argv, 0, &argv_id, &opts))) {
        return r;
    }

    int num_inputs = opts.argc;
    raw_t *input = NULL;
    switch (opts.source) {
        case SOURCE_ARGS:
            input = (raw_t *)calloc(num_inputs, sizeof(raw_t));
            for (int i = 0; i < num_inputs; i++) {
                input[i].data = (uint8_t *)opts.argv[i];
            }
            break;
        case SOURCE_FILE:
            input = (raw_t *)calloc(num_inputs, sizeof(raw_t));
            for (int i = 0; i < num_inputs; i++) {
                FILE *file = fopen(opts.argv[i], "r");
                if (NULL == file) {
                    int err = errno;
                    fprintf(stderr, "Error: fopen() %d, '%s' '%s'\n",
                            err, strerror(err), opts.argv[i]);
                    return EXIT_FAILURE;
                }

                if (fseek(file, 0L, SEEK_END)) {
                    int err = errno;
                    fprintf(stderr, "Error: fseek() %d, '%s'\n", err, strerror(err));
                    return EXIT_FAILURE;
                }
                size_t file_size = ftell(file);
                rewind(file);

                // Parse file line by line and remove the '\n' chars
                int ret;
                input[i].data = NULL;
                input[i].len = 0;
                if ((ret = init_raw(&input[i], file_size))) {
                    fprintf(stderr, "Error: init_raw() returned %d\n", ret);
                    return EXIT_FAILURE;
                }
                if (ENCODE_CHAR == opts.input && opts.binary) {
                    size_t num_read = fread(input[i].data, 1, file_size, file);
                    if (num_read != file_size) {
                        fprintf(stderr, "Did not read entire file %lu filesize %lu\n",
                                num_read, file_size);
                        return EXIT_FAILURE;
                    }
                    input[i].data[file_size] = '\0';
                    input[i].len = file_size;
                } else {
                    char *line = NULL;
                    size_t input_len = 0;
                    size_t n = 0;
                    int r = 0;
                    errno = 0;
                    while (-1 != (r = getline(&line, &n, file))) {
                        memcpy(input[i].data + input_len, line, r - 1);
                        input_len += (r - 1);
                    }
                    if (errno) {
                        int err = errno;
                        fprintf(stderr, "Error: getline() %d, '%s'", err, strerror(err));
                        return EXIT_FAILURE;
                    }

                    free(line);
                    if (fclose(file)) {
                        int err = errno;
                        fprintf(stderr, "Error: fclose() %d, '%s'", err, strerror(err));
                        return EXIT_FAILURE;
                    }

                    if ((r = init_raw(&input[i], input_len))) {
                        fprintf(stderr, "Error: init_raw() returned %d\n", r);
                        return EXIT_FAILURE;
                    }
                    input[i].data[input_len] = '\0';
                }
            }
            break;
        case SOURCE_STDIN:
            {
                FILE *file = stdin;
                num_inputs = 0;
                char *line = NULL;
                size_t n = 0;
                int r = 0;
                errno = 0;
                if (ENCODE_CHAR == opts.input && opts.binary) {
                    num_inputs = 1;
                    if (NULL == (input = (raw_t *)calloc(1, sizeof(raw_t)))) {
                        int err = errno;
                        fprintf(stderr, "Error: realloc() %d, '%s'", err, strerror(err));
                        return EXIT_FAILURE;
                    }
                    int ret;
                    size_t file_size = 100;
                    size_t num_read = 0;
                    do {
                        if ((ret = init_raw(&input[0], file_size))) {
                            fprintf(stderr, "Error: init_raw() returned %d\n", ret);
                            return EXIT_FAILURE;
                        }
                        num_read += fread(input[0].data, 1, file_size, file);
                        if (num_read == file_size) {
                            file_size += 100;
                        }
                    } while (!feof(file));
                    if ((ret = init_raw(&input[0], num_read))) {
                        fprintf(stderr, "Error: init_raw() returned %d\n", ret);
                        return EXIT_FAILURE;
                    }
                } else {
                    while (-1 != (r = getline(&line, &n, file))) {
                        if (NULL == (input = (raw_t *)realloc(input,
                                        (num_inputs + 1) * sizeof(raw_t)))) {
                            int err = errno;
                            fprintf(stderr, "Error: realloc() %d, '%s'",
                                    err, strerror(err));
                            return EXIT_FAILURE;
                        }
                        int ret;
                        input[num_inputs].data = NULL;
                        input[num_inputs].len = 0;
                        if ((ret = init_raw(&input[num_inputs], r - 1))) {
                            fprintf(stderr, "Error: init_raw() returned %d\n", ret);
                            return EXIT_FAILURE;
                        }
                        memcpy(input[num_inputs].data, line, r - 1);
                        input[num_inputs].len = r-1;
                        num_inputs++;
                    }
                    if (errno) {
                        int err = errno;
                        fprintf(stderr, "Error: getline() %d, '%s'", err, strerror(err));
                        return EXIT_FAILURE;
                    }

                    free(line);
                }
            }
            break;
        default:
            fprintf(stderr, "Error: Invalid source_t source\n");
            return EXIT_FAILURE;
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
                    printf("Error: BASE64-ENCODED-STRING is empty\n");
                    return 2;
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
                        printf("Error: BASE64-ENCODED-STRING improper length\n");
                        return 2;
                    case 2:
                        data_len++;
                        break;
                    case 3:
                        data_len += 2;
                        break;
                }

                int r;
                if ((r = init_raw(&decoded[i], data_len))) {
                    printf("Error: init_raw() returned %d\n", r);
                    return 3;
                }

                if ((r = b64_to_raw((char *)input[i].data, decoded[i]))) {
                    printf("Error: b64_to_raw() returned %d\n", r);
                    return 2;
                }
            }
            break;
        default:
            fprintf(stderr, "Error: Invalid encode_t input\n");
            return EXIT_FAILURE;
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
            fprintf(ofile, "%s%s", output[i],
                    i == (num_inputs - 1) ? "\n": "");
        }
    }

    return EXIT_SUCCESS;
}
