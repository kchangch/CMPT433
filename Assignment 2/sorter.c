#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <math.h>
#include <ctype.h>
#include <stdbool.h>

#include <pthread.h>
#include <unistd.h>

#include <errno.h>

#include "segDisplay.c"
#include "potManager.c"
#include "networkManager.c"

#define ARRAY_END -1
#define MAX_CHAR_ARR_LEN 8000
#define BUFFER_MAX_SIZE 50000
#define MAX_PACKET_SIZE 1500

int running = 1;
char charArr[MAX_CHAR_ARR_LEN];

// display
long long totalSorted = 0;
int numSorted = 0;
int numSortedDisplay = 0;
int currentSize = 0;
int i2cFileDesc = 0;

// 0: no; 1: yes
int sortingCurrArr = 0;
int* arr = NULL;

// termination condition
bool isDone = false;
bool isReading = false;

// threading
pthread_mutex_t sortingLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t sortingCond = PTHREAD_COND_INITIALIZER;

pthread_mutex_t arrLenLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t arrLenCond = PTHREAD_COND_INITIALIZER;

pthread_mutex_t readLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t readCond = PTHREAD_COND_INITIALIZER;

// display the help dialog when the user requests it
// or an unknown command is entered
char* Sorter_showHelp() {
    char* helpMessage =
    "Accepted command examples:\n"
    "count      -- display number arrays sorted.\n"
    "get length -- display length of array currently being sorted.\n"
    "get array  -- display the full array being sorted.\n"
    "get 10     -- display the tenth element of array currently being sorted.\n"
    "stop       -- cause the server program to end.\n";
    
    return helpMessage;
}

// simply returns the number of arrays sorted so far
long long Sorter_getNumberArraysSorted(void) {
    return totalSorted;
}

// returns the number of elements in the current array
int Sorter_getArrayLength(int* arr) {
    int length = 0;

    pthread_mutex_lock(&arrLenLock);
    
    // inside the critical section, keep counting until we hit the end
    if (arr != NULL) {
        if (sortingCurrArr == 0) {
            pthread_cond_wait(&arrLenCond, &arrLenLock);
        }

        int curr = *(arr+length);

        while (curr != ARRAY_END) {
            length++;
            curr = *(arr+length);
        }

        sortingCurrArr = 1;
    }

    pthread_mutex_unlock(&arrLenLock);
    return length;
}

// finds and returns a specific number from the current array
int Sorter_getNumber(int index) {
    int value_index = -1;
    
    pthread_mutex_lock(&readLock);
    {
        isReading = true;
        if (arr == NULL) {
            pthread_cond_wait(&readCond, &readLock);
        }
        int arrSize = Sorter_getArrayLength(arr);
        if (index < arrSize) {
            value_index = arr[index];   
        }
        isReading = false;
    }
    pthread_mutex_unlock(&readLock);
    pthread_cond_signal(&readCond);
    
    return value_index;
}

// returns the current array being sorted
char* Sorter_getArrayData() {
    memset(charArr, '\0', MAX_CHAR_ARR_LEN);
    char value[6];
    
    pthread_mutex_lock(&readLock);
    {
        isReading = true;
        if (arr == NULL) {
            pthread_cond_wait(&readCond, &readLock);
        }

        const int size = Sorter_getArrayLength(arr);
        int* currentArr = arr;
        
        // Build up the huge string which represents the current state
        // of the current array.  We want to split it every 10 numbers
        // so it fits in the screen when printed.
        for (int i = 1; i < size + 1 ; i++) {
            if (i != size) {
                if (i % 10 != 0) {
                    sprintf(value, "%d, ", currentArr[i - 1]);
                }
                else {
                    sprintf(value, "%d\n", currentArr[i - 1]);
                }
            }
            else {
                sprintf(value, "%d\n", currentArr[i - 1]);
            }

            strcat(charArr, value);
        }
        isReading = false;
    }
    pthread_mutex_unlock(&readLock);
    pthread_cond_signal(&readCond);
    
    return charArr;
}

// Check if the command contains any digit.
// Case for get <some number>
int findDigits(char* command) {
    char number[5] = "";
    char temp[2];
    int value = -1;

    for (int i = 4; i < strlen(command) - 1; i++) {
        if (isdigit(command[i]) != 0) {
            sprintf(temp, "%c", command[i]);
            strcat(number, temp);
        }
        else {
            break;
        }
    }

    if (strlen(number) != 0) {
        value = atoi(number);
    }
    return value;
}

