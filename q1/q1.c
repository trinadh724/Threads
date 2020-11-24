#define _POSIX_C_SOURCE 199309L //required for clock
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>
#include <limits.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <inttypes.h>
#include <math.h>



int * shareMem(size_t size){
     key_t mem_key = IPC_PRIVATE;
     int shm_id = shmget(mem_key, size, IPC_CREAT | 0666);
     return (int*)shmat(shm_id, NULL, 0);
}

void swap(int *xp, int *yp)  
{  
    int temp = *xp;  
    *xp = *yp;  
    *yp = temp;  
}  
  
void selectionSort(int arr[], int l,int r)  
{  
    int i, j, min_idx;  
  
    // One by one move boundary of unsorted subarray  
    for (i = l; i < r; i++)  
    {  
        // Find the minimum element in unsorted array  
        min_idx = i;  
        for (j = i+1; j < r+1; j++)  
        if (arr[j] < arr[min_idx])  
            min_idx = j;  
  
        // Swap the found minimum element with the first element  
        swap(&arr[min_idx], &arr[i]);  
    }  
}  

void merging(int arr[],long long start,long long mid,long long end)
{
	long long len1,len2,i,j,k;
	len1=mid-start+1;
	len2=end-mid-1+1;
	long long right[len2],left[len1];
	//seperating the given array into two arrays so that they are then later on combined
	for(i=0;i<len1;i++)
	{
		left[i]=arr[i+start];
	}
	for(i=0;i<len2;i++)
	{
		right[i]=arr[i+mid+1];
	}
	i=0;j=0;
	k=start;
	while((i<len1)&&(j<len2))
	{
		if(left[i]>right[j])
		{
			arr[k]=right[j];
			j++;
			k++;
		}
		else
		{
			arr[k]=left[i];
			i++;
			k++;
		}
	}
	if(i==len1)
	{
		while(j<len2)
		{
			arr[k]=right[j];
			j++;
			k++;
		}
	}
	else
	{
		while(i<len1)
		{
			arr[k]=left[i];
			i++;
			k++;
		}
	}
}


void normal_mergeSort(int *arr, int low, int high){
     if(high-low+1<=5)
     {
          selectionSort(arr,low,high);  
     }
     else if(low<high){
          int pi=(low+high)/2;
          normal_mergeSort(arr, low, pi);
          normal_mergeSort(arr, pi + 1, high);
          merging(arr,low,pi,high);
     }
}

void mergeSort(int *arr, int low, int high){
     if(high-low+1<=5)//checking if I should do seletion sort.
     {
          selectionSort(arr,low,high);  
     }
     else if(low<high){
          int pi = (low+high)/2;
          int pid1=fork();
          int pid2;
          if(pid1==0){
               mergeSort(arr, low, pi);
               _exit(1);
          }
          else{
               pid2=fork();
               if(pid2==0){
                    mergeSort(arr, pi + 1, high);
                    _exit(1);
               }
               else{
                    int status;
                    waitpid(pid1, &status, 0);
                    waitpid(pid2, &status, 0);
                    merging(arr,low,pi,high);
               }
          }
     }
}

struct arg{
     int l;
     int r;
     int* arr;
};

void *threaded_mergeSort(void* a){
     //note that we are passing a struct to the threads for simplicity.
     struct arg *args = (struct arg*) a;

     int l = args->l;
     int r = args->r;
     int *arr = args->arr;
     if(l>=r) return NULL;
     if(r-l+1<=5)
     {
          selectionSort(arr,l,r);  
     }
     else{

     int ind=(l+r)/2;
     //sort left half array
     struct arg a1;
     a1.l = l;
     a1.r = ind;
     a1.arr = arr;
     pthread_t tid1;
     pthread_create(&tid1, NULL, threaded_mergeSort, &a1);

     //sort right half array
     struct arg a2;
     a2.l = ind+1;
     a2.r = r;
     a2.arr = arr;
     pthread_t tid2;
     pthread_create(&tid2, NULL, threaded_mergeSort, &a2);

     //wait for the two halves to get sorted
     pthread_join(tid1, NULL);
     pthread_join(tid2, NULL);
     merging(arr,l,ind,r);
     }
}

void runSorts(long long int n){

     struct timespec ts;

     //getting shared memory
     int *arr = shareMem(sizeof(int)*(n+1));
     for(int i=0;i<n;i++) scanf("%d", arr+i);

     int brr[n+1];
     for(int i=0;i<n;i++) brr[i] = arr[i];

     printf("Running concurrent_mergesort for n = %lld\n", n);
     clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
     long double st = ts.tv_nsec/(1e9)+ts.tv_sec;

     //multiprocess mergesort
     mergeSort(arr, 0, n-1);
     for(int i=0; i<n; i++){
          printf("%d ",arr[i]);
     }
     printf("\n");
     clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
     long double en = ts.tv_nsec/(1e9)+ts.tv_sec;
     printf("time = %Lf\n", en - st);
     long double t1 = en-st;

     pthread_t tid;
     struct arg a;
     a.l = 0;
     a.r = n-1;
     a.arr = brr;
     printf("Running threaded_concurrent_mergesort for n = %lld\n", n);
     clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
     st = ts.tv_nsec/(1e9)+ts.tv_sec;

     //multithreaded mergesort
     pthread_create(&tid, NULL, threaded_mergeSort, &a);
     pthread_join(tid, NULL);
     for(int i=0; i<n; i++){
          printf("%d ",a.arr[i]);
     }
     printf("\n");
     clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
     en = ts.tv_nsec/(1e9)+ts.tv_sec;
     printf("time = %Lf\n", en - st);
     long double t2 = en-st;

     printf("Running normal_mergesort for n = %lld\n", n);
     clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
     st = ts.tv_nsec/(1e9)+ts.tv_sec;

     // normal mergesort
     normal_mergeSort(brr, 0, n-1);
     for(int i=0; i<n; i++){
          printf("%d ",brr[i]);
     }
     printf("\n");
     clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
     en = ts.tv_nsec/(1e9)+ts.tv_sec;
     printf("time = %Lf\n", en - st);
     long double t3 = en - st;

     printf("normal_mergesort ran:\n\t[ %Lf ] times faster than concurrent_mergesort\n\t[ %Lf ] times faster than threaded_concurrent_mergesort\n\n\n", t1/t3, t2/t3);
     shmdt(arr);
     return;
}

int main(){

     long long int n;
     scanf("%lld", &n);
     runSorts(n);
     return 0;
}
