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


Algorytm 1:
    - każdy wątek czyta tablicę początkową (lepiej żeby każdy nie startował z tego samego indeksu)
    - każdy wątek posiada własne kubełki z ustalonego dla wątku zakresu liczb
    - każdy wątek sortuje własne kubełki
    - barriera?
    - każdy wątek zapisuje dane z kubełków do odpowiedniego fragmentu tablicy początkowej

*/

unsigned int maxValue = 1000;//1000000000;

struct Bucket {
    int *tab;
    int max_size;
    int current_size;
};

// generowanie tablicy z losowymi liczbami z zakresu 0 - maxValue
double generate_random_array(int *tab, int table_size){
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
    //printf("Generowanie tablicy: %fs\n", end-start);
    free(seeds);
    return end; //- start;
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

// dodaje wartość do tablicy w kubełku, jeśli tablica za mała powiększa ją
void add_to_bucket(struct Bucket *bucket, int value){
    if(bucket->current_size == bucket->max_size){
        resize_bucket(bucket);
    }
    bucket->tab[bucket->current_size] = value;
    bucket->current_size++;
}

// sortowanie bąbelkowe w kubełku
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

// sprawdzenie czy tablica jest malejąca, jeśli tak to zwraca 1
int is_sorted(int *tab, int table_size){
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
    unsigned int THREAD_N = 4;
 

    srand(time(NULL));
    unsigned long long table_size = 0;
    if(argc < 2){
        printf("Parameters: table_size\n");
        return -1;
    }
    table_size = strtoll(argv[1], NULL, 0);
    if(argc > 2){
        int n = atoi(argv[2]);
        if(n>0 && n <=4){
            THREAD_N=n;
        }
    }
    int BUCKET_N = THREAD_N * 10;
    omp_set_num_threads(THREAD_N); // ustawienie liczby wątków
    unsigned int* v = malloc(table_size * sizeof(unsigned int)); // alokowanie początkowej tablicy
    if(v == NULL){
        printf("Could not allocate table_size: %lld\n", table_size);
        return -1;
    }
    double program_start, program_end, generation_time, separating_start, sorting_start, write_start;
    int range_offset = maxValue / THREAD_N;
    int bucket_num = BUCKET_N / THREAD_N;
    int *thread_index_offsest = malloc(THREAD_N * sizeof(int));
    program_start = omp_get_wtime();
    generation_time = generate_random_array(v, table_size); // generowanie tablicy
    #pragma omp parallel firstprivate(range_offset, bucket_num, table_size) shared(v, thread_index_offsest)
    {
        int thread_id = omp_get_thread_num();
        int thread_num = omp_get_num_threads();
        int *tab_ptr = v; // prywatna kopia adresu tablicy
        int range_start, range_end; // początek i koniec przetwarzanego zakresu
        range_start = thread_id * range_offset;
        if(thread_id + 1 == thread_num){
            range_end = maxValue;
        } else {
            range_end = (thread_id + 1) * range_offset;
        }
        int start_index = thread_id * table_size / thread_num; // startowy indeks dla wątku
        int bucket_divider = (range_end - range_start) / bucket_num; // podzielnik wykorzystywany przy obliczaniu indeksu kubełka dla liczby
        struct Bucket **bucket_tab = malloc(bucket_num * sizeof(struct Bucket*)); // tablica kubełków
        int bucket_tab_size = (int) table_size / thread_num / bucket_num * 1.1; // rozmiar tablicy kubełka
        int thread_num_counter = 0; // counter ile dany zestaw kubełków posiada liczb
        int i;
        for(i=0; i<bucket_num; i++){
            bucket_tab[i] = declare_bucket(bucket_tab_size); // tworzenie pustych kubełków
        }
        #pragma omp master
        {
            separating_start = omp_get_wtime(); 
        }
        // odczytanie wartości z tablicy i przypisanie do kubełka jeżeli znajduje się w przetwarzanym zakresie
        for(i=0; i<table_size; i++){
            int value = tab_ptr[(i + start_index) % table_size];
            if(value >= range_start && value < range_end){
                int bucket_index = (value - range_start) / bucket_divider; // interpolacja z zakresu liczbowego na zakres indeksów
                if(bucket_index >= bucket_num){
                    bucket_index = bucket_num - 1;
                }
                add_to_bucket(bucket_tab[bucket_index], value);
                thread_num_counter++;
            }
        }
        thread_index_offsest[thread_id] = thread_num_counter;

        #pragma omp barrier // barriera aby wszystkie wątki wpisały wartość do wspólnej tablicy
        #pragma omp master
        {
            sorting_start = omp_get_wtime(); 
        }

        for(i=0; i<bucket_num; i++){
            buble_sort(bucket_tab[i]);
        }
        #pragma omp barrier // barriera aby wszystkie wątki wpisały wartość do wspólnej tablicy
        #pragma omp master
        {
            write_start = omp_get_wtime(); 
        }
    
        int index = 0; // indeks od którego dany wątek ma zacząć wpisywać swoje wartości
        if(thread_id > 0){
            for(i=0; i<thread_id; i++){
                index += thread_index_offsest[i];
            }
        }
        int j;

        for(i=0; i<bucket_num; i++){
            struct Bucket *tmp = bucket_tab[i];
            for(j=0; j<tmp->current_size; j++){
                tab_ptr[index] = tmp->tab[j];
                index++;
            }
            free_bucket(tmp);
        }
    }
    program_end = omp_get_wtime();
    int sorted;
    sorted = is_sorted(v, table_size); // sprawdzenie czy posortowane 1 => wszystko dobrze, 0 => źle
    //show_tab(v, table_size); // funkcja debugująca jeśli chcesz zajrzeć co i jak
    printf("Is sorted: %s\n", sorted ? "True" : "False");
    
    // program_start, program_end, generation_time, separating_start, sorting_start, write_start;
    double separation_time = sorting_start - separating_start,
            sorting_time = write_start - sorting_start,
            writing_time = program_end - write_start,
            program_time = program_end - program_start;
    printf("Generacja tablicy: %fs\n", generation_time - program_start);
    printf("Deklaracja kubełków: %fs\n", separating_start - generation_time);
    printf("Rozdzielenie do kubełków: %fs\n", separation_time);
    printf("Sortowanie: %fs\n", separation_time);
    printf("Wpisywanie do tablicy: %fs\n", writing_time);
    printf("Całkowity czas wykonania algorytmu: %fs\n", program_time);
    free(v);
    return 0;
}