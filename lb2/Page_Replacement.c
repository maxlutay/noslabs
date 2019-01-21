
#include <stdio.h>
#include <stdlib.h>


/**************************************
 * Параметры памяти и дисковых страниц
 *
 * FrameListPage: Динамически распределенный массив, представляющий страницы памяти
 * FrameNumber: количество страниц в памяти
 * elementCout: Указатель на следующую страницу, которая будет заменена
 *
 * RefString: Последовательность запрашиваемых страниц
 * RefLength: Длина рандомизированной строки
 * RefRange: диапазон номеров страниц в  строке
 **************************************
 */

int RefLength; 

typedef struct
{
    int *FrameListPage;
    int elementCount;	
}PageFrame;

int RefRange, FrameNumber;

PageFrame memory;

int *RefString;
 

/* Дополнительные функции для тестирования */

void GenRefStr();

void InitPageFrame();

void ShowReferenceString();

void ShowPageFrame();


/* Функции алгоритма замещения страниц Fifo, Lru */

int AlgFifo();

int AlgLru();
int AlgOpt();

/* Дополнительные функции*/

int SearchFifo(int PageNumber);
int InsertFifo(int PageNumber);

int SearchLru(int PageNumber);
int InsertLru(int PageNumber,int* pagetable);
void LRUupdatePageTable(int index,int* pagetable);

int SearchOpt(int PageNumber);
int InsertOpt(int PageNumber, int i);

//*/



/*******************************
 *
 *  В основной функции реализован тест для алгоритмов AlgFifo & AlgLru & AlgOpt
 *
 * 1. Инициализация параметров системы
 * 2. Инициализировать страниц памяти 
 * 3. Генерирование рандомной  строки
 * 4. Выполнение алгоритма AlgFifo, вывод PagefaultCount
 * 5. Выполнение алгоритма AlgLru, вывод PagefaultCount
 */


int main(int argc, char* argv[])
{
 
    if( argc < 4 )
    {
        printf("Command format: ./Page_Replacement [RefRange] [FrameNumber] [RefLength]");
		return 1;
    }


    RefRange = atoi(argv[1]);
    FrameNumber = atoi(argv[2]);
    RefLength = atoi(argv[3]);


    GenRefStr();


    InitPageFrame();
    printf("Page_fault of Fifo: %d\n",AlgFifo());
    free(memory.FrameListPage);


    ShowReferenceString();
    InitPageFrame();
    printf("Page_fault of Lru: %d\n",AlgLru());
    free(memory.FrameListPage);
   
    ShowReferenceString();
    InitPageFrame();
    printf("Page_fault of Opt: %d\n",AlgOpt());
    free(memory.FrameListPage);


    free(RefString);	

    return 0;

}


/**********************************
 *
 * Реализация дополнительных функций для тестирования 
 *
 **********************************
 */

void GenRefStr()
{
    int i;
    srand(time(0));
    RefString = (int *)malloc( sizeof(int) * RefLength );
    printf("The randomized Reference String: ");
    for(i=0; i< RefLength; i++)
    {
 	 RefString[i] = rand() % RefRange;
     printf("%d ", RefString[i]);       
    }
    printf("\n");
}


void InitPageFrame()
{
    int i;
    memory.FrameListPage = (int *)malloc( sizeof(int)* FrameNumber );
    memory.elementCount =0;    
    for(i=0; i< FrameNumber; i++)
    {
	    memory.FrameListPage[i] = -1;       
    }
}


void ShowPageFrame()
{
    int i;
    for(i = 0; i < FrameNumber; i++)
    {
        printf("%2d",memory.FrameListPage[i]);
    }
    printf("\n");
}


void ShowReferenceString()
{
   int i;
   printf("\nThe Same Reference String: ");
   for(i=0; i< RefLength; i++)
   {
        printf("%d ", RefString[i]);       
   }
   printf("\n");

}


/**********************************
 **********************************
 *
 * Шаблон кода для  AlgFifo & AlgLru 
 * 
 * Примечание: ВАМ не обязательно следовать коду шаблона представленого здесь 
 *       разрешаеться создавать новые структуры данных и изменять функции  int AlgFifo(); int AlgLru();
 *       НО убедитесь, что ваш алгоритм правильный и возвращает PagefaultCount !!!!!!
 *       Рекомендуется распечатать PageFrames
 *       для чтобы вы могли следить за тем, как работает алгоритм, и проверить его.
 *
 **********************************
 */


