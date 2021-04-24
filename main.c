#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

int main(int argc, char *argv[])
{        
    srand(time(NULL));
    unsigned long long table_size = 0;
    if(argc < 2){
        printf("Parameters: table_size\n");
        return -1;
    }
    table_size = strtoll(argv[1], NULL, 0);
    printf("table_size: %lld\n", table_size);

    unsigned int* v = malloc(table_size * sizeof(unsigned int));
    if(v == NULL){
        printf("Could not allocate table_size: %lld\n", table_size);
        return -1;
    }
    //#pragma omp parallel
    {
        int i;
        //#pragma omp for
        for(i=0 ; i < table_size ; i++){
            v[i] = rand();
            //printf("iterację %d wykonuje wątek nr %d wartosc %d \n" , i , omp_get_thread_num(), v[i]);
        }

        
    }

    free(v);
    return 0;
}