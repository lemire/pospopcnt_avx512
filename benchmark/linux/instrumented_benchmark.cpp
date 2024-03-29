#include <cinttypes>
#if FLAGSIZE == 64
typedef uint64_t flags_type;
#else
typedef uint32_t flags_type;
#endif

#ifdef __linux__
#include <cassert>
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
#ifdef __x86_64__
#include "pospopcnt_avx512bw.h"
#endif
#include "pospopcnt.h"
#ifdef ALIGN
#include "memalloc.h"
#define memory_allocate(size) aligned_alloc(64, (size))
#else
#define memory_allocate(size) malloc(size)
#endif

// command line options
enum {
  OPT_VERBOSE    = 1 << 0,
  OPT_TEST       = 1 << 1,
  OPT_COMPENSATE = 1 << 2,
  OPT_TOUCH      = 1 << 3,
  OPT_FORCE      = 1 << 4,
};

// Function pointer definition.
typedef void (*pospopcnt_u16_method_type)(const uint16_t *data, uint32_t len,
                                          flags_type *flags);

#ifdef __x86_64__
extern "C" void count16avx512(flags_type flags[16], const uint16_t *buf, size_t len);
extern "C" void count16avx2(flags_type flags[16], const uint16_t *buf, size_t len);

static void pospopcnt_count16avx512(const uint16_t *data, uint32_t len, flags_type *flags)
{
	count16avx512(flags, data, len);
}

static void pospopcnt_count16avx2(const uint16_t *data, uint32_t len, flags_type *flags)
{
	count16avx2(flags, data, len);
}
#endif

#ifdef __aarch64__
extern "C" void count16neon(flags_type flags[16], const uint16_t *buf, size_t len);

static void pospopcnt_count16neon(const uint16_t *data, uint32_t len, flags_type *flags)
{
	count16neon(flags, data, len);
}
#endif

// dummy for taking the overhead
static void pospopcnt_dummy(const uint16_t *data, uint32_t len, flags_type *flags)
{
	(void)data;
	(void)len;
	(void)flags;
}

static const struct {
  pospopcnt_u16_method_type method;
  const char *name;
} methods[] = {
  { pospopcnt_u16_scalar, "pospopcnt_u16_scalar" },
#ifdef __x86_64__
  { pospopcnt_u16_avx512bw_harvey_seal_1KB, "pospopcnt_u16_avx512bw_harvey_seal_1KB" },
  { pospopcnt_u16_avx512bw_harvey_seal_512B, "pospopcnt_u16_avx512bw_harvey_seal_512B" },
  { pospopcnt_u16_avx512bw_harvey_seal_256B, "pospopcnt_u16_avx512bw_harvey_seal_256B" },
  { pospopcnt_count16avx512, "pospopcnt_count16avx512" },
  { pospopcnt_count16avx2, "pospopcnt_count16avx2" },
#endif
#ifdef __aarch64__
  { pospopcnt_count16neon, "pospopcnt_count16neon" },
#endif
  { NULL, NULL },
};

void print16(flags_type *flags) {
  for (int k = 0; k < 16; k++)
    printf(" %8ju ", (uintmax_t)flags[k]);
  printf("\n");
}

std::vector<unsigned long long>
compute_mins(std::vector<std::vector<unsigned long long> > allresults) {
  if (allresults.size() == 0)
    return std::vector<unsigned long long>();

  std::vector<unsigned long long> answer = allresults[0];

  for (size_t k = 1; k < allresults.size(); k++) {
    assert(allresults[k].size() == answer.size());
    for (size_t z = 0; z < answer.size(); z++) {
      if (allresults[k][z] < answer[z])
        answer[z] = allresults[k][z];
    }
  }
  return answer;
}

std::vector<double>
compute_averages(std::vector<std::vector<unsigned long long> > allresults) {
  if (allresults.size() == 0)
    return std::vector<double>();

  std::vector<double> answer(allresults[0].size());

  for (size_t k = 0; k < allresults.size(); k++) {
    assert(allresults[k].size() == answer.size());
    for (size_t z = 0; z < answer.size(); z++) {
      answer[z] += allresults[k][z];
    }
  }

  for (size_t z = 0; z < answer.size(); z++) {
    answer[z] /= allresults.size();
  }
  return answer;
}

