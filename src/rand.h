#pragma once

#include <stdint.h>

#define countof(a) (sizeof(a) / sizeof(0[a]))

#define PI 3.141592653589793

extern uint64_t rand_state;

uint64_t xorshift(uint64_t *state);
void     xorshift_fill(uint64_t *state, void *, size_t);

float rand_uniform_s(uint64_t *state, float min, float max);
float rand_uniform(float min, float max);

int rand_range_s(uint64_t *state, int min, int max);
int rand_range(int min, int max);

void rand_name(char *name, size_t max);
