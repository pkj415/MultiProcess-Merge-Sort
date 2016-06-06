#include <stdio.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

void DieWithError(char* error){
	perror(error);
	exit(1);
}

int comparatorFunc(int i, int j){
	if(i<=j)
		return 1;
	else
		return 0;
}

void merge(int * array, int start, int end, int (*comparatorFunc)(int, int)){
	int mid = (start + end)/2;
	int i,j;
	i = start;
	j = mid+1;
	int numOfElem = end-start+1;
	int result[numOfElem];
	int crwl=0;
	while(i<=mid&&j<=end){
		if(comparatorFunc(array[i],array[j])){
			result[crwl]=array[i];
			i++;
			crwl++;
		}else{
			result[crwl]=array[j];
			j++;
			crwl++;
		}
	}
	if(i<=mid){
		for(;i<=mid;i++){
			result[crwl]=array[i];
			crwl++;
		}
	}
	if(j<=end){
		for(;j<=end;j++){
			result[crwl]=array[j];
			crwl++;
		}
	}
	crwl=0;
	//printf("Merge Result : \t");
	for(i=start;i<=end;i++){
		array[i]=result[crwl];
		crwl++;
		//printf("%d  ",array[i]);
	}

}

void mergesort(int *array, int start, int end, int (*comparatorFunc)(int, int), int level){
	if(start==end){
		//printf("Leaf %d %d .\n",start,end);
		return;
	}
	int chld1, chld2;
	int numOfElem = end-start+1;
	int mid = (start + end)/2;
	//printf("Node %d %d %d  Level %d.\n",start, mid, end, level);
	struct sembuf sb;
  	int semid = semget (IPC_PRIVATE, 1, IPC_CREAT | 0666);
  	if (semid < 0){
  		DieWithError("Error creating Semaphores.\n");
  	}
	if (semctl (semid, 0, SETVAL, 2) == -1)
		DieWithError("Error setting sem values.\n");

	if((chld1=fork())==0){
		mergesort(array, start, mid, comparatorFunc, level+1);
		shmdt(array);
		sb.sem_num = 0;
	    sb.sem_op = -1;
	    sb.sem_flg = 0;
	    if (semop (semid, &sb, 1) == -1)
		{
		  DieWithError("Child SemOp failed.\n");
		}
		exit(0);
	}
	else{
		if((chld2=fork())==0){
			mergesort(array, mid+1, end, comparatorFunc, level+1);
			shmdt(array);
			sb.sem_num = 0;
		    sb.sem_op = -1;
		    sb.sem_flg = 0;
		    if (semop (semid, &sb, 1) == -1)
			{
			  DieWithError("Child SemOp failed.\n");
			}
			exit(0);
		}		
	}
	sb.sem_num = 0;
    sb.sem_op = 0;
    sb.sem_flg = 0;
    if (semop (semid, &sb, 1) == -1)
	{
	  printf("Parent SemOp failed.\n");
	}
	if(semctl(semid,0,IPC_RMID)==-1){
		printf("Semaphore couldn't be removed aftr use.\n");
	}
	merge(array, start, end, comparatorFunc);
	printf("PID : %d \t Merged sub-array (%d,%d): ",getpid(),start, end);
	int i;
	for(i=start;i<=end;i++){
		printf("%d ",array[i]);
	}
	printf("\n");
}
int main(int argc, char* argv[]){
	/*if(argc!=3){
		printf("Format is : ./a.out <num of integers> <filename>\n");
		exit(0);
	}*/
	signal(SIGCHLD, SIG_IGN);
	key_t key;
	if((key = ftok("input.txt",'r'))==-1){
		DieWithError("Key couldn't be obtained using input.txt.\n");
	}

	FILE *inp;
    inp = fopen("input.txt", "r");
    int i;
    int array_size;
    if (inp == NULL)
    {
        DieWithError("Error Reading File\n");
    }
    fscanf(inp, "%d\n", &array_size);
    int shmid, shmflg = IPC_CREAT | 0666;
	if((shmid = shmget(key, array_size*sizeof(int), shmflg)) == -1){
		perror("shmget: shmget failed.\n");
		exit(1);
	}
	int *array;
	if((array = (int*) shmat(shmid, NULL, 0)) == (int*) -1 ){
		DieWithError("Shared memory attach failed.\n");
	}
    
    for (i = 0; i < array_size; i++)
    {
        fscanf(inp, "%d\n", array+i);
    }
    fclose(inp);

	

	
  	/*if (semid < 0){
  		DieWithError("Error creating Semaphores.\n");
  	}
	if (semctl (semid, 0, SETALL, 0) == -1)
		DieWithError("Error setting sem values.\n");*/
	int level = 0;
	/*printf("Input numbers : \t");
	for (i = 0; i < array_size; i++)
    {
           printf("%d  ", array[i]);
    }
    printf("\n");*/
	mergesort(array, 0, array_size - 1, comparatorFunc, level);
	printf("Sorted numbers :  ");
	for (i = 0; i < array_size; i++)
    {
           printf("%d  ", array[i]);
    }
    printf("\n");
	shmdt(array);
	if(shmctl(shmid, IPC_RMID, NULL)==-1){
		perror("Shared Memory couldn't be detached.\n");
	}
	return 0;
}
