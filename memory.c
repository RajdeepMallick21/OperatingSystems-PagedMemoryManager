#include "memory.h"
#pragma warning(disable:4996)
#define HAVE_STRUCT_TIMESPEC
extern int pageSize;
extern int numProcesses;
extern int memorySize;
extern MemoryManagementMonitor MemoryMonitor;
extern OutputMonitor outputMonitor;

int Translate(int pid, int VA, int op, int virtualMemorySize){
    int physicalAddress;
    int frameNum = 0;
    int pageNum = VA/pageSize;
    int offsetNum = VA % pageSize;
    

    // Use VA to determine whether page is resident in memory
    // Check if page number has a frame number associated with it
    // Check if the page table entry is valid
    // Page fault: the page is not currently in physical memory
    // Invoke the page replacement algorithm to choose a victim page
    // Load the page from disk into a free frame in physical memory
    // Update the page table entry with the new frame number
    // Set the "valid" bit to 1
    if (!(MemoryMonitor.pageTable[pid][pageNum].valid)) {
        if (MemoryMonitor.freeFrames != NULL) {
            // FREE FRAMES MUTEX LOCK
            pthread_mutex_lock(&MemoryMonitor.freeFramesLock);

            fprintf(outputMonitor.outputFile, "P%d: page %d not resident in memory\n", pid, pageNum);
            frameNum = GetFreeFrame();            
            MemoryMonitor.pageTable[pid][pageNum].pageFrameNum = frameNum;
            fprintf(outputMonitor.outputFile, "P%d: using free frame %d\n", pid, frameNum);
            fprintf(outputMonitor.outputFile, "P%d: new translation from page %d to frame %d\n", pid, pageNum, frameNum);
            fflush(outputMonitor.outputFile);
            void RemoveFreeFrame();
            MemoryMonitor.pageTable[pid][pageNum].valid = 1;
   
            // CLOCK MUTEX LOCK
            pthread_mutex_lock(&MemoryMonitor.clockLock);
            // Add page to Page Replacement array
            AddClockNode(pageNum, pid, &MemoryMonitor.pageTable[pid][pageNum]);

            pthread_mutex_unlock(&MemoryMonitor.clockLock);

            pthread_mutex_unlock(&MemoryMonitor.freeFramesLock);

            
        }
        else {
            // CLOCK MUTEX LOCK
            pthread_mutex_lock(&MemoryMonitor.clockLock);
            //// Call Page Replacement algorithm to evict page from physical memory
            fprintf(outputMonitor.outputFile, "P%d: page %d not resident in memory\n", pid, pageNum);
            EvictClockNode(pid);
            pthread_mutex_unlock(&MemoryMonitor.clockLock);

            // FREE FRAMES MUTEX LOCK
            pthread_mutex_lock(&MemoryMonitor.freeFramesLock);
            frameNum = GetFreeFrame();

            MemoryMonitor.pageTable[pid][pageNum].pageFrameNum = frameNum;
            fprintf(outputMonitor.outputFile, "P%d: new translation from page %d to frame %d\n", pid, pageNum, frameNum);
            fflush(outputMonitor.outputFile);
            MemoryMonitor.pageTable[pid][pageNum].valid = 1;

            pthread_mutex_unlock(&MemoryMonitor.freeFramesLock);
        }
    }
    else {
        frameNum = MemoryMonitor.pageTable[pid][pageNum].pageFrameNum;
        fprintf(outputMonitor.outputFile, "P%d: valid translation from page %d to frame %d\n", pid, pageNum, frameNum);
        MemoryMonitor.pageTable[pid][pageNum].referenced = 1;
    }

    physicalAddress = (frameNum * pageSize) + offsetNum;
    

    return physicalAddress;
}
// Add new frame to the front of the freeFrames linked list
void AddFreeFrame(int frameNum){
    struct FreeFrameNode *newFrame = (struct FreeFrameNode*) malloc(sizeof(struct FreeFrameNode));
    newFrame->frameNum = frameNum;
    newFrame->next = MemoryMonitor.freeFrames;
    MemoryMonitor.freeFrames = newFrame;
}

