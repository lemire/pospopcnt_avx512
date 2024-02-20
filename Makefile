OPTFLAGS  := -O3 -march=native
CFLAGS     = -std=c99 -DFLAGSIZE=$(FLAGSIZE) $(OPTFLAGS) $(DEBUG_FLAGS)
FLAGSIZE   = 32
#FLAGSIZE  = 64
CPPFLAGS   = -std=c++0x -DFLAGSIZE=$(FLAGSIZE) $(OPTFLAGS) $(DEBUG_FLAGS)
ARCH      != uname -m

# Default target
all: instrumented_benchmark golike_benchmark

# Generic rules
itest: instrumented_benchmark
	$(CXX) --version
	./instrumented_benchmark

DEPS=benchmark/linux/linux-perf-events.h \
    include/pospopcnt_avx512bw.h  \
    include/pospopcnt.h \

KERNELS= $(KERNELS_$(ARCH))
KERNELS_amd64= $(KERNELS_x86_64)
KERNELS_x86_64= asm/countavx512_$(FLAGSIZE).S \
    asm/countavx2_$(FLAGSIZE).S
KERNELS_arm64= $(KERNELS_aarch64)
KERNELS_aarch64= asm/countneon_$(FLAGSIZE).S

stream_benchmark: $(DEPS)  benchmark/linux/stream_benchmark.cpp
	$(CXX) $(CPPFLAGS) benchmark/linux/stream_benchmark.cpp -Iinclude -Ibenchmark/linux -o $@

instrumented_benchmark: $(DEPS) benchmark/linux/instrumented_benchmark.cpp
	$(CXX) $(CPPFLAGS) benchmark/linux/instrumented_benchmark.cpp $(KERNELS) -Iinclude -Ibenchmark/linux -o $@

instrumented_benchmark_align64: $(DEPS)
	$(CXX) $(CPPFLAGS) benchmark/linux/instrumented_benchmark.cpp $(KERNELS) -DALIGN -Iinclude -Ibenchmark/linux -o $@

golike_benchmark: $(DEPS) benchmark/golike/benchmark.c
	$(CC) $(CFLAGS) -Iinclude -o $@ benchmark/golike/benchmark.c $(KERNELS)

golike_benchmark_align64: $(DEPS) benchmark/golike/benchmark.c
	$(CC) $(CFLAGS) -Iinclude -DALIGN -o $@ benchmark/golike/benchmark.c $(KERNELS)

clean:
	rm -f bench example instrumented_benchmark golike_benchmark

.PHONY: all clean itest
