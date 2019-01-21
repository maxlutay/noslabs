#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>



void* sumelement(void* _args);
void* work(void* _args);

struct param {
    double* arr;
    int size;
};

sem_t sem1, sem2;

int main(){

    struct param p;

    char* str_arrlen = (char*)malloc(10);
    printf("Enter array length:");
    fgets(str_arrlen,10,stdin);
    p.size = atoi(str_arrlen);

    char* str_data = (char*)malloc(256);
    printf("Enter elements:");
    fgets(str_data,256,stdin);

    char* tmp;
    int i = 0;
    p.arr = (double*)malloc(p.size*sizeof(double));
    tmp = strtok(str_data," ");
    p.arr[i++] = atof(tmp);
    
    while(NULL != (tmp=strtok(NULL," ")) ){  p.arr[i++] = atof(tmp);  }

    p.size = i;
    //p.arr = realloc(p.arr, p.size * sizeof(int));
    

    printf("Entered %d numbers : ",i);
    while(i > 0){ printf("%4.2f ", p.arr[p.size - i--]); };

    printf("\n");
    

    sem_init(&sem1,0,0);
    sem_init(&sem2,0,0);

    pthread_t th_work, th_SumElement;
    int iret_w, iret_se;
    iret_w = pthread_create(&th_work, NULL,work,(void*)&p);//pass ptr to array structure
    iret_se = pthread_create(&th_SumElement, NULL, sumelement, (void*)&p);//-||-
    
    for(int i = 0; i < p.size; i++){ 
        sem_wait(&sem1);
        printf("%4.2f ", p.arr[i]);
        fflush(stdout);
    };
    printf("\n");
    sem_post(&sem2);
    pthread_join(th_SumElement,NULL);
    sem_destroy(&sem1);
    sem_destroy(&sem2);
    exit(0);
}



void* work(void* _args){
    struct param p = *((struct param *)_args);

    

    printf("\nEnter sleep time: ");
    char* str_sleeptime = (char *)malloc(10);
    fgets(str_sleeptime, 20, stdin);
    
    int sleeptime = atoi(str_sleeptime);
    
    double* worker_arr = (double*) malloc(p.size*sizeof(double));

    int li = 0, ri = p.size;
    

    while(li < (ri-1) ){
        int countfound = 0;
        for(int i = (li + 1); i < ri ; i++){
            if(p.arr[li] == p.arr[i]){
                countfound++;
                //swap
                double tmp = p.arr[i];
                p.arr[i] = p.arr[li + countfound];
                p.arr[li + countfound] = tmp;
                //endswap

                sem_post(&sem1);//notify has clone


                               
 
                sleep(sleeptime);
            };
        };
        if(countfound > 0){
            li = li + countfound + 1;
        } else {//countfound=0
            ri--;
            //swap
            double tmp = p.arr[li];
            p.arr[li] = p.arr[ri];
            p.arr[ri]=tmp;
    
            //sem_post(&sem1);//notify no clones
            sleep(sleeptime);
        };

    };
    for(int i = 0; i < p.size; i++) sem_post(&sem1);
}


void* sumelement(void* _args){
    sem_wait(&sem2);
    double sum = 0;
    struct param p = *((struct param*)_args);
    for(int i = 0; i < p.size; i++){
        sum += p.arr[i];
    };
    printf("\nSum %4.2f\n", sum);
    sem_post(&sem2);
}

