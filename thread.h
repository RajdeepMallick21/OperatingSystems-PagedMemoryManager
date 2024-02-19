#ifndef THREAD_H_
#define THREAD_H_
#define HAVE_STRUCT_TIMESPEC
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <string.h>
#include <math.h>
#include "memory.h"

typedef struct{
    int threadID;
    char filename[100];
}ThreadArgs;

/* This function mostly sets up everything that needs to be done on a perthread basis, then reads every memory access from the input file and calls
a function that does most of the work */
void *processThread(void *arg);

#endif