#!/bin/bash
for i in $(seq 2 60); do 
	mpirun -n $i ./main 100 --log --delay 10 || exit 1; 
done
for i in $(seq 2 60); do 
	mpirun -n $i ./main 100 --log --delay 1 || exit 2; 
done
