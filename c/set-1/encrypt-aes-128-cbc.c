#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/conf.h>

#include "raw.h"

#define BLOCK_SIZE (16)

void usage(char *);

int main(int argc, char **argv)
{
    if (argc != 3) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    // Load base64 encoded cipher file
    FILE *plain_file = fopen(argv[1], "r");
    if (NULL == plain_file) {
        int err = errno;
        fprintf(stderr, "Error: fopen() %d, '%s'", err, strerror(err));
        return EXIT_FAILURE;
    }

    if (fseek(plain_file, 0L, SEEK_END)) {
        int err = errno;
        fprintf(stderr, "Error: fseek() %d, '%s'", err, strerror(err));
        return EXIT_FAILURE;
    }
    size_t file_size = ftell(plain_file);
    rewind(plain_file);

    raw_t plain = {NULL, 0};
    if (init_raw(&plain, file_size)) {
        fprintf(stderr, "Error: init_raw()\n");
        return EXIT_FAILURE;
    }

    size_t read = fread(plain.data, 1, plain.len, plain_file);
    if (read != plain.len || ferror(plain_file)) {
        fprintf(stderr, "Error: fread()\n");
        return EXIT_FAILURE;
    }

    fclose(plain_file);

    add_padding(&plain, 16);

    raw_t cipher = {NULL, 0};
    if (init_raw(&cipher, plain.len)) {
        fprintf(stderr, "Error: init_raw()\n");
        return EXIT_FAILURE;
    }

    raw_t key = {(uint8_t *)argv[2], 0};
    key.len = strlen((char *)key.data);

    EVP_CIPHER_CTX *ctx;
    if (!(ctx = EVP_CIPHER_CTX_new())) {
        ERR_print_errors_fp(stderr);
        fprintf(stderr, "Error: EVP_CIPHER_CTX_new()\n");
        return EXIT_FAILURE;
    }

    if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_128_ecb(), NULL, key.data, NULL)) {
        ERR_print_errors_fp(stderr);
        fprintf(stderr, "Error: EVP_EncryptInit_ex()\n");
        return EXIT_FAILURE;
    }


    raw_t xor_block     = {NULL, 0},
          plain_block   = {NULL, BLOCK_SIZE},
          cipher_block  = {NULL, BLOCK_SIZE},
          iv_block      = {NULL, 0};

    if (init_raw(&xor_block, 16)) {
        fprintf(stderr, "Error: init_raw()\n");
        return EXIT_FAILURE;
    }
    if (init_raw(&iv_block, 16)) {
        fprintf(stderr, "Error: init_raw()\n");
        return EXIT_FAILURE;
    }
    int len = BLOCK_SIZE;
    plain_block.data = &plain.data[0];
    fixed_xor(iv_block, plain_block, xor_block);
    if (1 != EVP_EncryptUpdate(ctx, &cipher.data[0], &len,
                xor_block.data, BLOCK_SIZE)) {
        ERR_print_errors_fp(stderr);
        fprintf(stderr, "Error: EVP_DecryptUpdate()\n");
        return EXIT_FAILURE;
    }
    size_t prev_id = 0;
    for (size_t i = 1; i < plain.len / BLOCK_SIZE; i++) {
        size_t id = i * BLOCK_SIZE;
        cipher_block.data = &cipher.data[prev_id];
        plain_block.data = &plain.data[id];
        fixed_xor(cipher_block, plain_block, xor_block);
        if (1 != EVP_EncryptUpdate(ctx, &cipher.data[id], &len,
                    &xor_block.data[0], BLOCK_SIZE)) {
            ERR_print_errors_fp(stderr);
            fprintf(stderr, "Error: EVP_DecryptUpdate()\n");
            return EXIT_FAILURE;
        }
        if ((int)cipher_block.len != len) {
            fprintf(stderr, "Error: EVP_EncryptUpdate() len != BLOCK_SIZE\n");
            return EXIT_FAILURE;
        }
        prev_id = id;
    }

    free_raw(&xor_block);

    size_t name_len = strlen(argv[1]);
    char *cipher_filename = (char *)malloc(name_len + 5);
    strncpy(cipher_filename, argv[1], name_len);
    strncpy(cipher_filename + name_len, ".enc", 5);
    FILE *cipher_file = fopen(cipher_filename, "w");
    if (NULL == cipher_file) {
        int err = errno;
        fprintf(stderr, "Error: fopen() %d, '%s'", err, strerror(err));
        return EXIT_FAILURE;
    }

    fwrite(cipher.data, 1, cipher.len, cipher_file);

    fclose(cipher_file);

    return 0;
}


void usage(char * prgm)
{
    printf("Usage: %s /path/to/cipher \"KEYPHRASE\"\n", prgm);
}
