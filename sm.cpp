#include "sm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_POOLS 32
#define MAX_BLOCK_SIZE 256

unsigned char m_SizeToPoolLookup[MAX_BLOCK_SIZE + 1];

PoolData_t m_Pools[MAX_POOLS];
int m_NumPools = 0;

int comparePools(const void *a, const void *b) {
    return (*(unsigned int*)a - *(unsigned int*)b);
}

void initStorageManager(const unsigned int initialPoolSize, int numPools, const unsigned int *pools)
{
    if (numPools > MAX_POOLS) {
        printf("Error: Too many pools requested. Max is %d\n", MAX_POOLS);
        abort();
    }

    unsigned int *sortedPools = (unsigned int *)malloc(numPools * sizeof(unsigned int));
    memcpy(sortedPools, pools, numPools * sizeof(unsigned int));
    qsort(sortedPools, numPools, sizeof(unsigned int), comparePools);

    m_NumPools = numPools;

    printf("StorageManager:: Initial Pools- ");
    for (int i = 0; i < numPools; i++)
    {
        unsigned int blockSize = sortedPools[i];
        if (blockSize > MAX_BLOCK_SIZE) {
            printf("\nError: Pool Size %u exceeds MAX_BLOCK_SIZE (%d)\n", blockSize, MAX_BLOCK_SIZE);
            abort();
        }
        
        printf("%d ", blockSize);

        size_t poolTotalBytes = (size_t)blockSize * initialPoolSize; 
        char *poolMem = (char *)malloc(poolTotalBytes);
        if (!poolMem) {
            printf("Memory allocation failed for pool %u\n", blockSize);
            abort();
        }

        m_Pools[i].blockSize = blockSize;
        m_Pools[i].startAddress = poolMem;
        m_Pools[i].endAddress = poolMem + poolTotalBytes;
        m_Pools[i].totalBlocks = poolTotalBytes / blockSize;
        m_Pools[i].freeCount = m_Pools[i].totalBlocks;
        m_Pools[i].totalAllocations = 0;

        char *current = poolMem;
        char *end = poolMem + poolTotalBytes;
        
        unsigned int totalBlocks = m_Pools[i].totalBlocks;
        for (unsigned int b = 0; b < totalBlocks - 1; ++b) {
            char *next = current + blockSize;
            *(void **)current = (void *)next;
            current = next;
        }
        *(void **)current = nullptr;

        m_Pools[i].freeListHead = (void *)poolMem;
    }

    int poolIdx = 0;
    for (int s = 0; s <= MAX_BLOCK_SIZE; ++s) {
        while (poolIdx < m_NumPools && m_Pools[poolIdx].blockSize < s) {
            poolIdx++;
        }
        
        if (poolIdx < m_NumPools) {
            m_SizeToPoolLookup[s] = (unsigned char)poolIdx;
        } else {
            m_SizeToPoolLookup[s] = 255; // Invalid marker
        }
    }

    free(sortedPools);

    printf("\nStorageManager:: Pool init complete\n");
}

void destroyStorageManager()
{
    for (int i = 0; i < m_NumPools; i++) {
        if (m_Pools[i].startAddress) {
            free(m_Pools[i].startAddress);
            m_Pools[i].startAddress = nullptr;
        }
    }
}

void * SM_alloc(size_t size)
{
    if (size > MAX_BLOCK_SIZE) return nullptr; 
    
    unsigned char poolIdx = m_SizeToPoolLookup[size];
    if (poolIdx == 255) return nullptr;

    PoolData_t *pool = &m_Pools[poolIdx];

    void *ptr = pool->freeListHead;
    if (ptr == nullptr) {
        printf("ERROR: Pool %u exhausted!\n", pool->blockSize);
        abort(); 
    }

    pool->freeListHead = *(void **)ptr;
    
    pool->freeCount--;
    pool->totalAllocations++;

    return ptr;
}

void SM_dealloc(void *ptr)
{
    if (ptr == nullptr) return;

    PoolData_t *pool = nullptr;
    for (int i = 0; i < m_NumPools; ++i) {
        if (ptr >= m_Pools[i].startAddress && ptr < m_Pools[i].endAddress) {
            pool = &m_Pools[i];
            break;
        }
    }

    if (pool) {
        // O(1) Push to Free List
        *(void **)ptr = pool->freeListHead;
        pool->freeListHead = ptr;
        
        pool->freeCount++;
    } else {
        // Pointer does not belong to any pool?
        // This is a bug in caller or heap pointer passed to SM_dealloc
        printf("ERROR: SM_dealloc called on unknown pointer %p\n", ptr);
        abort();
    }
}

void displayPoolInfo()
{
    printf("\n\n");
    for (int i = 0; i < m_NumPools; ++i)
    {
        PoolData_t *pool = &m_Pools[i];
        printf("Pool %u\n", pool->blockSize);
        printf("  totalAllocationsFromThisPool       : %u\n", pool->totalAllocations);
        printf("  totalBlocks                        : %u\n", pool->totalBlocks);
        printf("  freeBlocks                         : %u\n", pool->freeCount);
        printf("\n");
    }
    printf("\n** Total Pools: %d **\n", m_NumPools);
}
