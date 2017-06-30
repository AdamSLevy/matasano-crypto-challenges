#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "raw.h"

void usage(char *);

int main(int argc, char **argv)
{
    if (argc != 3) {
        usage(argv[0]);
        return 1;
    }

    char *hex1 = argv[1];
    char *hex2 = argv[2];
    size_t hex1_len = strlen(hex1);
    size_t hex2_len = strlen(hex2);

    if (0 == hex1_len) {
        printf("Error: HEX-ENCODED-STRING-1 is empty\n");
        return 2;
    }

    if (0 == hex2_len) {
        printf("Error: HEX-ENCODED-STRING-2 is empty\n");
        return 2;
    }

    if (hex1_len % 2) {
        printf("Error: HEX-ENCODED-STRING-1 is not of even length\n");
        return 2;
    }

    if (hex2_len % 2) {
        printf("Error: HEX-ENCODED-STRING-2 is not of even length\n");
        return 2;
    }

    if (hex1_len != hex2_len) {
        printf("Error: HEX-ENCODED-STRING-1 and HEX-ENCODED-STRING-2 "
               "are not of equal length\n");
        return 2;
    }

    int r;
    raw_t raw1, raw2, result;
    if ((r = init_raw(&raw1, hex1_len/2))) {
        printf("Error: init_raw() returned %d\n", r);
        return 3;
    }

    if ((r = init_raw(&raw2, hex2_len/2))) {
        printf("Error: init_raw() returned %d\n", r);
        return 3;
    }

    if ((r = init_raw(&result, hex2_len/2))) {
        printf("Error: init_raw() returned %d\n", r);
        return 3;
    }

    if ((r = hex_to_raw(hex1, raw1))) {
        printf("Error: hex_to_raw() returned %d\n", r);
        return 2;
    }

    if ((r = hex_to_raw(hex2, raw2))) {
        printf("Error: hex_to_raw() returned %d\n", r);
        return 2;
    }

    if (fixed_xor(raw1, raw2, result)) {
        printf("Error: fixed_xor() returned %d\n", r);
        return 2;
    }

    char *result_hex = (char *)malloc(result.len * 2 + 1);
    if (NULL == result_hex) {
        printf("Error: malloc()\n");
    }

    raw_to_hex(result, result_hex);

    printf("%s\n", (char *)result_hex);

    if ((r = free_raw(&raw1))) {
        printf("Error: free_raw() returned %d\n", r);
        return 3;
    }

    if ((r = free_raw(&raw2))) {
        printf("Error: free_raw() returned %d\n", r);
        return 3;
    }

    if ((r = free_raw(&result))) {
        printf("Error: free_raw() returned %d\n", r);
        return 3;
    }
    return 0;
}

void usage(char * prgm)
{
    printf("Usage: %s HEX-ENCODED-STRING-1 HEX-ENCODED-STRING-2\n", prgm);
}
