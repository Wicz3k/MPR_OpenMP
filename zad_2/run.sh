#!/bin/bash

# sposób użycia: ./run.sh [<rozmiar_tablicy> <scheduler> <chunk_size>]

exec_file="exec"
main="main.c"
size=$1
scheduler=$2
chunk_size=$3
if [ -z ${size} ]; then
size="20"
fi
if [ -z ${scheduler} ]; then
scheduler="static"
fi
if [ -z ${chunk_size} ]; then
chunk_size="4"
fi

gcc $main -o $exec_file -fopenmp
export OMP_SCHEDULE="$scheduler, $chunk_size"
./$exec_file $size
