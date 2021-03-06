#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "raw.h"
#include <stdio.h>

// Allocate len + 1 bytes at r->data
int init_raw(raw_t *r, size_t len)
{
    if (NULL == r) {
        return 1;
    }

    if (NULL == r->data) {
        if (NULL == (r->data = (uint8_t*)calloc(len + 1, sizeof(char)))) {
            r->data = NULL;
            return 2;
        }
    } else {
        if (NULL == (r->data = (uint8_t*)realloc((void *)r->data, len + 1))) {
            r->data = NULL;
            return 2;
        }
    }

    char *end = (char *)r->data + len;
    *end = '\0';
    r->len = len;
    return 0;
}

// Free r->data
int free_raw(raw_t *r)
{
    if (NULL == r || NULL == r->data || 0 == r->len) {
        return 1;
    }

    free(r->data);
    r->data = NULL;
    r->len = 0;
    return 0;
}

// All of the following functions assume that any necessary memory has been
// appropriately allocated by the calling function

// Operations
int fixed_xor(raw_t r1, raw_t r2, raw_t res)
{
    size_t min_len = r1.len < r2.len ?
                        (r1.len < res.len ? r1.len : res.len)
                            : r2.len;

    for (size_t i = 0; i < min_len; i++) {
        res.data[i] = r1.data[i] ^ r2.data[i];
    }
    res.data[min_len] = '\0';
    return 0;
}

// Conversion functions
int hex_to_raw(char *h, raw_t r)
{
    size_t h_len = strlen(h);
    if (NULL == r.data || 0 == r.len || 0 == h_len || h_len % 2) {
        return -1;
    }
    size_t min_len = h_len < r.len/2 ? h_len/2 : r.len;
    char hex_byte[] = "00";
    for (size_t b = 0; b < min_len; b++) {
        hex_byte[0] = h[b*2];
        hex_byte[1] = h[b*2 + 1];
        char *endptr = NULL;
        r.data[b] = (uint8_t)strtol(hex_byte, &endptr, 16);
        if (*endptr != '\0') {
            return -(b + 1);
        }
    }
    return 0;
}

uint8_t b64_to_val(char b64)
{
    if ('A' <= b64 && b64 <= 'Z') {
        return b64 - 'A';
    } else if ('a' <= b64 && b64 <= 'z') {
        return b64 - 'a' + 26;
    } else if ('0' <= b64 && b64 <= '9') {
        return b64 - '0' + 52;
    } else {
        switch (b64) {
            case '+':
                return 62;
            case '/':
                return 63;
            default:
                return 255;
        }
    }
}

int b64_to_raw(char *b64, raw_t r)
{
    size_t b64_len = strlen(b64);
    if (NULL == r.data || 0 == r.len || 0 == b64_len || 1 == b64_len % 4) {
        return -1;
    }
    size_t min_len = b64_len/4 < r.len/3 ? b64_len/4 : r.len/3;
    for (size_t b = 0; b < min_len; b++) {
        uint8_t b64_val[] = {
            b64_to_val(b64[b*4    ]),
            b64_to_val(b64[b*4 + 1]),
            b64_to_val(b64[b*4 + 2]),
            b64_to_val(b64[b*4 + 3]),
        };
        for (size_t i = 0; i < sizeof(b64_val); i++) {
            if (63 < b64_val[i]) {
                return b*4 + i;
            }
        }
        r.data[b*3    ] = b64_val[0] << 2 | b64_val[1] >> 4;
        r.data[b*3 + 1] = b64_val[1] << 4 | b64_val[2] >> 2;
        r.data[b*3 + 2] = b64_val[2] << 6 | b64_val[3];
    }

    switch (r.len % 3) {
        case 1:
            {
                uint8_t b64_val[] = {
                    b64_to_val(b64[4 * (r.len / 3)]),
                    b64_to_val(b64[4 * (r.len / 3) + 1]),
                };
                for (size_t i = 0; i < sizeof(b64_val); i++) {
                    if (63 < b64_val[i]) {
                        return 4 * (r.len / 3) + i;
                    }
                }
                r.data[r.len - 1] = b64_val[0] << 2 | b64_val[1] >> 4;
            }
            break;
        case 2:
            {
                uint8_t b64_val[] = {
                    b64_to_val(b64[4 * (r.len / 3)]),
                    b64_to_val(b64[4 * (r.len / 3) + 1]),
                    b64_to_val(b64[4 * (r.len / 3) + 2]),
                };
                for (size_t i = 0; i < sizeof(b64_val); i++) {
                    if (63 < b64_val[i]) {
                        return 4 * (r.len / 3) + i;
                    }
                }
                r.data[r.len - 2] = b64_val[0] << 2 | b64_val[1] >> 4;
                r.data[r.len - 1] = b64_val[1] << 4 | b64_val[2] >> 2;
            }
            break;
    }

    return 0;
}

char hexset[17] = "0123456789abcdef";
void raw_to_hex(raw_t r, char *h)
{
    for (size_t b = 0; b < r.len; b++) {
        uint8_t byte = r.data[b];
        h[2 * b    ] = hexset[byte >> 4];
        h[2 * b + 1] = hexset[byte & 0x0f];
    }
    h[2 * r.len] = '\0';
}

// Repeating the symbol set allows us to avoid masking off the high bits
char b64set[257] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"
                   "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"
                   "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"
                   "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void raw_to_b64(raw_t r, char *b64)
{
    uint8_t *byte;
    for (size_t b = 0; b < r.len / 3; b++) {
        byte = r.data + 3 * b;
        b64[4 * b    ] = b64set[(uint8_t)(byte[0] >> 2                              )];
        b64[4 * b + 1] = b64set[(uint8_t)(byte[0] << 4 | byte[1] >> 4               )];
        b64[4 * b + 2] = b64set[(uint8_t)(               byte[1] << 2 | byte[2] >> 6)];
        b64[4 * b + 3] = b64set[(uint8_t)(                              byte[2]     )];
    }

    size_t b64_len = 4 * (r.len / 3) + 4 * (bool)(r.len % 3);
    byte = r.data + r.len - (r.len % 3);
    switch (r.len % 3) {
        case 1:
            b64[b64_len - 4] = b64set[(uint8_t)(byte[0] >> 2)];
            b64[b64_len - 3] = b64set[(uint8_t)(byte[0] << 4)];
            b64[b64_len - 2] = '=';
            b64[b64_len - 1] = '=';
            break;
        case 2:
            b64[b64_len - 4] = b64set[(uint8_t)(byte[0] >> 2               )];
            b64[b64_len - 3] = b64set[(uint8_t)(byte[0] << 4 | byte[1] >> 4)];
            b64[b64_len - 2] = b64set[(uint8_t)(               byte[1] << 2)];
            b64[b64_len - 1] = '=';
            break;
    }

    b64[b64_len] = '\0';
}
