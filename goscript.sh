max=30
n=10
#output=golike

#for i in `seq 0 $max`
#do
#	for j in `seq $n`
#	do
#		./golike_benchmark $((2**i))
#	done
#done >${output}_ij.out

for j in `seq $n`
do
	./golike_benchmark 1
	for i in `seq 1 $max`
	do
		./golike_benchmark $((2**i))
		./golike_benchmark $((2**(i-1)*3))
	done
done
