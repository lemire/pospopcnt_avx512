
## Position population-count benchmarks

# requirements

- Linux, with bare-metal access (you may need root)
- Make sure your processor is in performance mode (not powersaving)
- x64 processor supporting the AVX512BW extension

# instructions

```
make
./instrumented_benchmark -v
```
# Sample output

On a Cannon Lake processor:

```
$ ./instrumented_benchmark -v
n = 10000000 m = 1 
iterations = 100 
array size: 19.073 MB
nothing                                         instructions per cycle 0.20, cycles per 16-bit word:  0.000, instructions per 16-bit word 0.000 
min:       65 cycles,       13 instructions,           1 branch mis.,        0 cache ref.,        0 cache mis.
avg:     69.4 cycles,     13.0 instructions,         1.0 branch mis.,      0.1 cache ref.,      0.1 cache mis.


pospopcnt_u16_scalar                            alignments: 16 
instructions per cycle 3.75, cycles per 16-bit word:  17.325, instructions per 16-bit word 65.000 
min: 173251245 cycles, 650000159 instructions,         3 branch mis.,   407959 cache ref.,   283659 cache mis.
avg: 173473725.4 cycles, 650000160.2 instructions,           8.4 branch mis., 409996.0 cache ref., 295584.1 cache mis.
 0.367 GB/s 
estimated clock in range 3.102 GHz to 3.183 GHz


pospopcnt_u16_avx512bw_harvey_seal_1KB          alignments: 16 
instructions per cycle 0.46, cycles per 16-bit word:  0.497, instructions per 16-bit word 0.227 
min:  4966068 cycles,  2271648 instructions,         114 branch mis.,   547590 cache ref.,   262188 cache mis.
avg: 5296987.3 cycles, 2271648.7 instructions,     137.9 branch mis., 552796.2 cache ref., 275536.8 cache mis.
 12.407 GB/s 
estimated clock in range 2.916 GHz to 3.140 GHz


pospopcnt_u16_avx512bw_harvey_seal_512B         alignments: 16 
instructions per cycle 0.60, cycles per 16-bit word:  0.538, instructions per 16-bit word 0.325 
min:  5382323 cycles,  3245605 instructions,          82 branch mis.,   487390 cache ref.,   268386 cache mis.
avg: 5697539.5 cycles, 3245605.8 instructions,     125.8 branch mis., 498338.1 cache ref., 279114.2 cache mis.
 11.658 GB/s 
estimated clock in range 2.985 GHz to 3.142 GHz


pospopcnt_u16_avx512bw_harvey_seal_256B         alignments: 16 
instructions per cycle 0.84, cycles per 16-bit word:  0.618, instructions per 16-bit word 0.518 
min:  6184692 cycles,  5181931 instructions,         114 branch mis.,   441161 cache ref.,   267157 cache mis.
avg: 6661233.2 cycles, 5181931.2 instructions,     124.0 branch mis., 446684.9 cache ref., 280685.8 cache mis.
 10.162 GB/s 
estimated clock in range 2.956 GHz to 3.148 GHz
```

