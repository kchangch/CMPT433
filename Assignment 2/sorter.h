// sorter.h
// Module to spawn a separate thread to sort random arrays
// (permutations) on a background thread. It provides access to the
// contents of the current (potentially partially sorted) array,
// and to the count of the total number of arrays sorted.
#ifndef _SORTER_H_
#define _SORTER_H_

void Sorter_Shutdown();

// Get the size of the array currently being sorted.
// Set the size the next array to sort (don’t change current array)
int Sorter_getArrayLength(int* arr);

// Get a copy of the current (potentially partially sorted) array.
char* Sorter_getArrayData();
// Get the number of arrays which have finished being sorted.
long long Sorter_getNumberArraysSorted(void);

char* Sorter_showHelp();
int Sorter_processCommand(char* command);

void Sorter_Shutdown();

#endif
