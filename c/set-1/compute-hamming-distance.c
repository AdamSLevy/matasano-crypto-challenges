#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "raw.h"
#include "hamming-distance.h"

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
    }

    raw_t left, right;
    left.data = (uint8_t *)argv[1];
    left.len = len[0];
    right.data = (uint8_t *)argv[2];
    right.len = len[1];

    size_t ham_score = hamming_distance(left, right);

    printf("Hamming distance: %lu\n", ham_score);

    return 0;
}

void usage(char * prgm)
{
    printf("Usage: %s STRING-1 STRING-2\n", prgm);
}