class BenchmarkState {
  std::vector<int> evts = {
    PERF_COUNT_HW_CPU_CYCLES,
    PERF_COUNT_HW_INSTRUCTIONS,
    PERF_COUNT_HW_BRANCH_MISSES,
    PERF_COUNT_HW_CACHE_REFERENCES,
    PERF_COUNT_HW_CACHE_MISSES,
    PERF_COUNT_HW_REF_CPU_CYCLES
  };

  BenchmarkState *overhead = nullptr;
  LinuxEvents<PERF_TYPE_HARDWARE> unified;
  std::vector<unsigned long long> results; // tmp buffer
  std::chrono::time_point<std::chrono::steady_clock> start;
  std::vector<std::vector<unsigned long long> > allresults;
  std::vector<double> timings;
  std::vector<double> freqs;
  bool in_progress = false;

public:
  BenchmarkState() : unified(evts) {
    results.resize(evts.size());
  }

  BenchmarkState(BenchmarkState *oh) : unified(evts) {
    overhead = oh;
    results.resize(evts.size());
  }

  // begin measurement of a benchmark iteration
  void begin() {
    assert(!in_progress);
    in_progress = true;

    start = std::chrono::steady_clock::now();
    unified.start();
  }

  // end measurement of a benchmark iteration
  void end() {
    assert(in_progress);

    unified.end(results);
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> secs = end - start;
    double time_in_s = secs.count();
    timings.push_back(time_in_s);
    freqs.push_back(results[0]/(1e9*time_in_s));
    allresults.push_back(results);

    in_progress = false;
  }

  void printResults(bool verbose, uint32_t n, uint32_t m)
  {
    std::vector<unsigned long long> mins = compute_mins(allresults);
    std::vector<double> avg = compute_averages(allresults);
    double min_timing = *min_element(timings.begin(), timings.end());
    double min_freq = *min_element(freqs.begin(), freqs.end());
    double max_freq = *max_element(freqs.begin(), freqs.end());

    // compensate for measuring overhead by subtracting the overhead
    if (overhead != nullptr) {
      std::vector<unsigned long long> oh_mins = compute_mins(overhead->allresults);
      std::vector<double> oh_avg = compute_averages(overhead->allresults);
      min_timing -= *min_element(overhead->timings.begin(), overhead->timings.end());

      assert(mins.size() == oh_mins.size());
      for (size_t i = 0; i < mins.size(); i++) {
        mins[i] -= oh_mins[i];
        avg[i] -= oh_avg[i];
      }
    }

    double speedinGBs = (m * n * sizeof(uint16_t)) / (min_timing * 1e9);

    if (verbose) {
      printf("instructions per cycle %4.2f, cycles per 16-bit word: %4.3f, "
             "instructions per 16-bit word %4.3f \n",
             double(mins[1]) / mins[0], double(mins[0]) / (m * n),
             double(mins[1]) / (n * m));
      // first we display mins
      printf("min: %8llu cycles, %8llu instructions, \t%8llu branch mis., %8llu "
             "cache ref., %8llu cache mis.\n",
             mins[0], mins[1], mins[2], mins[3], mins[4]);
      printf("avg: %8.1f cycles, %8.1f instructions, \t%8.1f branch mis., %8.1f "
             "cache ref., %8.1f cache mis.\n",
             avg[0], avg[1], avg[2], avg[3], avg[4]);
      printf(" %4.3f GB/s \n", speedinGBs);
      printf("estimated clock in range %4.3f GHz to %4.3f GHz\n", min_freq, max_freq);
    } else {
      printf("cycles per 16-bit word:  %4.3f; ref cycles per 16-bit word: %4.3f; speed in GB/s %4.3f \n",
      double(mins[0]) / (n * m), double(mins[5]) / (n * m), speedinGBs);
    }
  }
};

