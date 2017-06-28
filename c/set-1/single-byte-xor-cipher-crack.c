#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "raw.h"
#include "frequency-analysis.h"

void usage(char *);

typedef struct {
    raw_t msg;
    uint8_t xor_key;
    long double variance;
} min_var_msg_t;
#define NUM_MSGS (2)

int main(int argc, char **argv)
{
    if (argc != 2) {
        usage(argv[0]);
        return 1;
    }

    char *hex = argv[1];
    size_t hex_len = strlen(hex);

    if (0 == hex_len) {
        printf("Error: HEX-ENCODED-STRING is empty\n");
        return 2;
    }

    if (hex_len % 2) {
        printf("Error: HEX-ENCODED-STRING is not of even length\n");
        return 2;
    }

    int r;
    raw_t cipher, plain;
    if ((r = init_raw(&cipher, hex_len/2))) {
        printf("Error: init_raw() returned %d\n", r);
        return 3;
    }

    if ((r = init_raw(&plain, hex_len/2))) {
        printf("Error: init_raw() returned %d\n", r);
        return 3;
    }

    if ((r = hex_to_raw(hex, &cipher))) {
        printf("Error: hex_to_raw() returned %d\n", r);
        return 2;
    }

    min_var_msg_t best_msgs[NUM_MSGS];
    for (size_t m = 0; m < NUM_MSGS; m++) {
        if ((r = init_raw(&best_msgs[m].msg, hex_len/2))) {
            printf("Error: init_raw() returned %d\n", r);
            return 3;
        }
        best_msgs[m].xor_key = 0;
        best_msgs[m].variance = 100000;
    }

    for (size_t x = 0; x < 256; x++) {
        uint8_t xor_key = x;
        for (size_t b = 0; b < cipher.len; b++) {
            plain.data[b] = cipher.data[b] ^ xor_key;
        }
        long double var = letter_freq_variance(plain);
        if (var <= best_msgs[0].variance) {
            for (size_t m = NUM_MSGS - 1; m > 0; m--) {
                memcpy(best_msgs[m].msg.data, best_msgs[m - 1].msg.data, cipher.len);
                best_msgs[m].variance = best_msgs[m - 1].variance;
                best_msgs[m].xor_key = best_msgs[m - 1].xor_key;
            }
            memcpy(best_msgs[0].msg.data, plain.data, cipher.len);
            best_msgs[0].variance = var;
            best_msgs[0].xor_key = xor_key;
        }
    }

    for (size_t m = 0; m < NUM_MSGS; m++) {
        printf("var: %Lf msg: '%s' xor_key: %d\n",
                best_msgs[m].variance, best_msgs[m].msg.data, best_msgs[m].xor_key);
    }

    if ((r = free_raw(&cipher))) {
        printf("Error: free_raw() returned %d\n", r);
        return 3;
    }
    return 0;
}

void usage(char * prgm)
{
    printf("Usage: %s HEX-ENCODED-STRING\n", prgm);
}
