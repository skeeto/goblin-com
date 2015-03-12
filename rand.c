#include <string.h>
#include <ctype.h>
#include <math.h>
#include "rand.h"

#define MIN(x, y) ((y) < (x) ? (y) : (x))

uint64_t rand_state = 0;

uint64_t
xorshift(uint64_t *state) {
    uint64_t x = *state;
    x ^= x >> 12; // a
    x ^= x << 25; // b
    x ^= x >> 27; // c
    *state = x;
    return x * UINT64_C(2685821657736338717);
}

void
xorshift_fill(uint64_t *state, void *buffer, size_t size)
{
    char *p = buffer;
    while (size) {
        uint64_t entropy = xorshift(state);
        size_t amount = MIN(size, sizeof(entropy));
        memcpy(p, &entropy, amount);
        size -= amount;
        p += amount;
    }
}

float
rand_uniform_s(uint64_t *state, float min, float max)
{
    float u = xorshift(state) / (double)UINT64_MAX;
    return u * (max - min) + min;
}

float
rand_uniform(float min, float max)
{
    return rand_uniform_s(&rand_state, min, max);

}

int
rand_range_s(uint64_t *state, int min, int max)
{
    uint64_t x = xorshift(state);
    return (x % (max - min)) + min;
}

int
rand_range(int min, int max)
{
    return rand_range_s(&rand_state, min, max);
}

void
rand_name(char *name, size_t max)
{
    /* This thing sucks ... */
    const char *vowels = "aeiou";
    const char *consonants = "bcdfghjklmnpqrstvwxyz";
    int length = rand_range(6, max);
    int spaces = 0;
    int letters = 0;
    for (int i = 0; i < length; i++) {
        float spacep = letters / 3.0f * powf(0.1f, 1.0f / (10 * spaces + 1));
        if (i > 0 && rand_uniform(0, 1.0) < spacep) {
            name[i] = ' ';
            letters = 0;
        } else {
            if (i % 2)
                name[i] = consonants[rand_range(0, strlen(consonants))];
            else
                name[i] = vowels[rand_range(0, strlen(vowels))];
            if (letters == 0)
                name[i] = toupper((int)name[i]);
            letters++;
        }
    }
    name[length] = '\0';
}
