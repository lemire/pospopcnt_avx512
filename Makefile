OPTFLAGS  := -O2 -march=native
CFLAGS     = -std=c99 $(OPTFLAGS) $(DEBUG_FLAGS)
CPPFLAGS   = -std=c++0x $(OPTFLAGS) $(DEBUG_FLAGS)

# Default target
all: instrumented_benchmark

# Generic rules
itest: instrumented_benchmark
	$(CXX) --version
	./instrumented_benchmark

DEPS=benchmark/linux/instrumented_benchmark.cpp \
    benchmark/linux/linux-perf-events.h \
    include/pospopcnt_avx512bw.h  \
    include/pospopcnt.h \
    asm/countavx512.s

stream_benchmark: $(DEPS)  benchmark/linux/stream_benchmark.cpp 
	$(CXX) $(CPPFLAGS) benchmark/linux/stream_benchmark.cpp -Iinclude -Ibenchmark/linux -o $@

instrumented_benchmark: $(DEPS) 
	$(CXX) $(CPPFLAGS) benchmark/linux/instrumented_benchmark.cpp asm/countavx512.s -Iinclude -Ibenchmark/linux -o $@

instrumented_benchmark_align64: $(DEPS) 
	$(CXX) $(CPPFLAGS) benchmark/linux/instrumented_benchmark.cpp asm/countavx512.s -DALIGN -Iinclude -Ibenchmark/linux -o $@

clean:
	rm -f bench example instrumented_benchmark

.PHONY: all clean itest
