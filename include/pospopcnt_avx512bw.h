#ifndef POSPOPCNT_AVX512BW_H
#define POSPOPCNT_AVX512BW_H

#include <x86intrin.h>
#include <stdint.h>

#if defined(__AVX512BW__) && __AVX512BW__ == 1

// utility function for use in pospopcnt_u16_avx512bw_harvey_seal
static inline void pospopcnt_csa_avx512(__m512i *__restrict__ h,
                                        __m512i *__restrict__ l, __m512i b,
                                        __m512i c) {
  *h = _mm512_ternarylogic_epi32(c, b, *l, 0xE8); // 11101000
  *l = _mm512_ternarylogic_epi32(c, b, *l, 0x96); // 10010110
}

// given a stream of len 16-bit words (in array), generates
// an histogram of 16 counts stored in flags, corresponding
// to the number of bit sets at the corresponding indexes (0,1,...,15).
//
// Uses 1KB blocks
static void pospopcnt_u16_avx512bw_harvey_seal_1KB(const uint16_t *array,
                                               uint32_t len, uint64_t *flags) {
  for (uint32_t i = len - (len % (32 * 16)); i < len; ++i) {
    for (int j = 0; j < 16; ++j) {
      flags[j] += (((array[i]) >> j) & 1);
    }
  }

  const __m512i *data = (const __m512i *)array;
  __m512i v1 = _mm512_setzero_si512();
  __m512i v2 = _mm512_setzero_si512();
  __m512i v4 = _mm512_setzero_si512();
  __m512i v8 = _mm512_setzero_si512();
  __m512i v16 = _mm512_setzero_si512();
  __m512i twosA, twosB, foursA, foursB, eightsA, eightsB;
  __m512i one = _mm512_set1_epi16(1);
  __m512i counter[16];

  const size_t size = len / 32;
  const uint64_t limit = size - size % 16;

  uint16_t buffer[32];

  uint64_t i = 0;
  while (i < limit) {
    for (size_t i = 0; i < 16; ++i)
      counter[i] = _mm512_setzero_si512();

    size_t thislimit = limit;
    if (thislimit - i >= (1 << 16))
      thislimit = i + (1 << 16) - 1;

    for (/**/; i < thislimit; i += 16) {
#define U(pos)                                                                 \
  {                                                                            \
    counter[pos] = _mm512_add_epi16(                                           \
        counter[pos], _mm512_and_si512(v16, _mm512_set1_epi16(1)));            \
    v16 = _mm512_srli_epi16(v16, 1);                                           \
  }
      pospopcnt_csa_avx512(&twosA, &v1, _mm512_loadu_si512(data + i + 0),
                           _mm512_loadu_si512(data + i + 1));
      pospopcnt_csa_avx512(&twosB, &v1, _mm512_loadu_si512(data + i + 2),
                           _mm512_loadu_si512(data + i + 3));
      pospopcnt_csa_avx512(&foursA, &v2, twosA, twosB);
      pospopcnt_csa_avx512(&twosA, &v1, _mm512_loadu_si512(data + i + 4),
                           _mm512_loadu_si512(data + i + 5));
      pospopcnt_csa_avx512(&twosB, &v1, _mm512_loadu_si512(data + i + 6),
                           _mm512_loadu_si512(data + i + 7));
      pospopcnt_csa_avx512(&foursB, &v2, twosA, twosB);
      pospopcnt_csa_avx512(&eightsA, &v4, foursA, foursB);
      pospopcnt_csa_avx512(&twosA, &v1, _mm512_loadu_si512(data + i + 8),
                           _mm512_loadu_si512(data + i + 9));
      pospopcnt_csa_avx512(&twosB, &v1, _mm512_loadu_si512(data + i + 10),
                           _mm512_loadu_si512(data + i + 11));
      pospopcnt_csa_avx512(&foursA, &v2, twosA, twosB);
      pospopcnt_csa_avx512(&twosA, &v1, _mm512_loadu_si512(data + i + 12),
                           _mm512_loadu_si512(data + i + 13));
      pospopcnt_csa_avx512(&twosB, &v1, _mm512_loadu_si512(data + i + 14),
                           _mm512_loadu_si512(data + i + 15));
      pospopcnt_csa_avx512(&foursB, &v2, twosA, twosB);
      pospopcnt_csa_avx512(&eightsB, &v4, foursA, foursB);
      U(0) U(1) U(2) U(3) U(4) U(5) U(6) U(7) U(8) U(9) U(10) U(11) U(12) U(13)
          U(14) U(15) // Updates
          pospopcnt_csa_avx512(&v16, &v8, eightsA, eightsB);
    }
    // Update the counters after the last iteration.
    for (size_t i = 0; i < 16; ++i)
      U(i)
#undef U

    for (size_t i = 0; i < 16; ++i) {
      _mm512_storeu_si512((__m512i *)buffer, counter[i]);
      for (size_t z = 0; z < 32; z++) {
        flags[i] += 16 * (uint64_t)buffer[z];
      }
    }
  }

  _mm512_storeu_si512((__m512i *)buffer, v1);
  for (size_t i = 0; i < 32; i++) {
    for (int j = 0; j < 16; j++) {
      flags[j] += 1 * ((buffer[i] >> j) & 1);
    }
  }

  _mm512_storeu_si512((__m512i *)buffer, v2);
  for (size_t i = 0; i < 32; i++) {
    for (int j = 0; j < 16; j++) {
      flags[j] += 2 * ((buffer[i] >> j) & 1);
    }
  }

  _mm512_storeu_si512((__m512i *)buffer, v4);
  for (size_t i = 0; i < 32; i++) {
    for (int j = 0; j < 16; j++) {
      flags[j] += 4 * ((buffer[i] >> j) & 1);
    }
  }

  _mm512_storeu_si512((__m512i *)buffer, v8);
  for (size_t i = 0; i < 32; i++) {
    for (int j = 0; j < 16; j++) {
      flags[j] += 8 * ((buffer[i] >> j) & 1);
    }
  }
}