// initialise all subarrays of the vdata array
template <class C>
void init_vdata(C &vdata)
{
  std::mt19937 gen;

  for (size_t k = 0; k < vdata.size(); k++) {
    for (size_t k2 = 0; k2 < vdata[k].size(); k2++) {
      vdata[k][k2] = gen() & 0xffff; // initialise to random integer
    }
  }
}

// Read all array entries and sum them up.  Discard the sum.
// The purpose of this is to ensure that the array has been
// recently accessed.
template <class C>
void touch(C &vdata)
{
  int sum;
  volatile int total;

  for (size_t k = 0; k < vdata.size(); k++) {
    sum += vdata[k];
  }

  total = sum;
}

/**
 * @brief
 *
 * @param n          Number of integers.
 * @parem m          Number of arrays.
 * @param iterations Number of iterations.
 * @param fn         Target function pointer.
 * @param options    Command line options
 * @return           Benchmark results.
 */
template <class C>
BenchmarkState benchmarkMany(C & vdata, BenchmarkState *overhead, uint32_t n, uint32_t m,
                   uint32_t iterations,
                   pospopcnt_u16_method_type fn, int options) {
#ifdef ALIGN
  for (auto &x : vdata) {
    assert(get_alignment(x.data()) == 64);
  }
#endif
  BenchmarkState bench(overhead);

  init_vdata(vdata);

  uint32_t test_iterations = 1; // we run one test iteration
  for (uint32_t i = 0; i < test_iterations; i++) {
    std::vector<std::vector<flags_type> > correctflags(m,
                                                     std::vector<flags_type>(16));
    for (size_t k = 0; k < m; k++) {
      pospopcnt_u16_scalar(vdata[k].data(), vdata[k].size(),
                           correctflags[k].data()); // this is our gold standard
    }
    std::vector<std::vector<flags_type> > flags(m, std::vector<flags_type>(16));
    for (size_t k = 0; k < m; k++) {
      fn(vdata[k].data(), vdata[k].size(), flags[k].data());
    }

    uint64_t tot_obs = 0;
    for (size_t km = 0; km < m; ++km)
      for (size_t k = 0; k < 16; ++k)
        tot_obs += flags[km][k];
    if (tot_obs == 0 && options & OPT_TEST) { // when a method is not supported it returns all zero
      printf("method not supported\n");
    }
    for (size_t km = 0; km < m; ++km) {
      for (size_t k = 0; k < 16; k++) {
        if (correctflags[km][k] != flags[km][k]) {
          if (options & OPT_TEST) {
            printf("bug:\n");
            printf("expected : ");
            print16(correctflags[km].data());
            printf("got      : ");
            print16(flags[km].data());
          }
        }
      }
    }
  }

  for (uint32_t i = 0; i < iterations; i++) {
    std::vector<std::vector<flags_type> > flags(m, std::vector<flags_type>(16));
    bench.begin();
    for (size_t k = 0; k < m; k++) {
      if (options & OPT_TOUCH) {
        touch(vdata[k]);
      }
      fn(vdata[k].data(), vdata[k].size(), flags[k].data());
    }
    bench.end();
  }

  bench.printResults(options & OPT_VERBOSE, n, m);

  return bench;
}

template <class C>
void  benchmarkCopy(C & vdata, BenchmarkState *overhead, uint32_t n, uint32_t m,
                    uint32_t iterations, int options) {
  size_t maxsize = 0;
#ifdef ALIGN
  for (auto &x : vdata) {
     if(maxsize < x.size()) maxsize = x.size();
     assert(get_alignment(x.data()) == 64);
  }
#endif
  for (auto &x : vdata) {
     if(maxsize < x.size()) maxsize = x.size();
  }

  init_vdata(vdata);

  BenchmarkState bench(overhead);
  std::vector<uint16_t> copybuf(maxsize);

  for (uint32_t i = 0; i < iterations; i++) {
    std::vector<std::vector<flags_type> > flags(m, std::vector<flags_type>(16));
    bench.begin();
    for (size_t k = 0; k < m; k++) {
      if (options & OPT_TOUCH) {
        touch(vdata[k]);
      }
      ::memcpy(copybuf.data(),vdata[k].data(),vdata[k].size());
    }
    bench.end();
  }

  bench.printResults(options & OPT_VERBOSE, n, m);
}

