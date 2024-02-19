#include "thread.h"
#pragma warning(disable:4996)
#define HAVE_STRUCT_TIMESPEC
extern int pageSize;
extern int numProcesses;
extern int memorySize;
extern MemoryManagementMonitor MemoryMonitor;
extern OutputMonitor outputMonitor;

void *processThread(void *arg){
   
    ThreadArgs *args = (ThreadArgs*) arg;
    int pid = args->threadID;

    FILE *threadFile = fopen(args->filename, "r");

    // Initialize the registers for this thread
    int registers[32];

    // Read the total virtual memory size for this thread
    int totalVirtualMemorySize;
    fscanf(threadFile, "%d\n", &totalVirtualMemorySize);

    // Initialize the page table for this thread
    //struct PTE *pageTable[totalVirtualMemorySize/pageSize];
    struct PTE* pageTable;
    pageTable = (struct PTE*)malloc((totalVirtualMemorySize / pageSize) * sizeof(struct PTE));
    for (int i = 0; i < (totalVirtualMemorySize/pageSize); i++) {
        pageTable[i].dirty = 0;
        pageTable[i].valid = 0;
        pageTable[i].referenced = 0;
    }
    
    MemoryMonitor.pageTable[pid] = (struct PTE*)malloc((totalVirtualMemorySize / pageSize) * sizeof(struct PTE));
    // sets pageTable for each process
    MemoryMonitor.pageTable[pid] = pageTable;
    // Read each memory access from the input file
    char rw;
    int regNum, virtualAddress, physicalAddress, data;
    
    while (fscanf(threadFile, "%c r%d %d\n", &rw, &regNum, &virtualAddress) == 3) {
        
        fprintf(outputMonitor.outputFile, "P%d OPERATION: %c r%d 0x%08x\n", pid, rw, regNum, virtualAddress);
        fflush(outputMonitor.outputFile);
        
        // FREEFRAME LOCK AND CLOCK LOCK INSIDE THIS FUNCTION
        // Translate the virtual address to a physical address
        // Check if the page is resident in memory
        physicalAddress = Translate(pid, virtualAddress, rw, totalVirtualMemorySize);
        
        // Print the translation information
        fprintf(outputMonitor.outputFile, "P%d: translated VA 0x%08x to PA 0x%08x\n", pid, virtualAddress, physicalAddress);
        fflush(outputMonitor.outputFile);
        

        // MEMORY LOCK INSIDE THIS FUNCITON
        int* result = Memory_Access(pid, virtualAddress, physicalAddress, rw);
        registers[regNum] = *result;
        fprintf(outputMonitor.outputFile, "P%d: r%d = 0x%08x (mem at virtual addr 0x%08x)\n", pid, regNum, registers[regNum], virtualAddress);
            
        
    }
    
    
}