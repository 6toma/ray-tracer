#include <stdint.h>

double to_double(uint64_t x);

uint64_t next_rand();

uint64_t next_rand(uint64_t (&state)[4]);