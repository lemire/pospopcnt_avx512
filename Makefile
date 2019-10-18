OPTFLAGS  := -O2 -march=native
CFLAGS     = -std=c99 $(OPTFLAGS) $(DEBUG_FLAGS)
CPPFLAGS   = -std=c++0x $(OPTFLAGS) $(DEBUG_FLAGS)

# Default target
all: instrumented_benchmark

# Generic rules
itest: instrumented_benchmark
	$(CXX) --version
	./instrumented_benchmark

DEPS=benchmark/linux/instrumented_benchmark.cpp benchmark/linux/linux-perf-events.h include/pospopcnt_avx512bw.h  include/pospopcnt.h 

instrumented_benchmark: $(DEPS) 
	$(CXX) $(CPPFLAGS) benchmark/linux/instrumented_benchmark.cpp -Iinclude -Ibenchmark/linux -o $@

instrumented_benchmark_align64: $(DEPS) 
	$(CXX) $(CPPFLAGS) benchmark/linux/instrumented_benchmark.cpp -DALIGN -Iinclude -Ibenchmark/linux -o $@

clean:
	rm -f bench example instrumented_benchmark

.PHONY: all clean itest