int GetFreeFrame() {
    int frame_num = -1; // assume no free frame is found
    if (MemoryMonitor.freeFrames != NULL) {
        // there is at least one free frame
        frame_num = MemoryMonitor.freeFrames->frameNum;
        // remove the first node from the linked list
        FreeFrameNode* temp = MemoryMonitor.freeFrames;
        MemoryMonitor.freeFrames = MemoryMonitor.freeFrames->next;
        free(temp);
    }
    return frame_num;
}


void AddClockNode(int pageNum, int pid, struct PTE* pte) {
    struct PageReplacementNode* newClockNode = (struct PageReplacementNode*)malloc(sizeof(struct PageReplacementNode));
    newClockNode->pageNum = pageNum;
    newClockNode->pid = pid;
    newClockNode->pte = pte;
    newClockNode->next = NULL;

    if (MemoryMonitor.clock == NULL) {
        // The list is empty, so the new node becomes the head
        MemoryMonitor.clock = newClockNode;
    }
    else {
        // Traverse the list until we reach the last node
        struct PageReplacementNode* lastNode = MemoryMonitor.clock;
        while (lastNode->next != NULL) {
            lastNode = lastNode->next;
        }

        // Add the new node after the last node
        lastNode->next = newClockNode;
    }
}

void EvictClockNode(int pid) {
    PageReplacementNode* current = MemoryMonitor.clock;
    PageReplacementNode* candidate = NULL;

    while (current != NULL) {
        if (current->pte->referenced == 0) {
            candidate = current;
            break;
        }
        current->pte->referenced = 0;
        current = current->next;
    }

    // remove the node from the list
    if (candidate == MemoryMonitor.clock) {
        MemoryMonitor.clock = MemoryMonitor.clock->next;
    }
    else {
        PageReplacementNode* prev = MemoryMonitor.clock;
        while (prev->next != candidate) {
            prev = prev->next;
        }
        prev->next = candidate->next;
    }
    fprintf(outputMonitor.outputFile, "P%d: evicting process %d, page %d from frame %d\n", pid, candidate->pid, candidate->pageNum, candidate->pte->pageFrameNum);
    // get the page table entry associated with the candidate node
    struct PTE* pte = candidate->pte;

    AddFreeFrame(candidate->pte->pageFrameNum);

    // free the memory used by the node
    free(candidate);

    // if the candidate page is not NULL, invalidate it
    if (pte != NULL) {
        pte->valid = 0;
    }

}

// Perform a memory access op (read or write)
int* Memory_Access(int pid, int VA, int PA, int op) {
    int pageNum = VA / pageSize;
    // Perform the read or write op
    if (op == 'R') {
        int* result = (int*)malloc(sizeof(int));

        // Read the bytes in reverse order
        int byte0 = MemoryMonitor.memoryArray[PA + 3] & 0xFF;
        int byte1 = MemoryMonitor.memoryArray[PA + 2] & 0xFF;
        int byte2 = MemoryMonitor.memoryArray[PA + 1] & 0xFF;
        int byte3 = MemoryMonitor.memoryArray[PA] & 0xFF;

        // Combine the bytes to form the integer
        *result = (byte0 << 24) | (byte1 << 16) | (byte2 << 8) | byte3;

        // Return the pointer to the allocated memory
        return result;

    }
    else if (op == 'W') {
        // Write the value to physical memory
        MemoryMonitor.memoryArray[PA + 3] = rand() % 256;
        MemoryMonitor.memoryArray[PA + 2] = rand() % 256;
        MemoryMonitor.memoryArray[PA + 1] = rand() % 256;
        MemoryMonitor.memoryArray[PA] = rand() % 256;
        // Set the "dirty" bit to 1 to indicate that the page has been modified
        MemoryMonitor.pageTable[pid][pageNum].dirty = 1;
    }

    return 0;  // Success
}
