#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "raw.h"

void usage(char *);

int main(int argc, char **argv)
{
    if (argc != 3) {
        usage(argv[0]);
        return 1;
    }

    size_t len[2];
    for (size_t t = 0; t < 2; t++) {
        len[t] = strlen(argv[t + 1]);
        if (0 == len[t]) {
            printf("Error: STRING-%lu is empty\n", t + 1);
            return 2;
        }
    }

    raw_t plain, key;
    if (len[0] >= len[1]) {
        plain.data = (uint8_t *)argv[1];
        plain.len = len[0];
        key.data = (uint8_t *)argv[2];
        key.len = len[1];
    } else {
        plain.data = (uint8_t *)argv[2];
        plain.len = len[1];
        key.data = (uint8_t *)argv[1];
        key.len = len[0];
    }

    int r;
    raw_t cipher;
    if ((r = init_raw(&cipher, plain.len))) {
        printf("Error: init_raw() returned %d\n", r);
        return 3;
    }
    for (size_t i = 0; i < plain.len; i++) {
        cipher.data[i] = plain.data[i] ^ key.data[i % key.len];
    }

    char *hex = (char *)malloc(cipher.len * 2 + 1);
    if (NULL == hex) {
        printf("Error: malloc()\n");
    }

    raw_to_hex(&cipher, hex);

    printf("%s\n", (char *)hex);

    free(hex);

    if ((r = free_raw(&cipher))) {
        printf("Error: free_raw() returned %d\n", r);
        return 3;
    }
    return 0;
}

void usage(char * prgm)
{
    printf("Usage: %s STRING-1 STRING-2\n", prgm);
}
