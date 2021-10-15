for i in {10..25} ; do ./instrumented_benchmark -v -n $((2**i)); done
