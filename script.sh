for i in {1..26}
do
	./instrumented_benchmark -c -v -n $((2**i))
	./instrumented_benchmark -c -v -n $((2**(i-1)*3))
done
