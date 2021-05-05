#!/bin/bash
exec_file="test_exec"
main="play_ground.c"
#schedulers=("static" "dynamic" "guided")
$chunk_size=10
echo "Chunk_size: $chunk_size"
export_val="static, $chunk_size"
export OMP_SCHEDULE=$export_val
gcc -Wall $main -o $exec_file -fopenmp
./$exec_file $1
rm -f $exec_file
