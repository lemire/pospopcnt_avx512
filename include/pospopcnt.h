#ifndef POSPOPCNT_H
#define POSPOPCNT_H

#include <stdint.h>

#if defined(__clang__)
#pragma clang loop vectorize(disable)
#elif defined(__GNUC__)
__attribute__((optimize("no-tree-vectorize")))
#endif
// given a stream of len 16-bit words (in data), generates
// an histogram of 16 counts stored in flags, corresponding
// to the number of bit sets at the corresponding indexes (0,1,...,15).
static void pospopcnt_u16_scalar(const uint16_t *data, uint32_t len,
                                 uint32_t *flags) {
  for (int i = 0; i < len; ++i) {
    for (int j = 0; j < 16; ++j) {
      flags[j] += (((data[i]) >> j) & 1);
    }
  }
}

#endif // POSPOPCNT_H
