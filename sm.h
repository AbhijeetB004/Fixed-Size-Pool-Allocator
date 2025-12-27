#ifndef SM_H
#define SM_H

#include <stddef.h>

#define SM_ALLOC_ARRAY(type, size)      (type *)SM_alloc(size * sizeof(type))
#define SM_ALLOC(type)                  (type *)SM_alloc(sizeof(type))
#define SM_DEALLOC(ptr)                 SM_dealloc(ptr)

struct PoolData_t
{
    unsigned int blockSize;
    char *startAddress;
    char *endAddress;
    
    void *freeListHead; 

    unsigned int totalBlocks;
    unsigned int freeCount;
    unsigned int totalAllocations;
};

void initStorageManager(const unsigned int poolSize, int numPools, const unsigned int *pools);
void displayPoolInfo();
void destroyStorageManager();
void *SM_alloc(size_t size);
void SM_dealloc(void *ptr);

#endif