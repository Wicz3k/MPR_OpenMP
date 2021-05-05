#!/bin/bash
exec_file="exec"
main="main.c"
results="wyniki.csv"
problem_sizes=(253200000 100000000 10000000 1000000 100000)

gcc -Wall $main -o $exec_file -fopenmp

echo "scheduler_type, chunk_size, problem_size, time_1[s], time_2[s], time_3[s]">> $results
scheduler="static"
chunk_sizes=(1 2 4 8 16 32 64 128 256 512 1024 2048 4096 8192 16384 32768 65536 131072)
schedulers=("static" "dynamic" "guided")
for scheduler in ${schedulers[@]}
do
    for chunk_size in ${chunk_sizes[@]}
    do
        for problem_size in ${problem_sizes[@]}
        do
            export_val="$scheduler, $chunk_size"
            export OMP_SCHEDULE=$export_val
            echo "export val: $OMP_SCHEDULE"
            echo -n "$scheduler, $chunk_size, $problem_size, ">> $results
            ./$exec_file $problem_size>> $results
            echo -n ", ">> $results
            ./$exec_file $problem_size>> $results
            echo -n ", ">> $results
            ./$exec_file $problem_size>> $results
            echo "">> $results
        done
    done
done