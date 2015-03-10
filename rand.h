#pragma once

#include <stdint.h>

uint64_t xorshift(uint64_t *state);
void     xorshift_fill(uint64_t *state, void *, size_t);

static inline double
randf(uint64_t *state)
{
    return (xorshift(state) * 2.0) / UINT64_MAX - 1.0;
}
