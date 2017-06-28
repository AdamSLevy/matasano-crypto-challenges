#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "raw.h"

void usage(char *);

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
    raw_t raw;
    if ((r = init_raw(&raw, hex_len/2))) {
        printf("Error: init_raw() returned %d\n", r);
        return 3;
    }

    if ((r = hex_to_raw(hex, &raw))) {
        printf("Error: hex_to_raw() returned %d\n", r);
        return 2;
    }


    size_t b64_len = 4 * (raw.len / 3) + 4 * (bool)(raw.len % 3);
    char *b64 = (char *)malloc(b64_len + 1);
    if (NULL == b64) {
        printf("Error: malloc()\n");
    }

    raw_to_b64(&raw, b64);

    printf("%s\n", (char *)b64);

    free(b64);

    if ((r = free_raw(&raw))) {
        printf("Error: free_raw() returned %d\n", r);
        return 3;
    }
    return 0;
}

void usage(char * prgm)
{
    printf("Usage: %s HEX-ENCODED-STRING\n", prgm);
}