int AlgFifo()
{
    int PagefaultCount=0;
    int i;

    for( i=0; i<RefLength; i++ ) 
    {
        PagefaultCount+=InsertFifo(RefString[i]);
        ShowPageFrame();
    }


    return PagefaultCount;
}


///*  ВАША РЕАЛИЗАЦИЯ АЛГОРИТМОВ
int SearchFifo(int PageNumber)
{

for(int i = 0; i < FrameNumber; i++){
    if(memory.FrameListPage[i] == PageNumber) return 1;
};
return 0;
    

}

int InsertFifo(int PageNumber)
{
    int Pagefault=0;
    if( 0==SearchFifo(PageNumber) )//if notinmemory
    {

        memory.FrameListPage[memory.elementCount] = PageNumber;//write to most old
        memory.elementCount = (memory.elementCount+1) % FrameNumber;//set most old
        Pagefault = 1; 
    }

    return Pagefault;      
}

//*/

///*
int AlgLru()
{
    int PagefaultCount=0;
    int i;
    int* pagetable = (int*) calloc(FrameNumber,sizeof(int));//for each page in mem alloc requestbit 0 fill

    for( i=0; i<RefLength; i++ ) 
    {
        PagefaultCount+=InsertLru(RefString[i], pagetable);
        for(int i = 0; i < FrameNumber; i ++) printf("%d ", pagetable[i]);
        printf("\t");
        ShowPageFrame(); 
    }
    free(pagetable);
    return PagefaultCount;
}
//*/
///*
int SearchLru(int PageNumber)
{
    for(int i = 0; i < FrameNumber; i++){
         if(memory.FrameListPage[i] == PageNumber) return i;
    };
    return -1;
}

int InsertLru(int PageNumber, int* pagetable)
{

    int index = SearchLru(PageNumber);

    if ( -1 == index )//if notfound :
    {//insert number to elementCount
     // set elementCount where pagetable==0
        memory.elementCount = 0;
        while(0!=pagetable[memory.elementCount]){//find where pagetable==0
            memory.elementCount = (memory.elementCount + 1) % FrameNumber;//cyclic next
        }
        memory.FrameListPage[memory.elementCount] = PageNumber;

        LRUupdatePageTable(memory.elementCount, pagetable);
        return 1;//PageFault=1
    };

    //else if found :
    LRUupdatePageTable(index, pagetable);
    return 0;//PageFault=0
}

void LRUupdatePageTable(int index, int* pagetable){ 
    pagetable[index]=1;//update index
    for(int i = 0; i < FrameNumber; i++) {
        if(0==pagetable[i]) return;//if 0 in pagetable : exit
    };
    for(int i = 0; i < FrameNumber; i++) pagetable[i] = 0;//else if all 1s : set all 0s        
}

//*/

///*
int AlgOpt()
{
    int PagefaultCount=0;
    int i;

   for( i=0; i<RefLength; i++ ) 
   {
       PagefaultCount+=InsertOpt(RefString[i], i);
       ShowPageFrame();
   }


   return PagefaultCount;
}

//*/
///*  ВАША РЕАЛИЗАЦИЯ АЛГОРИТМОВ
int SearchOpt(int PageNumber)
{
    return SearchFifo(PageNumber);
}

int InsertOpt(int PageNumber, int refi )
{
    
    if( 0==SearchFifo(PageNumber) )
    {
        memory.elementCount = 0;
        
        int* counts = (int*) calloc(FrameNumber,sizeof(int)); 
        for(int i = 0; i < FrameNumber; i++){
            int k = 0;
            while((refi+k) < RefLength && memory.FrameListPage[i] != RefString[refi + k]){
                counts[i]++;
                k++;
            };
            if(counts[i] > counts[memory.elementCount]){ memory.elementCount = i;};
            printf("%2d ", counts[i] );  
        }       
        printf("\t"); 
        memory.FrameListPage[memory.elementCount] = PageNumber;
        return 1;
    };
    for(int i = 0; i < FrameNumber; i++) printf("   ");
    printf("\t");
    return 0;  
}

//*/



