#ifndef MEMORY_H_
#define MEMORY_H_
#define HAVE_STRUCT_TIMESPEC
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <math.h>

typedef struct PTE{
    int valid;
    int dirty;
    int referenced;
    int pageFrameNum;
}PTE;

typedef struct FreeFrameNode{
    int frameNum;
    struct FreeFrameNode *next;
}FreeFrameNode;

typedef struct PageReplacementNode{
    int pid;
    int pageNum;
    struct PTE *pte;
    struct PageReplacementNode *next;
}PageReplacementNode;

typedef struct MemoryManagementMonitor{
    // Global memory management data structures
    char *memoryArray;
    struct PTE **pageTable;
    struct FreeFrameNode *freeFrames;
    struct PageReplacementNode *clock;

    // Locks for synchronization
    pthread_mutex_t memoryLock;
    pthread_mutex_t freeFramesLock;
    pthread_mutex_t clockLock;
}MemoryManagementMonitor;

typedef struct OutputMonitor{
    FILE *outputFile;
    pthread_mutex_t outputLock;
} OutputMonitor;

/* Given an operation (read/write), process number, and virtual address, perform the operation and return the result (or store it through a pointer) */
int* Memory_Access(int pid, int VA, int PA, int op);

/* Given a process number and virtual address, return the corresponding physical address */
int Translate(int pid, int VA, int op, int virtualMemorySize);

/* Return the number of the first available free frame */
int GetFreeFrame();

/* Add a frame to the free list */
void AddFreeFrame(int frameNum);

/* Remove a frame from the free list*/
//void RemoveFreeFrame();

/* Every time there’s a new page “brought in to memory,” it needs to be added as a candidate for eviction */
void AddClockNode(int pageNum, int pid, struct PTE* pte);

/* Choose a candidate for eviction and remove that node from the list */
void EvictClockNode();


#endif