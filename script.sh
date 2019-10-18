for i in {10..16} ; do ./instrumented_benchmark -v -n $((2**i)); done