void Sorter_processCommand(char* command, char* response_buffer) {
    // compare the received command to a list of our available ones
    // make sure to ignore the \n character on the end
    memset(response_buffer, '\0', BUFFER_MAX_SIZE);

    if ( strncmp(command, "help", strlen(command)-1) == 0 ) {
        strcpy(response_buffer, Sorter_showHelp());
    }

    else if ( strncmp(command, "count", strlen(command)-1) == 0 ) {
        long long totalCurrSorted = Sorter_getNumberArraysSorted();
        sprintf(response_buffer, "Number of arrays sorted = %lld\n", totalCurrSorted);
    }

    else if ( strncmp(command, "get length", strlen(command)-1) == 0 ) {
        // int len = Sorter_getArrayLength(arr);
        sprintf(response_buffer, "Current array length = %d\n", currentSize);
    }

    else if ( strncmp(command, "get array", strlen(command)-1) == 0 ) {
        // process get array command
        char *current_arr = Sorter_getArrayData();
        if (strlen(current_arr) != 0) {
            strcpy(response_buffer, current_arr);
        }
    }

    else if ( strncmp(command, "get", strlen("get")) == 0 ) {
        // process get <some number> command
        // if the number is greater than size of array -> return error
        int digit = findDigits(command);
        if (digit > currentSize || digit <= 0) {
            sprintf(response_buffer, "Invalid argument. Must be between 1 and %d (array length).\n", currentSize);
        }
        else {
            int value = Sorter_getNumber(digit - 1);
            sprintf(response_buffer, "Value %d = %d\n", digit, value);
        }
    }

    // process stop command
    else if ( strncmp(command, "stop", strlen(command)-1) == 0 ) {
        isDone = true;
    }

    // there was an unknown command entered
    else {
        strcpy(response_buffer, Sorter_showHelp());
    }
}

int* createNewArray(int size) {
    // allocate the new array and return it
    int arrLen = size;
    int arrSpace = (arrLen+1) * sizeof(int);
    arr = (int*)malloc(arrSpace);
    
    // fill the array with numbers and put INT_MAX on the end
    for (int i = 0; i < arrLen; i++) {
        *(arr+i) = i + 1;
        if (i == arrLen-1) {
            *(arr+i+1) = ARRAY_END;
        }
    }

    // shuffle them all
    for (int i = 0; i < arrLen; i++) {
        int j = rand() % arrLen;
        int temp = *(arr+j);
        *(arr+j) = *(arr+i);
        *(arr+i) = temp;
    }

    return arr;
}

int* bubbleSort(int* arr) {
    int arrLen = Sorter_getArrayLength(arr);

    for (int c = 0 ; c < arrLen; c++) {
        for (int d = 0 ; d < arrLen - c - 1; d++) {
            if (*(arr+d) > *(arr+d+1)) {
                pthread_mutex_lock(&sortingLock);
                {
                    int swap = *(arr+d);
                    *(arr+d)   = *(arr+d+1);
                    *(arr+d+1) = swap;
                }
                pthread_mutex_unlock(&sortingLock);
            }
        }
    }

    return arr;
}

// continually creates and sorts arrays
// length changable via POT
void* sorter_thread_method() {
    while (!isDone) {
        pthread_mutex_lock(&arrLenLock);
        
        int size = round(Pot_getApproxReading());
        currentSize = size;
        arr = createNewArray(size);
        sortingCurrArr = 1;
        pthread_mutex_unlock(&arrLenLock);
        
        if (sortingCurrArr == 1) {
            pthread_cond_signal(&arrLenCond);
        }

        if (arr != NULL) {
            pthread_cond_signal(&readCond);
        }

        bubbleSort(arr);

        // this counts the total sorted during execution
        totalSorted++;

        // this lock is released every 1.0 seconds
        // to count the sorting frequency
        pthread_mutex_lock(&sortingLock);
        numSorted++;
        pthread_mutex_unlock(&sortingLock);
        
        if (numSorted > 0) {
            pthread_cond_signal(&sortingCond);
        }

        pthread_mutex_lock(&readLock);
        {
            if (isReading) {
                pthread_cond_wait(&readCond, &readLock);
            }
            free(arr);
            arr = NULL;
        }
        pthread_mutex_unlock(&readLock);

    }
    pthread_exit(NULL);
    return NULL;
}

void send_response_to_client(char *response_buffer, socklen_t clientAddressLen) {
    // send the server's response back to the client
    sendto(
        sockfd,
        (const char *)response_buffer,
        strlen(response_buffer), 
        MSG_CONFIRM,
        (const struct sockaddr *) &cliaddr,
        clientAddressLen
    );
}

