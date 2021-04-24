#include <stdio.h>
#include <omp.h>

int main(int argc, char *argv[])
{
    #pragma omp parallel
    {
        int i;
        #pragma omp for
        for(i=0 ; i < 10 ; i++)
            printf("iterację %d wykonuje wątek nr %d \n" , i , omp_get_thread_num());
    }
    return 0;
}