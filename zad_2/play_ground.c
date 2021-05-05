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
pomiary czasu:
    - losowanie liczb
    - rozdział liczb do kubełków
        - w alg. 3 pomiar czasu połączenia kubełków, może być osobno, lub rozdzielone
    - sortowanie kubełków
    - przepisanie kubełków
    -czas wykonania całości

Mini schemat:
    - losowanie liczb
    - rozdział na kubełki
    - sortowanie kubełków
    - przepisanie do tablicy
    - sprawdzenie czy dane wyniki są posortowane (po za pomiarem czasu)

*/
unsigned int maxValue = 1000;//1000000000;
unsigned int BUCKET_N = 10; //10000;
unsigned int THRED_N = 2;

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
    printf("Generowanie tablicy: %fs\n", end-start);
    free(seeds);
    return;
}

// stworzenie kubełka z tablicą o rozmiarze table_size
struct Bucket *declare_bucket(int table_size){ 
    struct Bucket *bucket = malloc(sizeof(struct Bucket));
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

void buble_sort(struct Bucket *bucket){
    int table_size = bucket->current_size;
    int *tab = bucket->tab;
    int i, j, swap;
    for(i=0; i<table_size-1; i++){
        for(j=0; j<table_size-i-1; j++){
            if(tab[j] > tab[j+1]){
                swap = tab[j];
                tab[j] = tab[j+1];
                tab[j+1] = swap;
            }
        }
    }
}

int is_sorted(int *tab, int table_size){
    int sorted = 1;
    int i=0;
    while(i<table_size-1 && sorted!=0){
        if(tab[i]>tab[i+1]){
            //printf("Error %d jest większe niż %d, index: %d\n", tab[i], tab[i+1], i+1);
            sorted = 0;
        }
        i++;
    }
    return sorted;
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
    double start, end;
    start = omp_get_wtime();
    generate_random_array(v, table_size);
    struct Bucket *bucket = malloc(sizeof(struct Bucket));
    bucket->tab = v;
    bucket->max_size = table_size;
    bucket->current_size = table_size;
    buble_sort(bucket);
    if(bucket->tab == v){
        printf("tab == v\n");
        printf("fst elem: tab: %d, v: %d\n", bucket->tab[0], v[0]);
    } else {
        printf("tab != v\n");
    }
    int sorted;
    sorted = is_sorted(v, table_size);
    printf("Is sorted: %d\n", sorted);
    show_tab(v, table_size);
    end = omp_get_wtime();
    printf("Całkowity czas wykonania algorytmu: %fs\n", end-start);
    free(v);
    return 0;
}