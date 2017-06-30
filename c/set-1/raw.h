#ifndef RAW_H
#define RAW_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint8_t *data;
    size_t len;
} raw_t;

int init_raw(raw_t *r, size_t len);
int free_raw(raw_t *r);

extern char hexset[17];
extern char b64set[257];

// Operations
int fixed_xor(raw_t , raw_t , raw_t );

// Conversion functions
// All of the following functions assume that any necessary memory has been
// appropriately allocated by the calling function
uint8_t b64_to_val(char b64);
int hex_to_raw(char *, raw_t );
int b64_to_raw(char *, raw_t );
void raw_to_hex(raw_t , char *);
void raw_to_b64(raw_t , char *);

#endif // RAW_H