static void print_usage(char *command) {
  printf(" Try %s -n 100000 -i 15 -v\n", command);
  printf("-c compensate overhead in measurements\n");
  printf("-f force use of suboptimal benchmark parameters\n");
  printf("-m number of arrays\n");
  printf("-n number of 16-bit words per array\n");
  printf("-i number of iterations\n");
  printf("-t load arrays into cache before benchmarking\n");
  printf("-v enable verbose (perf counter) output\n");
}

int main(int argc, char **argv) {
  size_t n = 10000000;
  size_t m = 1;
  size_t iterations = 0;
  int options = OPT_TEST;
  int c;

  while ((c = getopt(argc, argv, "cfi:hm:n:tv")) != -1) {
    switch (c) {
    case 'c':
      options |= OPT_COMPENSATE;
      break;
    case 'f':
      options |= OPT_FORCE;
      break;
    case 't':
      options |= OPT_TOUCH;
      break;
    case 'n':
      n = atoll(optarg);
      break;
    case 'm':
      m = atoll(optarg);
      break;
    case 'v':
      options |= OPT_VERBOSE;
      break;
    case 'h':
      print_usage(argv[0]);
      return EXIT_SUCCESS;
    case 'i':
      iterations = atoi(optarg);
      break;
    default:
      print_usage(argv[0]);
      return EXIT_FAILURE;
    }
  }

  if (n > UINT32_MAX) {
    printf("setting n to %u \n", UINT32_MAX);
    n = UINT32_MAX;
  }

  if (iterations > UINT32_MAX) {
    printf("setting iterations to %u \n", UINT32_MAX);
    iterations = UINT32_MAX;
  }

  if (iterations == 0) {
    iterations = 100;
  }

  size_t min_volume = 1000000;
  if(~options & OPT_FORCE && m * n < min_volume) {
    printf("The benchmark is designed to measure the time in units of m*n inputs.\n");
    printf("But your choices make m*n too small, so increasing m.\n");
    while(m * n < min_volume) {
       m++;
    }
  }

  printf("n = %zu m = %zu \n", n, m);
  printf("iterations = %zu \n", iterations);
  if (n == 0) {
    printf("n cannot be zero.\n");
    return EXIT_FAILURE;
  }

  size_t array_in_bytes = sizeof(uint16_t) * n * m;
  if (array_in_bytes < 1024) {
    printf("array size: %zu B\n", array_in_bytes);
  } else if (array_in_bytes < 1024 * 1024) {
    printf("array size: %.3f kB\n", array_in_bytes / 1024.);
  } else {
    printf("array size: %.3f MB\n", array_in_bytes / (1024 * 1024.));
  }

  int maxtrial = 3;
#ifdef ALIGN
  std::vector<std::vector<uint16_t, AlignedSTLAllocator<uint16_t, 64> > > vdata(
      m, std::vector<uint16_t, AlignedSTLAllocator<uint16_t, 64> >(n));
#else
  std::vector<std::vector<uint16_t> > vdata(m, std::vector<uint16_t>(n));
#endif

  printf("%-40s\t", "overhead");
  auto ohbench = benchmarkMany(vdata, nullptr, n, m, iterations, pospopcnt_dummy, options & ~OPT_TEST);
  BenchmarkState *overhead = options & OPT_COMPENSATE ? &ohbench : nullptr;

  printf("%-40s\t", "memcpy");
  benchmarkCopy(vdata, overhead, n, m, iterations, options);
  printf("\n");
   
  for (int t = 0; t < maxtrial; t++) {
    printf("\n== Trial %d out of %d \n", t + 1, maxtrial);
    for (size_t k = 0; methods[k].name != NULL; k++) {
      printf("\n");
      printf("%-40s\t", methods[k].name);
      fflush(NULL);
      benchmarkMany(vdata, overhead, n, m, iterations, methods[k].method, options);
      if (options & OPT_VERBOSE)
        printf("\n");
    }
  }
  if (~options & OPT_VERBOSE)
    printf("Try -v to get more details.\n");

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
