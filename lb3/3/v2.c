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

sem_t sem;
pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;

int main(){

    struct param p;

    char* str_arrlen = (char*)malloc(10);
    printf("Enter array length:");
    fgets(str_arrlen,10,stdin);
    p.size = atoi(str_arrlen);

    char* str_data = (char*)malloc(256);
    printf("Enter elements:");
    fgets(str_data,256,stdin);

    int i = 0;
    p.arr = (double*)malloc(p.size*sizeof(*p.arr));
    char* tmp = strtok(str_data," ");
    p.arr[i++] = atof(tmp);
    
    while(NULL != (tmp=strtok(NULL," ")) ){  p.arr[i++] = atof(tmp);  }

    p.size = i;
    p.arr = realloc(p.arr, p.size * sizeof(*p.arr));
    
    printf("Entered %d numbers : ",i);
    while(i > 0){ printf("%4.2f ", p.arr[p.size - i--]); };
    printf("\n");
    


    sem_init(&sem,0,0);// mode, value
    pthread_mutex_lock(&mut);
    
    pthread_t th_work, th_SumElement;
    int iret_w, iret_se;
    iret_w = pthread_create(&th_work, NULL,work,(void*)&p);//pass ptr on structure
    iret_se = pthread_create(&th_SumElement, NULL, sumelement, (void*)&p);//-||-
    
    for(int i = 0; i < p.size; i++){ 
        sem_wait(&sem);
        printf("%4.2f ", p.arr[i]);
        fflush(stdout);
    };
    printf("\n");

    pthread_mutex_unlock(&mut);

    pthread_join(th_SumElement,NULL);
    sem_destroy(&sem);
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
                
                if(countfound==1){
                    sem_post(&sem);//notify original
                    sleep(sleeptime);
                }; 
                sem_post(&sem);//notify clone

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
            //endswap
        };
    };
    for(int i = ri; ri <= p.size; ri++) {
        sem_post(&sem);//notify rest unique
        sleep(sleeptime);
    };
}


void* sumelement(void* _args){
    pthread_mutex_lock(&mut);
    double sum = 0;
    struct param p = *((struct param*)_args);
    for(int i = 0; i < p.size; i++){
        sum += p.arr[i];
    };
    printf("\nSum %4.2f\n", sum);
    pthread_mutex_unlock(&mut);
}

