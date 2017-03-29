#include<pthread.h>
#include<stdio.h>
#include<ctype.h>
#include<math.h>
#include<stdlib.h>
#include<time.h>

struct timespec st,en;

double runtime;
int n,m,p;
int **matrix1, **matrix2, **matrix3;
int Row = 0,Col = 0;

int threadCount;

pthread_t * threads;

pthread_mutex_t mutex_Row = PTHREAD_MUTEX_INITIALIZER;

void *multiply(int d){
    int i,j,myCol,myRow;
    while(1){
        pthread_mutex_lock(&mutex_Row);
        if(Row>=n){
            pthread_mutex_unlock(&mutex_Row);
            pthread_exit(0);
            return;
        }
        myCol = Col;
        myRow = Row;
        Col++;
        if(Col==p){
            Col = 0;
            Row++;
        }
        pthread_mutex_unlock(&mutex_Row);
        for(i=0;i<m;i++){
            matrix3[myRow][myCol] += (matrix1[myRow][i]*matrix2[i][myCol]);
        }
    }
}

int main()
{
    int i,j;
	FILE *fptr;
    printf("Enter the rows and columns of matrices\n - n,m and p\n");
    scanf("%d %d %d", &n, &m, &p);


    matrix1 = (int **) malloc(n * sizeof(int *));
    for(i=0;i<n;i++){
        matrix1[i] = (int *)malloc(m * sizeof(int));
    }

    matrix2 = (int **) malloc(m * sizeof(int *));
    for(i=0;i<m;i++){
        matrix2[i] = (int *)malloc(p * sizeof(int));
    }

    
    matrix3 = (int **) malloc(n * sizeof(int *));
    for(i=0;i<n;i++){
        matrix3[i] = (int *)malloc(p * sizeof(int));
    }
    fptr = fopen("input1.txt", "r");
        for (i=0; i<n; i++)
        {
            for (j=0; j<m; j++)
            {
                fscanf(fptr, "%d", &matrix1[i][j]);
                //printf("%2d", matrix1[i][j]);
            }
           // printf("\n");
        }
        fclose(fptr);

    fptr = fopen("input2.txt", "r");
        for (i=0; i<m; i++)
        {
            for (j=0; j<p; j++)
            {
                fscanf(fptr, "%d", &matrix2[i][j]);
               // printf("%2d", matrix1[i][j]);
            }
          //  printf("\n");
        }
        fclose(fptr);
/*
   

*/
    printf("Enter the number of threads\n");
    scanf("%d", &threadCount);

    threads = (pthread_t *)malloc(sizeof(pthread_t) * threadCount);

    clock_gettime(CLOCK_MONOTONIC,&st);

    for(i=0;i<threadCount;i++){
        pthread_create(&threads[i], NULL, (void *(*) (void *)) multiply, (void *) (i + 1));
    }
    for (i = 0; i < threadCount; i++) {
        pthread_join(threads[i], NULL);
    }
    clock_gettime(CLOCK_MONOTONIC,&en);
    runtime = en.tv_sec - st.tv_sec+(en.tv_nsec-st.tv_nsec)/(1e9);
    printf("With multi threading -- %f\n", runtime);
/*
    */
fptr=fopen("output.txt","w");
    if(fptr==NULL){
        printf("Error!");
        exit(1);
    }
    for(i=0;i<n;i++){
        for(j=0;j<p;j++){
            fprintf(fptr,"%d ",matrix3[i][j]);
        }
        fprintf(fptr,"\n");
    }
    fclose(fptr);
    
    clock_gettime(CLOCK_MONOTONIC,&st);
    int k;
    int sum = 0;

    for(i=0;i<n;i++)
    {
        for(j=0;j<p;j++){
            for(k=0;k<m;k++){
                sum = sum + matrix1[i][k]*matrix2[k][j];
            }
            sum = 0;
        }
    }
    clock_gettime(CLOCK_MONOTONIC,&en);
    runtime = en.tv_sec - st.tv_sec+(en.tv_nsec-st.tv_nsec)/(1e9);	
    printf("Without multi threading -- %f\n", runtime);

    return 0;
}
