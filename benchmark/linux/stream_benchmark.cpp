#ifdef __linux__
#include <cassert>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <libgen.h>
#include <random>
#include <string>
#include <vector>

#include "linux-perf-events.h"
#include "aligned_alloc.h"
#include "pospopcnt_avx512bw.h"
#include "pospopcnt.h"
#ifdef ALIGN
#include "memalloc.h"
#define memory_allocate(size) aligned_alloc(64, (size))
#else
#define memory_allocate(size) malloc(size)
#endif

// Function pointer definition.
typedef void (*pospopcnt_u16_method_type)(const uint16_t *data, uint32_t len,
                                          uint32_t *flags);
#define PPOPCNT_NUMBER_METHODS 4
pospopcnt_u16_method_type pospopcnt_u16_methods[] = {
  pospopcnt_u16_scalar, pospopcnt_u16_avx512bw_harvey_seal_1KB, pospopcnt_u16_avx512bw_harvey_seal_512B, pospopcnt_u16_avx512bw_harvey_seal_256B
};

static const char *const pospopcnt_u16_method_names[] = {
  "pospopcnt_u16_scalar", "pospopcnt_u16_avx512bw_harvey_seal_1KB", "pospopcnt_u16_avx512bw_harvey_seal_512B", "pospopcnt_u16_avx512bw_harvey_seal_256B"
};


template <class C>
double benchmark(C & vdata, uint32_t n, pospopcnt_u16_method_type fn) {
  std::vector<double> timings;
  std::vector<uint32_t> correctflags(16);
  pospopcnt_u16_scalar(vdata.data(), n, correctflags.data()); // this is our gold standard
  for(size_t i = 0; i < 100; i++) {
    std::vector<uint32_t> flags(16);
    auto start = std::chrono::steady_clock::now();
    fn(vdata.data(), n, flags.data());
    auto end = std::chrono::steady_clock::now();
    if(correctflags != flags) { throw std::runtime_error("bug\n"); }
    std::chrono::duration<double> secs = end - start;
    double time_in_s = secs.count();
    timings.push_back(time_in_s);
  }
  double min_timing = *min_element(timings.begin(), timings.end());
  double speedinGBs = (n * sizeof(uint16_t)) / (min_timing * 1000000000.0);
  return speedinGBs;
}


int main(int argc, char **argv) {
  size_t max_val = 536870912;
  std::vector<uint16_t> vdata(max_val);
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, 0xFFFF);
  for (size_t k2 = 0; k2 < vdata.size(); k2++) {
    vdata[k2] = dis(gen); // random init.
  }
  printf("#");  
  for (size_t k = 0; k < PPOPCNT_NUMBER_METHODS; k++) {
      printf("\t");
      printf("%-40s\t", pospopcnt_u16_method_names[k]);
      fflush(NULL);
  }
  printf("\n");
   
  for (int n = 1024; n <= max_val; n*=2) {
    printf("%d ", n);
    for (size_t k = 0; k < PPOPCNT_NUMBER_METHODS; k++) {
      printf("\t");
      fflush(NULL);
      double speed = benchmark(vdata,n, pospopcnt_u16_methods[k]);
      printf("%f\t",speed);
    }
    printf("\n");
  }
  return EXIT_SUCCESS;
}
#else //  __linux__

#include <stdio.h>
#include <stdlib.h>

int main() {
  printf("This is a linux-specific benchmark\n");
  return EXIT_SUCCESS;
}

#endif
