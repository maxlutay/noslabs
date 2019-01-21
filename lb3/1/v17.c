#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void* sumeven(void * _args);

typedef struct {
    int * data;
    int size;
} Array;

int main(){
    pthread_t workerid;

    srand(77777777);
    
    Array arr;
    arr.size = 10;
    arr.data = (int*)malloc(sizeof(int)*arr.size);
    for(int i = 0; i < arr.size; i++) arr.data[i] = rand() % 9;    


    int iret = pthread_create(&workerid, NULL, sumeven, (void *)&arr);//NULL->default
    
    if(!iret) pthread_join(workerid,NULL);

}

void* sumeven(void* _args){
    Array arr = *((Array*)_args);

    int sum = 0;
    for(int i = 0; i < arr.size; i++){ 
        printf("%d ",arr.data[i]);
        if(arr.data[i]%2 == 0){  sum += arr.data[i]; }
    };
    printf("Sum is: %d \n", sum);
}











