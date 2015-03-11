#pragma once

#include <stdint.h>

extern uint64_t rand_state;

uint64_t xorshift(uint64_t *state);
void     xorshift_fill(uint64_t *state, void *, size_t);

float rand_uniform_s(uint64_t *state, float min, float max);
float rand_uniform(float min, float max);