// given a stream of len 16-bit words (in array), generates
// an histogram of 16 counts stored in flags, corresponding
// to the number of bit sets at the corresponding indexes (0,1,...,15).
//
// Uses 512B blocks
static void pospopcnt_u16_avx512bw_harvey_seal_512B(const uint16_t *array,
                                                    uint32_t len,
                                                    uint64_t *flags) {
  for (uint32_t i = len - (len % (32 * 8)); i < len; ++i) {
    for (int j = 0; j < 16; ++j) {
      flags[j] += (((array[i]) >> j) & 1);
    }
  }

  const __m512i *data = (const __m512i *)array;
  __m512i v1 = _mm512_setzero_si512();
  __m512i v2 = _mm512_setzero_si512();
  __m512i v4 = _mm512_setzero_si512();
  __m512i v8 = _mm512_setzero_si512();
  __m512i twosA, twosB, foursA, foursB;
  __m512i one = _mm512_set1_epi16(1);
  __m512i counter[16];

  const size_t size = len / 32;
  const uint64_t limit = size - size % 8;

  uint16_t buffer[32];

  uint64_t i = 0;
  while (i < limit) {
    for (size_t i = 0; i < 16; ++i)
      counter[i] = _mm512_setzero_si512();

    size_t thislimit = limit;
    if (thislimit - i >= (1 << 16))
      thislimit = i + (1 << 16) - 1;

    for (/**/; i < thislimit; i += 8) {
#define U(pos)                                                                 \
  {                                                                            \
    counter[pos] = _mm512_add_epi16(                                           \
        counter[pos], _mm512_and_si512(v8, _mm512_set1_epi16(1)));             \
    v8 = _mm512_srli_epi16(v8, 1);                                             \
  }
      pospopcnt_csa_avx512(&twosA, &v1, _mm512_loadu_si512(data + i + 0),
                           _mm512_loadu_si512(data + i + 1));
      pospopcnt_csa_avx512(&twosB, &v1, _mm512_loadu_si512(data + i + 2),
                           _mm512_loadu_si512(data + i + 3));
      pospopcnt_csa_avx512(&foursA, &v2, twosA, twosB);
      pospopcnt_csa_avx512(&twosA, &v1, _mm512_loadu_si512(data + i + 4),
                           _mm512_loadu_si512(data + i + 5));
      pospopcnt_csa_avx512(&twosB, &v1, _mm512_loadu_si512(data + i + 6),
                           _mm512_loadu_si512(data + i + 7));
      pospopcnt_csa_avx512(&foursB, &v2, twosA, twosB);
      U(0) U(1) U(2) U(3) U(4) U(5) U(6) U(7) U(8) U(9) U(10) U(11) U(12) U(13)
          U(14) U(15) // Updates
          pospopcnt_csa_avx512(&v8, &v4, foursA, foursB);
    }
    // Update the counters after the last iteration.
    for (size_t i = 0; i < 16; ++i)
      U(i)
#undef U

    for (size_t i = 0; i < 16; ++i) {
      _mm512_storeu_si512((__m512i *)buffer, counter[i]);
      for (size_t z = 0; z < 32; z++) {
        flags[i] += 8 * (uint64_t)buffer[z];
      }
    }
  }

  _mm512_storeu_si512((__m512i *)buffer, v1);
  for (size_t i = 0; i < 32; i++) {
    for (int j = 0; j < 16; j++) {
      flags[j] += 1 * ((buffer[i] >> j) & 1);
    }
  }

  _mm512_storeu_si512((__m512i *)buffer, v2);
  for (size_t i = 0; i < 32; i++) {
    for (int j = 0; j < 16; j++) {
      flags[j] += 2 * ((buffer[i] >> j) & 1);
    }
  }

  _mm512_storeu_si512((__m512i *)buffer, v4);
  for (size_t i = 0; i < 32; i++) {
    for (int j = 0; j < 16; j++) {
      flags[j] += 4 * ((buffer[i] >> j) & 1);
    }
  }

  _mm512_storeu_si512((__m512i *)buffer, v8);
  for (size_t i = 0; i < 32; i++) {
    for (int j = 0; j < 16; j++) {
      flags[j] += 8 * ((buffer[i] >> j) & 1);
    }
  }
}



