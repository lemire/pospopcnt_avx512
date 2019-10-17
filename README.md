
## Position population-count benchmarks

# requirements

- Linux
- x64 processor supporting the AVX512BW extension

# instructions

```
make
./instrumented_benchmark -v
```
# Sample output

On a Cannon Lake processor:

```
$ ./instrumented_benchmark  -v
n = 100000 m = 1 
iterations = 10000 
array size: 195.312 kB
nothing                                         instructions per cycle 0.32, cycles per 16-bit word:  0.000, instructions per 16-bit word 0.000 
min:       41 cycles,       13 instructions,           0 branch mis.,        0 cache ref.,        0 cache mis.
avg:     67.8 cycles,     13.0 instructions,         1.0 branch mis.,      0.0 cache ref.,      0.0 cache mis.

== Trial 1 out of 10 

pospopcnt_u16_scalar                            alignments: 128 
instructions per cycle 3.70, cycles per 16-bit word:  17.853, instructions per 16-bit word 66.001 
min:  1785279 cycles,  6600114 instructions,           2 branch mis.,     1803 cache ref.,        0 cache mis.
avg: 1791763.9 cycles, 6600114.6 instructions,       3.0 branch mis.,   2117.4 cache ref.,      3.8 cache mis.


pospopcnt_u16_avx512bw_harvey_seal              alignments: 128 
instructions per cycle 2.33, cycles per 16-bit word:  0.125, instructions per 16-bit word 0.291 
min:    12502 cycles,    29133 instructions,           2 branch mis.,     1902 cache ref.,        0 cache mis.
avg:  45044.0 cycles,  29133.0 instructions,         2.7 branch mis.,   2186.7 cache ref.,      1.0 cache mis.
```

