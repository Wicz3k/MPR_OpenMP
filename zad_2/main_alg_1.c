#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <omp.h>
//Sebastian Wilk
//Damian Dymek

/*

Niektóre zmienne na przykład maxValue będą zmienione tylko po to żeby móc to uruchamiać w celach testowych 

ISTOTNE: "Do pomiaru użyć omp_get_wtime()"

*/
unsigned int maxValue = 1000;//1000000000;
unsigned int BUCKET_N = 10; //10000;
unsigned int THRED_N = 1;

struct Bucket {
    int *tab;
    int max_size;
    int current_size;
};

// generowanie tablicy z losowymi liczbami z zakresu 0 - maxValue
void generate_random_array(int *tab, int table_size){
    int maxNThreads = 1;
    #pragma omp parallel 
    {
        if(omp_get_thread_num() == 0){
            maxNThreads = omp_get_num_threads();
        }
    }
    unsigned int* seeds;
    seeds = malloc(maxNThreads * sizeof(unsigned int));
    int tn;
    for (tn = 0; tn<maxNThreads; tn++){
        seeds[tn] = rand();
    }
    double start, end;
    struct timeval tval_before, tval_after, tval_result;
    gettimeofday(&tval_before, NULL); // !!! co innego do pomiaru czasu
    start = omp_get_wtime();
    #pragma omp parallel default(none) shared(seeds, table_size, tab, maxValue)
    {
        int i;
        unsigned int myseed = seeds[omp_get_thread_num()];
        #pragma omp for schedule(runtime)
        for(i=0 ; i < table_size ; i++){
            tab[i] = rand_r(&myseed)%maxValue;
        }
    }
    end = omp_get_wtime();
    gettimeofday(&tval_after, NULL); // !! co innego do pomiaru czasu
    timersub(&tval_after, &tval_before, &tval_result);
    printf("Generowanie tablicy: %fs\n", end-start);
    printf("generowanie tablicy: %ld.%06lds\n", (long int)tval_result.tv_sec, (long int)tval_result.tv_usec);
    free(seeds);
    return;
}

// stworzenie kubełka z tablicą o rozmiarze table_size
struct Bucket *declare_bucket(int table_size){ 
    struct Bucket *bucket;
    bucket->tab = malloc(table_size * sizeof(unsigned int));
    bucket->current_size = 0;
    bucket->max_size = table_size;
    return bucket;
}

// czyszczenie pamięci
void free_bucket(struct Bucket *bucket){ 
    free(bucket->tab);
    free(bucket);
    return;
}

// powiększenie bucketa jeśli jest zbyt mały
void resize_bucket(struct Bucket *bucket){
    int new_max_size = 2 * bucket->max_size;
    int *tab = realloc(bucket->tab, new_max_size*sizeof(int));
    bucket->tab = tab;
    bucket->max_size = new_max_size;
}

// ⬇️ kod do debugowania
void show_tab(int *tab, int table_size){
    printf("Show table:\n");
    int i;
    for(i=0; i<table_size; i++){
        printf("%d\n", tab[i]);
    }
    return;
}
void show_bucket(struct Bucket *bucket){
    int i;
    for(i=0; i<bucket->current_size; i++){
        printf("%d\n", bucket->tab[i]);
    }
}
// ⬆️ kod do debugowania

int main(int argc, char *argv[]){        
    srand(time(NULL));
    unsigned long long table_size = 0;
    if(argc < 2){
        printf("Parameters: table_size\n");
        return -1;
    }
    table_size = strtoll(argv[1], NULL, 0);
    omp_set_num_threads(THRED_N); // ustawienie liczby wątków
    unsigned int* v = malloc(table_size * sizeof(unsigned int)); // alokowanie początkowej tablicy
    if(v == NULL){
        printf("Could not allocate table_size: %lld\n", table_size);
        return -1;
    }
    generate_random_array(v, table_size);
    //show_tab(v, table_size);
    free(v);
    return 0;
}