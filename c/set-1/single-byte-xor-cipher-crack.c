#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <float.h>

#include "raw.h"
#include "frequency-analysis.h"

void usage(char *);

typedef struct {
    raw_t msg;
    size_t line_num;
    uint8_t xor_key;
    long double variance;
} min_var_msg_t;
#define NUM_MSGS (5)

void crack_xor(raw_t, raw_t, size_t, min_var_msg_t *, size_t);

int main(int argc, char **argv)
{
    min_var_msg_t best_msgs[NUM_MSGS] = {{{NULL,0},0,0,LDBL_MAX}};
    int r = 0;

    raw_t cipher, plain;
    if (argc > 1) {
        size_t num_ciphers = argc - 1;
        while (num_ciphers) {
            char * hex = argv[argc - num_ciphers];
            size_t hex_len = strlen(hex);
            if (0 == hex_len) {
                printf("Error: HEX-ENCODED-STRING is empty\n");
                return 2;
            }
            if (hex_len % 2) {
                printf("Error: HEX-ENCODED-STRING is not of even length\n");
                return 2;
            }
            if ((r = init_raw(&cipher, hex_len/2))) {
                printf("Error: init_raw() returned %d\n", r);
                return 3;
            }
            if ((r = init_raw(&plain, cipher.len))) {
                printf("Error: init_raw() returned %d\n", r);
                return 3;
            }
            if ((r = hex_to_raw(hex, &cipher))) {
                printf("Error: hex_to_raw() returned %d\n", r);
                return 2;
            }
            crack_xor(cipher, plain, argc - num_ciphers, best_msgs, NUM_MSGS);
            num_ciphers--;
        }

        for (size_t m = 0; m < NUM_MSGS; m++) {
            printf("id: %lu xor_key: %d score: %Lf\n",
                    best_msgs[m].line_num, best_msgs[m].xor_key, best_msgs[m].variance);
            size_t num_printed = 0;
            size_t num_nulls = 0;
            do {
                size_t n = printf("'%s' ", best_msgs[m].msg.data + num_printed);
                num_printed += (n - 3);
                num_nulls++;
            } while (num_printed + num_nulls < best_msgs[m].msg.len);
            printf("\n");
        }

        free_raw(&cipher);
        free_raw(&plain);
    } else {
        usage(argv[0]);
    }

    return 0;
}

void crack_xor(raw_t cipher, raw_t plain, size_t id, min_var_msg_t *best_msgs, size_t num_msgs)
{

    for (size_t x = 0; x < 256; x++) {
        uint8_t xor_key = x;
        for (size_t b = 0; b < cipher.len; b++) {
            plain.data[b] = cipher.data[b] ^ xor_key;
        }
        long double var = letter_freq_variance(plain);
        if (var <= best_msgs[0].variance) {
            raw_t tmp;
            tmp.data = best_msgs[num_msgs - 1].msg.data;
            tmp.len  = best_msgs[num_msgs - 1].msg.len;
            for (size_t m = num_msgs - 1; m > 0; m--) {
                if (NULL != best_msgs[m - 1].msg.data) {
                    memcpy(&best_msgs[m], &best_msgs[m - 1], sizeof(min_var_msg_t));
                }
            }

            best_msgs[0].msg.data = tmp.data;
            best_msgs[0].msg.len  = tmp.len;
            init_raw(&best_msgs[0].msg, plain.len);
            memcpy(best_msgs[0].msg.data, plain.data, cipher.len);
            best_msgs[0].variance = var;
            best_msgs[0].xor_key = xor_key;
            best_msgs[0].line_num = id;
        }
    }
}

void usage(char * prgm)
{
    printf("Usage: %s HEX-ENCODED-STRING-1 [HEX-ENCODED-STRING-2 ...]\n", prgm);
}
