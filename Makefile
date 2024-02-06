OPTFLAGS  := -O3 -march=native
CFLAGS     = -std=c99 -DFLAGSIZE=$(FLAGSIZE) $(OPTFLAGS) $(DEBUG_FLAGS)
FLAGSIZE   = 32
#FLAGSIZE  = 64
CPPFLAGS   = -std=c++0x -DFLAGSIZE=$(FLAGSIZE) $(OPTFLAGS) $(DEBUG_FLAGS)

# Default target
all: instrumented_benchmark golike_benchmark

# Generic rules
itest: instrumented_benchmark
	$(CXX) --version
	./instrumented_benchmark

DEPS=benchmark/linux/linux-perf-events.h \
    include/pospopcnt_avx512bw.h  \
    include/pospopcnt.h \
    asm/countavx512_$(FLAGSIZE).S \
    asm/kernelavx512.S

stream_benchmark: $(DEPS)  benchmark/linux/stream_benchmark.cpp
	$(CXX) $(CPPFLAGS) benchmark/linux/stream_benchmark.cpp -Iinclude -Ibenchmark/linux -o $@

instrumented_benchmark: $(DEPS) benchmark/linux/instrumented_benchmark.cpp
	$(CXX) $(CPPFLAGS) benchmark/linux/instrumented_benchmark.cpp asm/countavx512_$(FLAGSIZE).S -Iinclude -Ibenchmark/linux -o $@

instrumented_benchmark_align64: $(DEPS)
	$(CXX) $(CPPFLAGS) benchmark/linux/instrumented_benchmark.cpp asm/countavx512_$(FLAGSIZE).S -DALIGN -Iinclude -Ibenchmark/linux -o $@

golike_benchmark: $(DEPS) benchmark/golike/benchmark.c
	$(CC) $(CFLAGS) -Iinclude -o $@ benchmark/golike/benchmark.c asm/countavx512_$(FLAGSIZE).S

golike_benchmark_align64: $(DEPS) benchmark/golike/benchmark.c
	$(CC) $(CFLAGS) -Iinclude -DALIGN -o $@ benchmark/golike/benchmark.c asm/countavx512_$(FLAGSIZE).S

clean:
	rm -f bench example instrumented_benchmark golike_benchmark

.PHONY: all clean itest
