#include <stddef.h>
#include <stdint.h>

typedef struct {
    void *data;
    size_t len;
} raw_t;

int init_raw(raw_t *r, size_t len);
int free_raw(raw_t *r);

extern char hexset[17];
extern char b64set[257];

// Conversions
uint8_t b64_to_val(char b64);
int hex_to_raw(char *, raw_t *);
int b64_to_raw(char *, raw_t *);
void raw_to_hex(raw_t *, char *);
void raw_to_b64(raw_t *, char *);
