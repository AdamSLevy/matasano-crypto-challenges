#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "raw.h"

long double avg_letter_freq[] = {
    0.0651738,      // A
    0.0124248,      // B
    0.0217339,      // C
    0.0349835,      // D
    0.10414422,     // E
    0.0197881,      // F
    0.0158610,      // G
    0.0492888,      // H
    0.0558094,      // I
    0.0009033,      // J
    0.0050529,      // K
    0.0331490,      // L
    0.0202124,      // M
    0.0564513,      // N
    0.0596302,      // O
    0.0137645,      // P
    0.0008606,      // Q
    0.0497563,      // R
    0.0515760,      // S
    0.0729357,      // T
    0.0225134,      // U
    0.0082903,      // V
    0.0171272,      // W
    0.0013692,      // X
    0.0145984,      // Y
    0.0007836,      // Z
    0.1918182,      // SPACE
};

#define SPACE (26)
#define NUM_LETTERS (sizeof(avg_letter_freq)/sizeof(avg_letter_freq[0]))

long double letter_freq_variance(raw_t msg)
{
    size_t letter_count[NUM_LETTERS] = {0};
    char *text = (char *)msg.data;
    for (size_t l = 0; l < msg.len; l++) {
        //if (isspace(text[l])) {
        if (' ' == text[l]) {
            letter_count[SPACE]++;
        } else if (isalpha(text[l])) {
            letter_count[toupper(text[l]) - 'A']++;
        }
    }

    long double text_letter_freq[NUM_LETTERS] = {0};
    long double sum_squares = 0;
    for (size_t l = 0; l < NUM_LETTERS; l++) {
        //printf("'%c' %ld\n", SPACE == l ? ' ' : 'A' + (char)l, letter_count[l]);
        text_letter_freq[l] = (long double)letter_count[l] / msg.len;
        //printf("%Lf\n", text_letter_freq[l]);
        text_letter_freq[l] = text_letter_freq[l] - avg_letter_freq[l];
        text_letter_freq[l] *= text_letter_freq[l];
        sum_squares += text_letter_freq[l];
    }

    return sum_squares;
}
