#!/bin/bash
exec_file="exec"
main="main.c"
gcc -Wall $main -o $exec_file -fopenmp
./$exec_file $1
