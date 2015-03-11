#include <string.h>
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
