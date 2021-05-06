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
unsigned int maxValue = 1000000;//1000000000;
unsigned int BUCKET_N = 10; //10000;
unsigned int THRED_N = 2;
    
double program_start, program_end, generation_time, separating_start, sorting_start, write_start;


struct Bucket {
    int *tab;
    int max_size;
    int current_size;
};

// generowanie tablicy z losowymi liczbami z zakresu 0 - maxValue
double generate_random_array(unsigned int *tab, int table_size){
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
    //double start, end;
    //start = omp_get_wtime();
    #pragma omp parallel default(none) shared(seeds, table_size, tab, maxValue)
    {
        int i;
        unsigned int myseed = seeds[omp_get_thread_num()];
        #pragma omp for schedule(runtime)
        for(i=0 ; i < table_size ; i++){
            tab[i] = rand_r(&myseed)%maxValue;
        }
    }
    //end = omp_get_wtime();
    //printf("Generowanie tablicy: %fs\n", end-start);
    free(seeds);
    return  omp_get_wtime();
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

void free_bucket_arr(struct Bucket* bucket[], int n){ 
    int i;
    for(i=0; i<n; i++){
        free_bucket(bucket[i]);
    }
    return;
}

// powiększenie bucketa jeśli jest zbyt mały
void resize_bucket(struct Bucket *bucket){
    int new_max_size = 2 * bucket->max_size;
    int *tab = realloc(bucket->tab, new_max_size*sizeof(int));
    bucket->tab = tab;
    bucket->max_size = new_max_size;
}

// dodaje wartość do tablicy w kubełku, jeśli tablica za mała powiększa ją
void add_to_bucket(struct Bucket *bucket, int value){
    if(bucket->current_size == bucket->max_size){
        resize_bucket(bucket);
    }
    bucket->tab[bucket->current_size] = value;
    bucket->current_size++;
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

int is_sorted(unsigned int *tab, int table_size){
    int is_sorted = 1;
    int i=0;
    while(i<table_size-1 && is_sorted!=0){
        if(tab[i]>tab[i+1]){
            is_sorted = 0;
        }
        i++;
    }
    return is_sorted;
}
// ⬇️ kod do debugowania
void show_tab(unsigned int *tab, int table_size){
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

void sort_table_parallel3(unsigned int *array, unsigned int table_size, int n_threads, unsigned long int n_buckets){
    struct Bucket** all_buckets = malloc(n_threads * n_buckets * sizeof(struct Bucket*));
    struct Bucket*** buckets = malloc(n_threads * sizeof(struct Bucket*));
    
    {
        int i;
        int subtable_size = table_size * 2 / n_threads / n_buckets;

        for(i=0; i<n_threads * n_buckets; i++){
            all_buckets[i] = declare_bucket(subtable_size);
        }

        for(i=0; i<n_threads; i++){
            buckets[i] = all_buckets + i * n_buckets;
        }
    }

    struct Bucket** sum_buckets = malloc(n_buckets * sizeof(struct Bucket*));

    separating_start = omp_get_wtime(); 
    #pragma omp parallel
    {
        int thread_id = omp_get_thread_num();
        int start_index = table_size * thread_id / n_threads;
        int end_index = table_size * (thread_id + 1) / n_threads;
        int i;
        for(i=start_index; i< end_index; i++){
            int val = array[i];
            add_to_bucket(buckets[thread_id][val*n_buckets/maxValue], val);  
        }
    }

    int* thread_elements_count_sums = malloc(n_threads * sizeof(int));
    
    sorting_start = omp_get_wtime();
    #pragma omp parallel
    {
        int thread_elements_count_sum = 0;
        int thread_id = omp_get_thread_num();
        int start_index = n_buckets * thread_id / n_threads;
        int end_index = n_buckets * (thread_id + 1) / n_threads;
        int i, j, k;  
        for(i=start_index; i< end_index; i++){
            int elem_count_sum = 0;
            for(j = 0; j < n_threads; j++){
                elem_count_sum += buckets[j][i]->current_size;
            }
            sum_buckets[i] = declare_bucket(elem_count_sum);
            for(j = 0; j < n_threads; j++){
                int elem_in_bucket = buckets[j][i]->current_size;
                int* tmp_tab = buckets[j][i]->tab;
                for(k = 0; k < elem_in_bucket; k++){
                    add_to_bucket(sum_buckets[i], tmp_tab[k]);
                }
            }
            buble_sort(sum_buckets[i]);
            thread_elements_count_sum+=elem_count_sum;
        }
        thread_elements_count_sums[thread_id] = thread_elements_count_sum;
    }

    write_start = omp_get_wtime(); 
    #pragma omp parallel
    {
        int thread_id = omp_get_thread_num();
        int array_current_index = 0;
        int i, j;
        for(i=0; i<thread_id; i++){
            array_current_index += thread_elements_count_sums[i];
        }

        int start_index = n_buckets * thread_id / n_threads;
        int end_index = n_buckets * (thread_id + 1) / n_threads;

        for(i = start_index; i < end_index; i++){
            struct Bucket* current_bucket = sum_buckets[i];
            int max_elements = current_bucket->current_size;
            int* current_bucket_table = current_bucket -> tab;
            for(j = 0; j < max_elements; j++){
                array[array_current_index++] = current_bucket_table[j];
            }
        }
    }



    free(thread_elements_count_sums);

    free_bucket_arr(sum_buckets, n_buckets);
    free(sum_buckets);

    free_bucket_arr(all_buckets, n_threads * n_buckets);
    free(all_buckets);
    
    free(buckets);

    return;
}

int main(int argc, char *argv[]){        
    srand(time(NULL));
    unsigned long long table_size = 0;
    if(argc < 4){
        printf("Parameters: table_size, buckets_per_thread, max_threads\n");
        return -1;
    }
    table_size = strtoll(argv[1], NULL, 0);
    BUCKET_N = atoi(argv[2]);
    THRED_N = atoi(argv[3]);
    omp_set_num_threads(THRED_N); // ustawienie liczby wątków
    unsigned int* v = malloc(table_size * sizeof(unsigned int)); // alokowanie początkowej tablicy
    if(v == NULL){
        printf("Could not allocate table_size: %lld\n", table_size);
        return -1;
    }
    int maxNThreads = 1;
    #pragma omp parallel 
    {
        if(omp_get_thread_num() == 0){
            maxNThreads = omp_get_num_threads();
        }
    }

    program_start = omp_get_wtime();
    // przykład:
    generation_time = generate_random_array(v, table_size); // generowanie tablicy
    sort_table_parallel3(v, table_size, maxNThreads, BUCKET_N);
    program_end = omp_get_wtime();
    int sorted;
    sorted = is_sorted(v, table_size); // sprawdzenie czy posortowane 1 => wszystko dobrze, 0 => źle
    printf("Is sorted: %d\n", sorted);
    //show_tab(v, table_size); // funkcja debugująca jeśli chcesz zajrzeć co i jak
    double separation_time = sorting_start - separating_start,
            sorting_time = write_start - sorting_start,
            writing_time = program_end - write_start,
            program_time = program_end - program_start;
    printf("Generacja tablicy: %fs\n", generation_time - program_start);
    //printf("Deklaracja kubełków: %fs\n", separating_start - generation_time);
    printf("Rozdzielenie do kubełków: %fs\n", separation_time);
    printf("Sortowanie: %fs\n", sorting_time);
    printf("Wpisywanie do tablicy: %fs\n", writing_time);
    printf("Całkowity czas wykonania algorytmu: %fs\n", program_time);
    free(v);
    return 0;
}