void* udp_in_method() {
    char response_buffer[BUFFER_MAX_SIZE];
    // char packet_buffer[MAX_PACKET_SIZE];
    
	while (!isDone) {
        socklen_t clientAddressLen = sizeof(cliaddr);

        // server receives packet from client and stores it in a buffer
        char buffer[MAXCOMMANDBUFLEN];
        memset(buffer, '\0', MAXCOMMANDBUFLEN);
        int bytesReceived = recvfrom(
            sockfd,
            (char *)buffer,
            MAXCOMMANDBUFLEN,
            MSG_WAITALL,
            (struct sockaddr*) &cliaddr,
            &clientAddressLen
        );
        buffer[bytesReceived] = '\0'; 
        
        // process the client's command and compute a response
        char* bufferP;
        strtol(buffer, &bufferP, MAXCOMMANDBUFLEN);
        Sorter_processCommand(bufferP, response_buffer);

        int response_in_bytes = strlen(response_buffer) * sizeof(char);

        // check if response is within the limit of each packet size
        // the max size for each packet is 1500 bytes
        if (response_in_bytes < MAX_PACKET_SIZE) {
            send_response_to_client(response_buffer, clientAddressLen);
        }
        else {
            char send_buffer[MAX_PACKET_SIZE];
            int i = 0;
            while (i < response_in_bytes) {
                memset(send_buffer, '\0', MAX_PACKET_SIZE);
                int j = i;
                while (j < response_in_bytes && response_buffer[j] != '\n') {
                    j++;
                }
                strncpy(send_buffer, response_buffer + i, j - i + 1);
                i = j + 1;
                send_response_to_client(send_buffer, clientAddressLen);
                
                struct timespec seconds, nanoseconds;
                memset(&seconds, 0, sizeof(struct timespec));
                memset(&nanoseconds, 0, sizeof(struct timespec));

                seconds.tv_sec = 0;
                nanoseconds.tv_nsec = 400000000;
                nanosleep(&seconds, &nanoseconds);
            }
        }
        
    }
    pthread_exit(NULL);
    return NULL;
}

void* stat_thread_method() {
    struct timespec tim, tim2;
    tim.tv_sec = 1;
    tim.tv_nsec = 1000;

    while (!isDone) {
        // every 1 second... display numSorted and set it back to zero    
        nanosleep(&tim, &tim2);

        pthread_mutex_lock(&sortingLock);

        if (numSorted == 0) {
            pthread_cond_wait(&sortingCond, &sortingLock);
        }

        numSortedDisplay = numSorted;
        numSorted = 0;
        pthread_mutex_unlock(&sortingLock);

        // display the new array length being sorted
        printf("Current Array Size: %d\n", currentSize);
    }
    pthread_exit(NULL);
    return NULL;
}

void* display_method() {

    while (!isDone) {
        if (numSortedDisplay > 99) {
            Seg_Display_displayNumber(i2cFileDesc, 99);
        }
        else {
            Seg_Display_displayNumber(i2cFileDesc, numSortedDisplay);
        }
    }
    pthread_exit(NULL);
    return NULL;
}

// Clean up function
void Sorter_Shutdown() {
    pthread_mutex_destroy(&sortingLock);
    pthread_mutex_destroy(&arrLenLock);
    pthread_cond_destroy(&sortingCond);
    pthread_cond_destroy(&arrLenCond);
    close(sockfd);
    close(i2cFileDesc);
    Seg_Display_write2File(LEFT_VALUE, "0");
    Seg_Display_write2File(RIGHT_VALUE, "0");
}

int main() {
    if (networkConfigged == 0) {
        if (Network_init() == 1) {
            printf("Error: couldn't setup networking!\n");
            return 1;
        }
        networkConfigged = 1;
    }

    i2cFileDesc = Seg_Display_configI2C();

    // setup the threads
    pthread_t sorter_thread, stat_thread, display_thread, udp_in_thread;

    // not sure if it's better to pass the length as an arg here, or do it in the method
    if (pthread_create(&sorter_thread, NULL, sorter_thread_method, NULL) != 0) {
		printf("unable to create sorter process\n");
	}
    if (pthread_create(&stat_thread, NULL, stat_thread_method, NULL) != 0) {
		printf("unable to create stat process\n");
	}
    if (pthread_create(&display_thread, NULL, display_method, NULL) != 0) {
		printf("unable to create display process\n");
	}
    if (pthread_create(&udp_in_thread, NULL, udp_in_method, NULL) != 0) {
		printf("unable to create udp in process\n");
	}

	int res = pthread_join(sorter_thread, NULL);
	if (res != 0) {
		printf("Couldn't join thread sorter_thread\n");
	}
    res = pthread_join(stat_thread, NULL);
	if (res != 0) {
		printf("Couldn't join thread stat_thread\n");
	}
    res = pthread_join(display_thread, NULL);
	if (res != 0) {
		printf("Couldn't join thread display_thread\n");
	}
    res = pthread_join(udp_in_thread, NULL);
	if (res != 0) {
		printf("Couldn't join thread udp_in_thread\n");
	}

    Sorter_Shutdown();
    printf("Done!\n");

    return 0;
}
