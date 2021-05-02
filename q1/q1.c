#define _POSIX_C_SOURCE 199309L
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
typedef long long int lli;

struct arg{
	// arguments for multi-threaded merge sort
	lli l;
	lli r;
	lli* arr;
};

void swap(lli *xp, lli *yp)  
{  
	lli temp = *xp;  
	*xp = *yp;  
	*yp = temp;  
}  

void selectionSort(lli* arr, lli starty, lli endy)  
{  
	lli i, j, min_idx;
	lli n = endy - starty + 1;  

	for(i = starty; i < endy; i++)  
	{  
		// Find the minimum element in unsorted array  
		min_idx = i;

		for(j = i+1; j <= endy; j++)  
			if (arr[j] < arr[min_idx])  
				min_idx = j;  

		// Swap the found minimum element with the first element  
		swap(&arr[min_idx], &arr[i]);  
	}  
}  

lli* shareMem(size_t size)
{
	// allocating memory
	key_t mem_key = IPC_PRIVATE;
	lli shm_id = shmget(mem_key, size, IPC_CREAT | 0666);
	return (lli*)shmat(shm_id, NULL, 0);
}

void merge(lli* arr, lli l, lli m, lli r)
{
	lli i, j, k, n1 = m - l + 1, n2 = r - m; 

	/* temp arrays */
	lli L[n1], R[n2]; 

	for (i = 0; i < n1; i++) 
		L[i] = arr[l + i]; 
	for (j = 0; j < n2; j++) 
		R[j] = arr[m + 1 + j]; 

	/* Merge the temp arrays back into arr[l..r]*/

	i = 0; 
	j = 0;  
	k = l; 

	while (i < n1 && j < n2)
	{ 
		if (L[i] <= R[j]) 
		{ 
			arr[k++] = L[i++];  
		}

		else 
		{ 
			arr[k++] = R[j++];  
		}
	} 

	/* Copy the remaining elements of L[], if any */

	while (i < n1) 
	{ 
		arr[k++] = L[i++]; 
	} 

	/* Copy the remaining elements of R[], if any */
	while (j < n2) 
	{ 
		arr[k++] = R[j++];  
	} 
}

void normal_mergeSort(lli *arr, lli low, lli high)
{
	if(low < high)
	{
		if((high - low + 1) < 5)
			selectionSort(arr, low, high);		// selection sort used for smaller arrays
		else
		{
			lli mid = low + ((high - low)/2);
			// sorting left half
			normal_mergeSort(arr, low, mid);
			// sorting right half
			normal_mergeSort(arr, mid + 1, high);
			// merging both halves
			merge(arr, low, mid, high);
		}
	}
}

void mergeSort(lli *arr, lli low, lli high)
{
	if(low < high)
	{     
		int mid = low + ((high - low) / 2);

		if((high - low + 1) < 5)
			selectionSort(arr, low, high);		// selection sort used for smaller sized arrays

		else
		{
			int pid1 = fork();
			int pid2;

			if(pid1==0)
			{
				// left half
				mergeSort(arr, low, mid);
				_exit(1);
			}

			else
			{
				pid2=fork();

				if(pid2==0)
				{
					// right half

					mergeSort(arr, mid + 1, high);
					_exit(1);
				}

				else
				{
					int status;
					waitpid(pid1, &status, 0);
					waitpid(pid2, &status, 0);
					merge(arr, low, mid, high);
				}
			}

			return;
		}
	}
}

void *threaded_mergeSort(void* a)
{
	//Passing a struct to the threads as arguments.
	struct arg *args = (struct arg*) a;
	lli l = args->l;
	lli r = args->r;
	lli *arr = args->arr;

	if(l>r)
		return NULL;

	if((r - l + 1) < 5)
		selectionSort(arr, l, r);	// selection sort used for smaller arrays

	else
	{

		lli mid = l + ((r-l)/2);

		//sort left half array
		struct arg a1;
		a1.l = l;
		a1.r = mid;
		a1.arr = arr;
		pthread_t tid1;

		pthread_create(&tid1, NULL, threaded_mergeSort, &a1);


		//sort right half array
		struct arg a2;
		a2.l = mid+1;
		a2.r = r;
		a2.arr = arr;
		pthread_t tid2;

		pthread_create(&tid2, NULL, threaded_mergeSort, &a2);


		//wait for the two halves to get sorted
		pthread_join(tid1, NULL);
		pthread_join(tid2, NULL);

		merge(arr,l,mid,r);
	}
}

void sortInput()
{
	struct timespec ts;

	lli n;
	scanf("%lld", &n);

	// allocating memory to three different arrays
	lli* arr = shareMem(sizeof(lli)*(n+1));
	lli b[n+1];
	lli c[n+1];

	for(lli i=0;i<n;i++)
	{
		scanf("%lld", arr+i);
		b[i] = arr[i];
		c[i] = b[i];
	}

	printf("Running concurrent_merge-sort for n = %lld\n", n);
	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
	long double st = ts.tv_nsec/(1e9)+ts.tv_sec;

	//multiprocess merge sort
	mergeSort(arr, 0, n-1);

	for(lli i=0; i<n; i++)
	{
		printf("%lld ",arr[i]);
	}

	printf("\n");

	// calculating time taken for multi-process mergesort
	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
	long double en = ts.tv_nsec/(1e9)+ts.tv_sec;
	printf("Time taken for multi-process mergesort = %Lf\n", en - st);
	long double t1 = en-st;
	
	pthread_t tid;
	struct arg a;
	a.l = 0;
	a.r = n-1;
	a.arr = b;
	printf("Running threaded_concurrent_mergesort for n = %lld\n", n);
	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
	st = ts.tv_nsec/(1e9)+ts.tv_sec;

	//multi-threaded mergesort
	pthread_create(&tid, NULL, threaded_mergeSort, &a);
	pthread_join(tid, NULL);

	for(lli i=0; i<n; i++)
	{
		printf("%lld ",a.arr[i]);
	}

	printf("\n");
	// calculating time taken for multi-threaded merge sort
	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
	en = ts.tv_nsec/(1e9)+ts.tv_sec;
	printf("Time taken for multi-threaded mergesort= %Lf\n", en - st);
	long double t2 = en-st;

	printf("Running normal_mergesort for n = %lld\n", n);
	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
	st = ts.tv_nsec/(1e9)+ts.tv_sec;

	//normal mergesort
	normal_mergeSort(c, 0, n-1);

	for(lli i=0; i<n; i++)
	{
		printf("%lld ",c[i]);
	}

	printf("\n");
	// calculating time taken for normal merge sort
	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
	en = ts.tv_nsec/(1e9)+ts.tv_sec;
	printf("Time taken for normal mergesort = %Lf\n", en - st);
	long double t3 = en - st;

	printf("normal_mergeSort ran:\n\t[ %Lf ] times faster than concurrent_mergeSort\n\t[ %Lf ] times faster than threaded_concurrent_mergeSort\n\n\n", t1/t3, t2/t3);
	shmdt(arr);
	return;

}

int main()
{
	sortInput();
	return 0;
}
