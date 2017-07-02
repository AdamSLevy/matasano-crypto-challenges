#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/conf.h>

#include "raw.h"
#include "hamming-distance.h"
#include "sorted-list.h"

void usage(char *);

typedef struct {
    raw_t *raw;
    int id;
} data_t;

void free_data(void *data) {
    data_t *d = (data_t *)data;
    free_raw(d->raw);
}

double compute_score(raw_t);

int main(int argc, char **argv)
{
    if (argc != 2) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    // Load base64 encoded cipher file
    FILE *cipher_file = fopen(argv[1], "r");
    if (NULL == cipher_file) {
        int err = errno;
        fprintf(stderr, "Error: fopen() %d, '%s'", err, strerror(err));
        return EXIT_FAILURE;
    }

    raw_t *input = NULL;

    // Each line is an input
    char *cipher_line = NULL;
    size_t n = 0;
    int r = 0;
    errno = 0;
    int num_inputs = 0;
    while (-1 != (r = getline(&cipher_line, &n, cipher_file))) {
        if (NULL == (input = (raw_t *)realloc(input,
                        (num_inputs + 1) * sizeof(raw_t)))) {
            int err = errno;
            fprintf(stderr, "Error: realloc() %d, '%s'", err, strerror(err));
            return EXIT_FAILURE;
        }
        input[num_inputs].data = (uint8_t *)cipher_line;
        input[num_inputs].data[r - 1] = '\0';
        input[num_inputs].len = r - 1;
        cipher_line = NULL;
        n = 0;
        num_inputs++;
    }
    if (errno) {
        int err = errno;
        fprintf(stderr, "Error: getline() %d, '%s'", err, strerror(err));
        return EXIT_FAILURE;
    }

    raw_t *raw = (raw_t *)calloc(num_inputs, sizeof(raw_t));
    for (int i = 0; i < num_inputs; i++) {
        // Convert base64 encoded cipher_b64 to raw bytes
        size_t data_len = 3 * (input[i].len / 4);
        switch (input[i].len % 4) {
            case 0:
                for (int j = 0; j < 2; j++) {
                    if ('=' == input[i].data[input[i].len - j - 1]) {
                        data_len--;
                    }
                }
                break;
            case 1:
                fprintf(stderr, "Error: BASE64-ENCODED-STRING improper length\n");
                return EXIT_FAILURE;
            case 2:
                data_len++;
                break;
            case 3:
                data_len += 2;
                break;
        }

        if ((r = init_raw(&raw[i], data_len))) {
            fprintf(stderr, "Error: init_raw() returned %d\n", r);
            return EXIT_FAILURE;
        }
        if ((r = b64_to_raw((char *)input[i].data, raw[i]))) {
            fprintf(stderr, "Error: b64_to_raw() returned %d\n", r);
            return EXIT_FAILURE;
        }
    }

    list_t list = LIST_INIT;
    list.max_num_items = 6;
    list.max = true;
    for (int i = 0; i < num_inputs; i++) {
        data_t data;
        data.raw = &input[i];
        data.id = i;
        if (add_item(&list, compute_score(raw[i]),
                    (void *)&data, sizeof(raw_t))) {
            fprintf(stderr, "Error: add_item()\n");
            return EXIT_FAILURE;
        }
    }

    for (list_item_t *node = list.first; NULL != node;
            node = (list_item_t *)node->next) {
        data_t *data = (data_t *)node->data;
        raw_t *output = data->raw;
        printf("score: %f line: %d data: %s\n", node->score, data->id,
                (char *)output->data);
    }


    list.free_data = &free_data;
    free_list(&list);

    return EXIT_SUCCESS;
}

#define ECB_BLOCK_SIZE (16)
double compute_score(raw_t cipher) {
    double ham_score = 0;
    size_t num_samples = 0;
    for (size_t i = 0; i < (cipher.len / ECB_BLOCK_SIZE) - 1; i++) {
        raw_t left;
        left.data = cipher.data + i * ECB_BLOCK_SIZE;
        left.len = ECB_BLOCK_SIZE;
        for (size_t j = i + 1; j < cipher.len / ECB_BLOCK_SIZE; j++) {
            raw_t right;
            right.data = cipher.data + j * ECB_BLOCK_SIZE;
            right.len = ECB_BLOCK_SIZE;
            double ham = (double)hamming_distance(left, right) / ECB_BLOCK_SIZE;
            ham_score += 1 / (ham + 1);
            num_samples++;
        }
    }

    ham_score /= (double) num_samples;

    return ham_score;
}

void usage(char * prgm)
{
    printf("Usage: %s /path/to/cipherlist\n", prgm);
}
