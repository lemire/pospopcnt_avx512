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
                                 uint64_t *flags) {
  for (int i = 0; i < len; ++i) {
      uint64_t w = data[i];
      flags[0] += ((w >> 0) & 1);
      flags[1] += ((w >> 1) & 1);
      flags[2] += ((w >> 2) & 1);
      flags[3] += ((w >> 3) & 1);
      flags[4] += ((w >> 4) & 1);
      flags[5] += ((w >> 5) & 1);
      flags[6] += ((w >> 6) & 1);
      flags[7] += ((w >> 7) & 1);
      flags[8] += ((w >> 8) & 1);
      flags[9] += ((w >> 9) & 1);
      flags[10] += ((w >> 10) & 1);
      flags[11] += ((w >> 11) & 1);
      flags[12] += ((w >> 12) & 1);
      flags[13] += ((w >> 13) & 1);
      flags[14] += ((w >> 14) & 1);
      flags[15] += ((w >> 15) & 1);
  }
}

#endif // POSPOPCNT_H
