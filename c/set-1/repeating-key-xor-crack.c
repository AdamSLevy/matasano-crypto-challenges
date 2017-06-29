#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <float.h>

#include "raw.h"
#include "hamming-distance.h"
#include "frequency-analysis.h"

#define NUM_TOP_KEYSIZES (1)
#define MIN_KEYSIZE (2)
#define MAX_KEYSIZE (48)
typedef struct {
    size_t keysize;
    double ham_score;
} keysize_t;

typedef struct {
    raw_t block;
    uint8_t xor_key;
    long double variance;
} best_blocks_t;
#define NUM_BEST_BLOCKS (2)
int crack_xor(raw_t cipher, best_blocks_t *best, size_t num_best_blocks);

void usage(char *);

int main(int argc, char **argv)
{
    if (argc != 2) {
        usage(argv[0]);
        return 1;
    }

    FILE *cipher_file = fopen(argv[1], "r");
    if (NULL == cipher_file) {
        int err = errno;
        printf("Error: fopen() %d, '%s'", err, strerror(err));
        return -1;
    }

    if (fseek(cipher_file, 0L, SEEK_END)) {
        int err = errno;
        printf("Error: fseek() %d, '%s'", err, strerror(err));
        return -1;
    }
    size_t file_size = ftell(cipher_file);
    rewind(cipher_file);

    // Parse file line by line and remove the '\n' chars
    char *cipher_b64 = (char *)malloc(file_size + 1);
    char *cipher_line = NULL;
    size_t cipher_b64_len = 0;
    size_t n = 0;
    int r = 0;
    errno = 0;
    while (-1 != (r = getline(&cipher_line, &n, cipher_file))) {
        memcpy(cipher_b64 + cipher_b64_len, cipher_line, r - 1);
        cipher_b64_len += (r - 1);
    }
    if (errno) {
        int err = errno;
        printf("Error: getline() %d, '%s'", err, strerror(err));
        return -1;
    }

    free(cipher_line);

    if (NULL == (cipher_b64 = (char *)realloc(cipher_b64, cipher_b64_len + 1))) {
        int err = errno;
        printf("Error: realloc() %d, '%s'", err, strerror(err));
        return -1;
    }

    // Convert cipher_b64 to raw_t cipher
    size_t data_len = 3 * (cipher_b64_len / 4);
    switch (cipher_b64_len % 4) {
        case 0:
            for (int i = 0; i < 2; i++) {
                if ('=' == cipher_b64[cipher_b64_len - i - 1]) {
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

    raw_t cipher = {NULL, 0};
    if ((r = init_raw(&cipher, data_len))) {
        printf("Error: init_raw() returned %d\n", r);
        return 3;
    }

    if ((r = b64_to_raw(cipher_b64, &cipher))) {
        printf("Error: b64_to_raw() returned %d\n", r);
        return 2;
    }

    free(cipher_b64);

    // Determine top likely keysizes by minimum average hamming distance of keysize blocks
    keysize_t top_keysizes[NUM_TOP_KEYSIZES] = {{0, DBL_MAX}};
    for (size_t keysize = MIN_KEYSIZE; keysize <= MAX_KEYSIZE; keysize++) {
        size_t ham_sum = 0;
        for (size_t i = 0; i < (cipher.len / keysize) - 1; i += 2) {
            raw_t left =  {cipher.data +       i * keysize, keysize};
            raw_t right = {cipher.data + (i + 1) * keysize, keysize};
            ham_sum += hamming_distance(left, right);
        }
        double ham_score = ((double)ham_sum / keysize) / ((cipher.len / keysize) / 2);
        if (ham_score <= top_keysizes[0].ham_score) {
            for (size_t i = NUM_TOP_KEYSIZES - 1; i > 0; i--) {
                memcpy(&top_keysizes[i], &top_keysizes[i - 1], sizeof(keysize_t));
            }
            top_keysizes[0].keysize = keysize;
            top_keysizes[0].ham_score = ham_score;
        }
    }

    for (size_t i = 0; i < NUM_TOP_KEYSIZES; i++) {
        printf("average ham_score: %f, keysize: %lu\n",
                top_keysizes[i].ham_score, top_keysizes[i].keysize);
    }

    // Transpose cipher into num keysize blocks of size cipher.len / keysize + 1
    // Crack each block as single key xor
    for (size_t k = 0; k < NUM_TOP_KEYSIZES; k++) {
        // Allocate blocks
        size_t keysize = top_keysizes[k].keysize;
        raw_t *blocks = (raw_t *)calloc(keysize, sizeof(raw_t));
        for (size_t i = 0; i < keysize; i++) {
            if ((r = init_raw(&blocks[i], cipher.len / keysize +
                            (i < cipher.len % keysize ? 1 : 0)))) {
                printf("Error: init_raw() returned %d\n", r);
                return 3;
            }
        }
        printf("keysize: %lu\n", keysize);

        // Transpose cipher to blocks
        for (size_t b = 0; b < cipher.len; b++) {
            blocks[b % keysize].data[b / keysize] = cipher.data[b];
        }

        // Crack each block
        best_blocks_t *best = (best_blocks_t *)calloc(NUM_BEST_BLOCKS * keysize,
                sizeof(best_blocks_t));
        if (NULL == best) {
            printf("Error: calloc()\n");
            return -1;
        }

        for (size_t k = 0; k < keysize; k++) {
            crack_xor(blocks[k], &best[k * NUM_BEST_BLOCKS], NUM_BEST_BLOCKS);
        }

        raw_t plain[NUM_BEST_BLOCKS] = {{NULL, 0}};
        raw_t key[NUM_BEST_BLOCKS] = {{NULL, 0}};
        // Transpose best plaintext blocks back to correct order
        for (size_t d = 0; d < NUM_BEST_BLOCKS; d++) {
            printf("line %d d %lu\n", __LINE__, d);
            if (init_raw(&key[d], keysize)) {
                printf("Error: init_raw() returned %d\n", r);
                return -1;
            }
            printf("line %d d %lu\n", __LINE__, d);
            if (init_raw(&plain[d], cipher.len)) {
                printf("Error: init_raw() returned %d\n", r);
                return -1;
            }
            printf("line %d d %lu\n", __LINE__, d);
            for (size_t b = 0; b < cipher.len; b++) {
                plain[d].data[b] = best[NUM_BEST_BLOCKS * (b % keysize) + d]
                    .block.data[b / keysize];
                if (b < keysize) {
                    key[d].data[b] =
                        (&best[NUM_BEST_BLOCKS * (b % keysize)])[d].xor_key;
                }
            }
            printf("%lu\n\nkey:\n%s\n\nplaintext:\n%s\n\n",
                    d, key[d].data, plain[d].data);
        }
        free(best);
        free_raw(blocks);
    }


    if ((r = free_raw(&cipher))) {
        printf("Error: free_raw() returned %d\n", r);
        return 3;
    }
    return 0;
}

int crack_xor(raw_t cipher, best_blocks_t *best, size_t num_best_blocks)
{
    raw_t plain = {NULL, 0};
    if (init_raw(&plain, cipher.len)) {
        return -1;
    }
    best[0].variance = LDBL_MAX;

    for (size_t x = 0; x < 256; x++) {
        uint8_t xor_key = x;
        for (size_t b = 0; b < cipher.len; b++) {
            plain.data[b] = cipher.data[b] ^ xor_key;
        }
        long double var = letter_freq_variance(plain);
        if (var <= best[0].variance) {
            raw_t tmp;
            tmp.data = best[num_best_blocks - 1].block.data;
            tmp.len  = best[num_best_blocks - 1].block.len;
            for (size_t m = num_best_blocks - 1; m > 0; m--) {
                if (NULL != best[m - 1].block.data) {
                    memcpy(&best[m], &best[m - 1], sizeof(best_blocks_t));
                }
            }
            best[0].block.data = tmp.data;
            best[0].block.len  = tmp.len;
            init_raw(&best[0].block, plain.len);
            memcpy(best[0].block.data, plain.data, cipher.len);
            best[0].variance = var;
            best[0].xor_key = xor_key;
        }
    }

    free_raw(&plain);
    return 0;
}

void usage(char * prgm)
{
    printf("Usage: %s /path/to/cipher\n", prgm);
}
