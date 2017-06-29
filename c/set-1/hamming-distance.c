#include "hamming-distance.h"

int hamming_distance(raw_t a, raw_t b)
{
    raw_t *small, *big;
    if (a.len <= b.len) {
        small = &a;
        big = &b;
    } else {
        small = &b;
        big = &a;
    }

    size_t bitcount = 0;
    for (size_t i = 0; i < big->len; i++) {
        uint8_t byte;
        if (i < small->len) {
            byte = a.data[i] ^ b.data[i];
        } else {
            byte = big->data[i];
        }
        uint8_t count;
        for (count = 0; byte != 0; count++) {
            byte &= byte - 1;
        }
        bitcount += count;
    }

    return bitcount;
}
