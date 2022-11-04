#include "rand_utils.h"

#include <stdint.h>
#include <random>

static inline uint64_t rotl(const uint64_t x, int k) { return (x << k) | (x >> (64 - k)); }

double to_double(uint64_t x)
{
    const union {
        uint64_t i;
        double d;
    } u = { .i = UINT64_C(0x3FF) << 52 | x >> 12 };
    return u.d - 1.0;
}

// seed, randomly generated with a d10
//uint64_t shuffle_table[4] { 4550963226, 4884, 2, 724 };
uint64_t shuffle_table[4] { std::rand(), std::rand(), std::rand(), std::rand() };
//#pragma omp threadprivate(shufle_table)

uint64_t next_rand()
{
    const uint64_t result = rotl(shuffle_table[1] * 5, 7) * 9;

    const uint64_t t = shuffle_table[1] << 17;

    shuffle_table[2] ^= shuffle_table[0];
    shuffle_table[3] ^= shuffle_table[1];
    shuffle_table[1] ^= shuffle_table[2];
    shuffle_table[0] ^= shuffle_table[3];

    shuffle_table[2] ^= t;

    shuffle_table[3] = rotl(shuffle_table[3], 45);

    return result;
}

uint64_t next_rand(uint64_t (&state)[4])
{
    const uint64_t result = rotl(state[1] * 5, 7) * 9;

    const uint64_t t = state[1] << 17;

    state[2] ^= state[0];
    state[3] ^= state[1];
    state[1] ^= state[2];
    state[0] ^= state[3];

    state[2] ^= t;

    state[3] = rotl(state[3], 45);

    return result;
}