// given a stream of len 16-bit words (in array), generates
// an histogram of 16 counts stored in flags, corresponding
// to the number of bit sets at the corresponding indexes (0,1,...,15).
//
// Uses 256B blocks
static void pospopcnt_u16_avx512bw_harvey_seal_256B(const uint16_t *array,
                                                    uint32_t len,
                                                    uint64_t *flags) {
  for (uint32_t i = len - (len % (32 * 4)); i < len; ++i) {
    for (int j = 0; j < 16; ++j) {
      flags[j] += (((array[i]) >> j) & 1);
    }
  }

  const __m512i *data = (const __m512i *)array;
  __m512i v1 = _mm512_setzero_si512();
  __m512i v2 = _mm512_setzero_si512();
  __m512i v4 = _mm512_setzero_si512();
  __m512i twosA, twosB;
  __m512i one = _mm512_set1_epi16(1);
  __m512i counter[16];

  const size_t size = len / 32;
  const uint64_t limit = size - size % 4;

  uint16_t buffer[32];

  uint64_t i = 0;
  while (i < limit) {
    for (size_t i = 0; i < 16; ++i)
      counter[i] = _mm512_setzero_si512();

    size_t thislimit = limit;
    if (thislimit - i >= (1 << 16))
      thislimit = i + (1 << 16) - 1;

    for (/**/; i < thislimit; i += 4) {
#define U(pos)                                                                 \
  {                                                                            \
    counter[pos] = _mm512_add_epi16(                                           \
        counter[pos], _mm512_and_si512(v4, _mm512_set1_epi16(1)));             \
    v4 = _mm512_srli_epi16(v4, 1);                                             \
  }
      pospopcnt_csa_avx512(&twosA, &v1, _mm512_loadu_si512(data + i + 0),
                           _mm512_loadu_si512(data + i + 1));
      pospopcnt_csa_avx512(&twosB, &v1, _mm512_loadu_si512(data + i + 2),
                           _mm512_loadu_si512(data + i + 3));
      U(0) U(1) U(2) U(3) U(4) U(5) U(6) U(7) U(8) U(9) U(10) U(11) U(12) U(13)
          U(14) U(15) // Updates
      pospopcnt_csa_avx512(&v4, &v2, twosA, twosB);
    }
    // Update the counters after the last iteration.
    for (size_t i = 0; i < 16; ++i)
      U(i)
#undef U

    for (size_t i = 0; i < 16; ++i) {
      _mm512_storeu_si512((__m512i *)buffer, counter[i]);
      for (size_t z = 0; z < 32; z++) {
        flags[i] += 4 * (uint64_t)buffer[z];
      }
    }
  }

  _mm512_storeu_si512((__m512i *)buffer, v1);
  for (size_t i = 0; i < 32; i++) {
    for (int j = 0; j < 16; j++) {
      flags[j] += 1 * ((buffer[i] >> j) & 1);
    }
  }

  _mm512_storeu_si512((__m512i *)buffer, v2);
  for (size_t i = 0; i < 32; i++) {
    for (int j = 0; j < 16; j++) {
      flags[j] += 2 * ((buffer[i] >> j) & 1);
    }
  }

  _mm512_storeu_si512((__m512i *)buffer, v4);
  for (size_t i = 0; i < 32; i++) {
    for (int j = 0; j < 16; j++) {
      flags[j] += 4 * ((buffer[i] >> j) & 1);
    }
  }
}



#endif // __AVX512BW__

#endif // POSPOPCNT_AVX512BW_H
