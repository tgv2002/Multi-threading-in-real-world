# Report on question 1

## Basic functioning

1. It has been written in the function sortInput() which takes the input values as required and executes the flow such that concurrent merge sort (two child processes) is executed first, then multi-threaded merge sort is called, and normal merge sort is executed at the end. 
2. The time taken in each case is printed, which is calculated with the help of `clock_gettime()`, for the sake of comparison. 
3. Arrays are stored temporarily to ensure that the original array is only passed to all three cases. 
4. Helper functions like swap() and selectionSort() are used.

## Bonus

1. A third implementation of merge sort is included, which uses multi-threading for concurrent execution. Its performance comparison is also included in this file.

## Multi-process merge sort

1. It involves the functions mergeSort(), selectionSort() and merge() with their corresponding parameters. 
2. In the function mergeSort, the sorting of left and right halves take place in the child processes created using fork(). 
3. Parent waits for the completion of execution of both the children, and after both got terminated, the merge() function is called in the parent, which essentially merges both these sorted halves recursively. 
4. When an array to be sorted has less than 5 elements, selectionSort() is called on this array, as expected.

## Multi-threaded merge sort

1. It involves the functions `threaded_mergeSort(), merge() and selectionSort()` with their corresponding parameters. 
2. A struct data type (arg) is used to create a thread for the array, in the sortInput() function, which is joined after it returns. 
3. The `threaded_mergeSort()` is also a recursive function in which similar threads are created for both the left and right halves and are these threads are joined after their execution. 
4. After these threads complete their execution (sorting), merge function is used to merge both these halves.
5. Selection sort is performed if less than 5 elements are present in the array specified in the struct which contain parameters of that function call.

## Normal merge sort

1. It involves the functions `normal_mergeSort()` and `merge()`, with their own parameters. 
2. The `normal_mergeSort` function is used to implement the normal algorithm of merge sort, in which left and right halves of array are sorted independently, and merged after that.
3. The merge function is used for the latter purpose. 
4. Selection sort is involved in the same ways as above.

## Comparisons

1. On executing the three cases of mergeSort for various test cases, it is observed that normal merge sort turns out to be much faster than multi-threaded merge sort and multi-process merge sort (concurrent). 
2. The relation between multi-threaded process and multi-process merge sort is variable, and it is uncertain when one might exceed the other based on various factors for each, but it is very certain that the normal merge sort is more faster and efficient compared to both, by a significant factor.
3. The reason for the slower functioning of concurrent merge sort (threaded or not threaded) relative to normal merge sort, is because of the overheads due to frequent context switches.

