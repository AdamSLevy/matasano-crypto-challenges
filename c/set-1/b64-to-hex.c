#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "raw.h"

void usage(char *);

int main(int argc, char **argv)
{
    if (argc != 2) {
        usage(argv[0]);
        return 1;
    }

    char *b64 = argv[1];
    size_t b64_len = strlen(b64);
    if (0 == b64_len) {
        printf("Error: BASE64-ENCODED-STRING is empty\n");
        return 2;
    }
    size_t data_len = 3 * (b64_len / 4);
    switch (b64_len % 4) {
        case 0:
            for (int i = 0; i < 2; i++) {
                if ('=' == b64[b64_len - (i+1)]) {
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
    raw_t raw;
    if ((r = init_raw(&raw, data_len))) {
        printf("Error: init_raw() returned %d\n", r);
        return 3;
    }

    if ((r = b64_to_raw(b64, &raw))) {
        printf("Error: b64_to_raw() returned %d\n", r);
        return 2;
    }

    char *hex = (char *)malloc(raw.len * 2 + 1);
    if (NULL == hex) {
        printf("Error: malloc()\n");
    }

    raw_to_hex(&raw, hex);

    printf("%s\n", (char *)hex);

    free(hex);
    if ((r = free_raw(&raw))) {
        printf("Error: free_raw() returned %d\n", r);
        return 3;
    }

    return 0;
}

void usage(char * prgm)
{
    printf("Usage: %s BASE64-ENCODED-STRING\n", prgm);
}
