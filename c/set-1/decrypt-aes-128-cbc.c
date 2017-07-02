#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/conf.h>

#include "raw.h"

void usage(char *);

int main(int argc, char **argv)
{
    if (argc != 3) {
        usage(argv[0]);
        return 1;
    }

    // Load base64 encoded cipher file
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

    // Convert base64 encoded cipher_b64 to raw bytes
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

    raw_t cipher = {NULL, 0}, plain = {NULL, 0};
    if ((r = init_raw(&cipher, data_len))) {
        fprintf(stderr, "Error: init_raw() returned %d\n", r);
        return EXIT_FAILURE;
    }
    if ((r = init_raw(&plain, data_len + 32))) {
        fprintf(stderr, "Error: init_raw() returned %d\n", r);
        return EXIT_FAILURE;
    }

    if ((r = b64_to_raw(cipher_b64, cipher))) {
        fprintf(stderr, "Error: b64_to_raw() returned %d\n", r);
        return EXIT_FAILURE;
    }

    free(cipher_b64);

    raw_t key = {(uint8_t *)argv[2], 0};
    key.len = strlen((char *)key.data);

    EVP_CIPHER_CTX *ctx;
    if (!(ctx = EVP_CIPHER_CTX_new())) {
        ERR_print_errors_fp(stderr);
        fprintf(stderr, "Error: EVP_CIPHER_CTX_new() returned %d\n", r);
        return EXIT_FAILURE;
    }

    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key.data, NULL)) {
        ERR_print_errors_fp(stderr);
        fprintf(stderr, "Error: EVP_EncryptInit_ex() returned %d\n", r);
        return EXIT_FAILURE;
    }


    int len = plain.len;
    if (1 != EVP_DecryptUpdate(ctx, plain.data, &len, cipher.data, cipher.len)) {
        ERR_print_errors_fp(stderr);
        fprintf(stderr, "Error: EVP_DecryptUpdate() returned %d\n", r);
        return EXIT_FAILURE;
    }
    plain.len = len;

    if (1 != EVP_DecryptFinal_ex(ctx, plain.data + len, &len)) {
        ERR_print_errors_fp(stderr);
        fprintf(stderr, "Error: EVP_DecryptFinal_ex() returned %d\n", r);
        return EXIT_FAILURE;
    }
    plain.len += len;

    printf("%s\n", (char *)plain.data);


    return 0;
}


void usage(char * prgm)
{
    printf("Usage: %s /path/to/cipher \"KEYPHRASE\"\n", prgm);
}
