#!/bin/bash
if [ $1 -eq 1 ] ; then
	for i in $(seq 2 60); do 
		mpirun -n $i ./main 100 --log --delay 10 || exit 1; 
	done
elif [ $1 -eq 2 ] ; then
	for i in $(seq 2 60); do 
		mpirun -n $i ./main 100 --log --delay 1 || exit 2; 
	done
elif [ $1 -eq 3 ] ; then
	for i in $(seq 10 200); do 
		mpirun -n 9 ./main $i --log --delay 1 || exit 2; 
	done
else 
	echo "No test with this id!"
	exit 1;
fi
