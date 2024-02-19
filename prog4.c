/*
Operating Systems (EECE.4811)
Section 201
Project 4
Jeffrey Nguyen
Rajdeep Mallick

Project 4 will implement a paged memory manager. The program will allocate an array to represent memory, then handle a series of requests from threads
representing different processes. Each “process” will have its own virtual memory space—and therefore its own page table—while your program will also track the list of
free frames and handle page replacement using the clock algorithm discussed in class
*/
#pragma warning(disable:4996)
#include "thread.h"
#include "memory.h"

int memorySize;
int pageSize;
int numProcesses;

MemoryManagementMonitor MemoryMonitor;
OutputMonitor outputMonitor;

int main(int argc, char* argv[]){
    if (argc == 4 || argc == 3) {
        // Initializing Mutex locks
        pthread_mutex_init(&MemoryMonitor.memoryLock, NULL);
        pthread_mutex_init(&MemoryMonitor.freeFramesLock, NULL);
        pthread_mutex_init(&MemoryMonitor.clockLock, NULL);

        FILE* inputFile = fopen(argv[1], "r");

        fscanf(inputFile, "%d", &memorySize);
        fscanf(inputFile, "%d", &pageSize);
        fscanf(inputFile, "%d", &numProcesses);

        //char configFiles[numProcesses][100]; // assuming the maximum length of a file name is 100
        char** configFiles;
        configFiles = malloc((numProcesses) * sizeof(char*));

        for (int i = 0; i < numProcesses; i++) {
            configFiles[i] = malloc((100) * sizeof(char));
        }

        for (int i = 0; i < numProcesses; i++) {
            fscanf(inputFile, "%s", configFiles[i]);
        }

        fclose(inputFile);

        // set up output file stream
        FILE* outputFile = fopen(argv[2], "w");
        outputMonitor.outputFile = outputFile;
        

        // filling main memory with values
        MemoryMonitor.memoryArray = (char*)malloc(memorySize * sizeof(char));
        if (argc == 4) {
            int seed = atoi(argv[3]);
            srand(seed);
        }
        else {
            srand(time(NULL));
        }
        for (int i = 0; i < memorySize; i++) {
            MemoryMonitor.memoryArray[i] = rand() % 256;
        }
        
        // initialize page table for all processes
        MemoryMonitor.pageTable = (struct PTE**)malloc(numProcesses * sizeof(struct PTE));


        // initialize free frame list and add all free frames
        MemoryMonitor.freeFrames = NULL;
        for(int i = (memorySize/pageSize) - 1; i >= 0; i--){
            AddFreeFrame(i);
        }
        
        pthread_t* threads;
        threads = (pthread_t*)malloc(numProcesses * sizeof(pthread_t));
    
        ThreadArgs* threadArgs;
        threadArgs = (ThreadArgs*)malloc(numProcesses * sizeof(ThreadArgs));

        // Create threads and pass in the index of the configuration file
        for (int i = 0; i < numProcesses; i++) {
            threadArgs[i].threadID = i;
            strcpy(threadArgs[i].filename, configFiles[i]);
            int rc = pthread_create(&threads[i], NULL, processThread, &threadArgs[i]);
            fprintf(outputMonitor.outputFile, "Process %d started\n", i);
            if (rc) {
                fprintf(stderr, "Error creating thread %d\n", i);
                return 1;
            }
        }

        // Join threads to wait for them to finish
        for (int i = 0; i < numProcesses; i++) {
            int rc = pthread_join(threads[i], NULL);
            if (rc) {
                fprintf(stderr, "Error joining thread %d\n", i);
                return 1;
            }
            else {
                fprintf(outputMonitor.outputFile, "Process %d complete\n", i);
            }
        }

        pthread_mutex_destroy(&MemoryMonitor.memoryLock);
        pthread_mutex_destroy(&MemoryMonitor.freeFramesLock);
        pthread_mutex_destroy(&MemoryMonitor.clockLock);
        
    }else if(argc < 4){
        // if there are not enough arguments
        fprintf(stderr, "Not enough arguments. Format: \n");
        return 1;
    }else{
        // if there are too many arguments
        fprintf(stderr, "Too many arguments. Format: \n");
        return 1;
    } 

    fprintf(outputMonitor.outputFile, "Main: program completed\n");
    return 1;
}