#include <unistd.h>
#include <cstring>
#include <sys/mman.h>
#include <cstdint>
#include <iostream>

#define MAX_ORDER 10

struct MallocMetadata {
    size_t size; // size of the allocated data (not including MallocMetadata)
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;
};


MallocMetadata* metaByOrderArr[11] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

MallocMetadata* metaByOrderAllocatedArr[10] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

MallocMetadata* bigBlocksListFirst = NULL;

static void* heap_start = NULL;

static bool is_first_alloc = true;



// =========================== TESTING - DELETE AFTER FINISH =====================================

int how_many_free_in_order(int order) {
    int count = 0;
    MallocMetadata* currect = metaByOrderArr[order];

    while (currect != NULL) {
        count++;
        currect = currect->next;
    }
    return count;
}

int how_many_alloc_in_order(int order) {
    int count = 0;
    MallocMetadata* currect = metaByOrderAllocatedArr[order];

    while (currect != NULL) {
        count++;
        currect = currect->next;
    }
    return count;
}

void print_meta_by_order_array() {
    std::cout << "=== metaByOrderArr Contents ===" << std::endl;
    for (int i = 0; i <= 10; ++i) {
        std::cout << "Order " << i << ": ";
        MallocMetadata* current = metaByOrderArr[i];
        if (!current) {
            std::cout << "(empty)";
        } else {
            while (current) {
                std::cout << "[size: " << current->size
                          << ", is_free: " << (current->is_free ? "true" : "false") << "] -> ";
                current = current->next;
            }
            std::cout << "NULL";
        }
        std::cout << std::endl;
    }
    std::cout << "===============================" << std::endl;
}

void print_meta_by_order_allocated_array() {
    std::cout << "=== metaByOrderAllocatedArr Contents ===" << std::endl;
    for (int i = 0; i <= 9; ++i) {
        std::cout << "Order " << i << ": ";
        MallocMetadata* current = metaByOrderAllocatedArr[i];
        if (!current) {
            std::cout << "(empty)";
        } else {
            while (current) {
                std::cout << "[size: " << current->size
                          << ", is_free: " << (current->is_free ? "true" : "false") << "] -> ";
                current = current->next;
            }
            std::cout << "NULL";
        }
        std::cout << std::endl;
    }
    std::cout << "===============================" << std::endl;
}

// =========================== TESTING - DELETE AFTER FINISH =====================================


void* alignHeapTo4MB() {
    void* curr_brk = sbrk(0);
    size_t misalignment = (size_t)curr_brk % (128 * 1024 * 32);
    if (misalignment != 0) {
        size_t adjustment = 128*1024*32 - misalignment;
        void* result = sbrk(adjustment);
        if (result == (void*)(-1)) {
            return NULL;
        }
    }

    heap_start = sbrk(0); // Store the aligned heap start
    return heap_start;
}


bool isFirstAlloc() {
    if(is_first_alloc) {
        is_first_alloc = false;
        return true;
    }
    return false;
}

// If the function returns 11 the size is bigger than 128KB
int getDesiredOrderBySize(size_t size) {
    int order = 0;
    int blockSize = 128;
    while (order < 11 && size > blockSize - sizeof(MallocMetadata)) {
        blockSize *= 2;
        order++;
    }
    return order;
}

MallocMetadata* getSmallestAddressBlockByOrder(int order) {
    MallocMetadata* current = metaByOrderArr[order];
    MallocMetadata* smallestAddr = NULL;
    while (current != NULL) {
        if (current->is_free){
            if(smallestAddr == NULL || current < smallestAddr) {
                smallestAddr = current;
            }
        }
        current = current->next;
    }
    return smallestAddr;
}

int getPowerOfTwo(int power) {
    int result = 1;
    while (power != 0) {
        result *= 2;
        power--;
    }
    return result;
}

void removePtrFromOrderList(MallocMetadata* ptr, int order) {
    if (ptr->next != NULL) {
        ptr->next->prev = ptr->prev;
    }
    if (ptr->prev != NULL) {
        ptr->prev->next = ptr->next;
    } else {
        metaByOrderArr[order] = ptr->next;
    }
    ptr->next = NULL;
    ptr->prev = NULL;
}

void addMetaToOrderList(MallocMetadata* meta, int order) {
    meta->prev = NULL;
    if (metaByOrderArr[order] == NULL) {
        meta->next = NULL;
    } else {
        meta->next = metaByOrderArr[order];
        metaByOrderArr[order]->prev = meta;
    }
    metaByOrderArr[order] = meta;
}

void removePtrFromOrderAllocatedList(MallocMetadata* ptr, int order) {
    if (ptr->next != NULL) {
        ptr->next->prev = ptr->prev;
    }
    if (ptr->prev != NULL) {
        ptr->prev->next = ptr->next;
    } else {
        metaByOrderAllocatedArr[order] = ptr->next;
    }
    ptr->next = NULL;
    ptr->prev = NULL;
}

void addMetaToOrderAllocatedList(MallocMetadata* meta, int order) {
    meta->prev = NULL;
    if (metaByOrderAllocatedArr[order] == NULL) {
        meta->next = NULL;
    } else {
        meta->next = metaByOrderAllocatedArr[order];
        metaByOrderAllocatedArr[order]->prev = meta;
    }
    metaByOrderAllocatedArr[order] = meta;
}

void* allocateBigBlock(size_t size) {
    void* block = mmap(NULL, size + sizeof(MallocMetadata),
                       PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS,
                       -1, 0);
    if(block == MAP_FAILED)
        return NULL;

    MallocMetadata* newMetaData = (MallocMetadata*)block;
    void* ptrToAlloc = NULL;
    ptrToAlloc = (void*)((char*)block + sizeof(MallocMetadata));
    MallocMetadata* current = bigBlocksListFirst;

    newMetaData->next = NULL;
    newMetaData->is_free = false;
    newMetaData->size = size;
    if (bigBlocksListFirst == NULL) {
        newMetaData->prev = NULL;
        bigBlocksListFirst = newMetaData;
    } else {
        current = bigBlocksListFirst;
        while (current->next != NULL) {
            current = current->next;
        }
        newMetaData->prev = current;
        current->next = newMetaData;
    }
    return ptrToAlloc;
}

void removeFromBigBlockList(MallocMetadata* ptr) {
    if (ptr->next != NULL) {
        ptr->next->prev = ptr->prev;
    }
    if (ptr->prev != NULL) {
        ptr->prev->next = ptr->next;
    } else {
        bigBlocksListFirst = ptr->next;
    }
    ptr->next = NULL;
    ptr->prev = NULL;
}

void* smalloc(size_t size) {
    //std::cout << "smalloc called with size: " << size << std::endl;
    if (size == 0 || size > 100000000) { //TODO CHECK IF STILL NEEDED
        return NULL;
    }
    void* ptrToAlloc = NULL;
    if (isFirstAlloc()) { //Alloc the first 32 128KB blocks if needed
        if (alignHeapTo4MB() == NULL) {
            return NULL; // didn't manage to alignHeap
        }

        intptr_t sizeIntPtr = 128*1024*32;
        void* pointerToCurrentBlock = sbrk(sizeIntPtr);
        if (pointerToCurrentBlock == (void*)(-1)) {
            return NULL;
        }
        MallocMetadata* currentMeta;
        for (int i = 0; i < 32; i++) {
            currentMeta = (MallocMetadata*)pointerToCurrentBlock;
            currentMeta->size = 128*1024 - sizeof(MallocMetadata);
            currentMeta->is_free = true;
            addMetaToOrderList(currentMeta, 10);
            pointerToCurrentBlock = (void*)((char*)pointerToCurrentBlock + 128*1024);
        }
    }

    //Alloc the desired size
    //std::cout << "[DEBUG] : Not First Alloc" << std::endl;
    int order = getDesiredOrderBySize(size);
    if (order > 10) { //If size bigger than 128KB
        //std::cout << "[DEBUG] : Big Block" << std::endl;
        ptrToAlloc = allocateBigBlock(size);
        if(ptrToAlloc == NULL)
            return NULL;
    } else {
        //std::cout << "[DEBUG] : Not Big Block" << std::endl;
        int blockOrder = order;
        MallocMetadata* blockForAlloc = NULL;

        // Search for available block, with upper limit to prevent infinite loop
        while (blockForAlloc == NULL && blockOrder <= 10) { //Find the smallest fitting block with the smallest address
            //std::cout << "searching smallest fitting block" << std::endl;
            if (metaByOrderArr[blockOrder] == NULL) {
                blockOrder++;
            } else {
                bool freeBlockExists = false;
                MallocMetadata* current = metaByOrderArr[blockOrder];
                while (current != NULL) {
                    //std::cout << "searching free block" << std::endl;
                    if (current->is_free == true) {
                        freeBlockExists = true;
                        break;
                    }
                    current = current->next;
                }
                if (freeBlockExists) {
                    blockForAlloc = getSmallestAddressBlockByOrder(blockOrder);
                    //std::cout << "found free block" << std::endl;
                } else {
                    blockOrder++;
                }
            }
        }

        // If no block found, return NULL (no more memory available)
        if (blockForAlloc == NULL) {
            return NULL;
        }

        removePtrFromOrderList(blockForAlloc, blockOrder);
        //std::cout << "removed block from free list" << std::endl;

        while (blockOrder != order) { //Split the block until it's in the right size
            //std::cout << "splitting blocks " << blockOrder << std::endl;
            blockOrder--;
            MallocMetadata* buddy = (MallocMetadata*)((char*)blockForAlloc + 128*getPowerOfTwo(blockOrder));

            // Validate buddy pointer before using it
            if (buddy == NULL) {
                return NULL;
            }

            buddy->size = 128*getPowerOfTwo(blockOrder) - sizeof(MallocMetadata);
            buddy->is_free = true;
            //std::cout << "adding to order list " << blockOrder << std::endl;
            addMetaToOrderList(buddy, blockOrder);
            //std::cout << "added to order list " << blockOrder << std::endl;
        }
        //std::cout << "[DEBUG] : splitted blocks" << std::endl;
        blockForAlloc->size = 128*getPowerOfTwo(blockOrder) - sizeof(MallocMetadata);
        blockForAlloc->is_free = false;
        //now add to allocated List
        addMetaToOrderAllocatedList(blockForAlloc, blockOrder);
        ptrToAlloc = (void*)((char*)blockForAlloc + sizeof(MallocMetadata));
    }
    //std::cout << "returning" << std::endl;
    return ptrToAlloc;
}

void* scalloc(size_t num, size_t size) {
    size_t sizeToAlloc = num * size;
    void* data = smalloc(sizeToAlloc); //will find if size or num = 0, as size * num = 0
    if(data == NULL)
        return NULL;
    memset(data, 0, sizeToAlloc);
    return data;
}

MallocMetadata* mergeBuddies(MallocMetadata* metadata) {
    while(true) {
        size_t curr_size = metadata->size;
        int order = getDesiredOrderBySize(curr_size);

        if(order >= MAX_ORDER)
            break;

        size_t block_size = 128 * getPowerOfTwo(order);

        uintptr_t current_addr = (uintptr_t)metadata;
        uintptr_t buddy_addr = current_addr ^ block_size;


        MallocMetadata* buddy_metadata = (MallocMetadata*)buddy_addr;

        // Check if buddy is valid and free
        if(!buddy_metadata->is_free || buddy_metadata->size != curr_size)
            break;

        // Remove both blocks from their current order lists
        removePtrFromOrderList(buddy_metadata, order);
        removePtrFromOrderList(metadata, order);

        // Determine which block comes first in memory
        MallocMetadata* mergedBuddiesMetadata;
        if(buddy_addr < current_addr)
            mergedBuddiesMetadata = buddy_metadata;
        else
            mergedBuddiesMetadata = metadata;

        // Correct size calculation: new size is exactly double the current size
        mergedBuddiesMetadata->size = curr_size * 2 + sizeof(MallocMetadata);
        mergedBuddiesMetadata->is_free = true;

        metadata = mergedBuddiesMetadata;
    }

    int newOrder = getDesiredOrderBySize(metadata->size);
    addMetaToOrderList(metadata, newOrder);
    return metadata;
}

void sfree(void* p) {
    if(p == NULL)
        return;

    MallocMetadata* metadata = (MallocMetadata*)((char*)p - sizeof(MallocMetadata)); //p's metadata
    if(metadata->is_free) //already free
        return;

    int order = getDesiredOrderBySize(metadata->size);

    if (order > 10) { // Changed from >= 10 to > 10
        size_t total_size = metadata->size + sizeof(MallocMetadata);
        removeFromBigBlockList(metadata);
        munmap((void*)metadata, total_size);
        return;
    }

    metadata->is_free = true; //now p space is free
    removePtrFromOrderAllocatedList(metadata, order);

    mergeBuddies(metadata);
}

void* srealloc(void* oldp, size_t size) {
    if (size == 0 || size > 100000000) {
        return NULL;
    }
    if (oldp == NULL) {
        return smalloc(size);
    }
    MallocMetadata* oldMeta = (MallocMetadata*)((char*)oldp - sizeof(MallocMetadata));
    if (size <= oldMeta->size) {
        return oldp;
    }
    void* newBlock = smalloc(size);
    if (newBlock == NULL) {
        return NULL;
    }
    memmove(newBlock, oldp, oldMeta->size);
    sfree(oldp);
    return newBlock;
}

size_t _num_free_blocks() {
    size_t countFreeBlocks = 0;
    for (int i = 0; i <= 10; i++) {
        MallocMetadata *current = metaByOrderArr[i];
        while (current != NULL) {
            if (current->is_free) {
                countFreeBlocks++;
            }
            current = current->next;
        }
    }
    return countFreeBlocks;
}

size_t _num_free_bytes() {
    size_t numOfFreeBytes = 0;
    for (int i = 0; i <= 10; i++) {
        MallocMetadata* current = metaByOrderArr[i];
        while (current != NULL) {
            if (current->is_free) {
                numOfFreeBytes += current->size;
            }
            current = current->next;
        }
    }
    return numOfFreeBytes;
}

size_t _num_allocated_blocks() {
    size_t countOverAllBlocks = 0;
    //std::cout << "[DEBUG] : not null" << std::endl;
    //print_meta_by_order_array();
    // print_meta_by_order_allocated_array();
    for (int i = 0; i <= 10; i++) {
        //std::cout << i << std::endl;
        MallocMetadata* current = metaByOrderArr[i];
        while (current != NULL) {
            countOverAllBlocks++;
            current = current->next;
        }
    }
    for (int i = 0; i <= 9; i++) {
        //std::cout << i << std::endl;
        MallocMetadata* current = metaByOrderAllocatedArr[i];
        while (current != NULL) {
            countOverAllBlocks++;
            current = current->next;
        }
    }
    //std::cout << "got here" << std::endl;

    MallocMetadata* current = bigBlocksListFirst;
    //std::cout << "[DEBUG] : current is " << (current == nullptr) << std::endl;
    while (current != NULL) {
        countOverAllBlocks++;
        current = current->next;
    }
    return countOverAllBlocks;
}

size_t _num_allocated_bytes() {
    size_t numOfBytes = 0;
    for (int i = 0; i <= 10; i++) {
        MallocMetadata* current = metaByOrderArr[i];
        while (current != NULL) {
            numOfBytes += current->size;
            current = current->next;
        }
    }
    for (int i = 0; i <= 9; i++) {
        //std::cout << i << std::endl;
        MallocMetadata* current = metaByOrderAllocatedArr[i];
        while (current != NULL) {
            numOfBytes += current->size;
            current = current->next;
        }
    }
    MallocMetadata* current = bigBlocksListFirst;
    while (current != NULL) {
        numOfBytes += current->size;
        current = current->next;
    }
    return numOfBytes;
}

size_t _num_meta_data_bytes() { //TODO CHECK IF NEED TO CHANGE
    size_t numOfBlocksOnHeap = _num_allocated_blocks();
    return numOfBlocksOnHeap * sizeof(MallocMetadata);
}

size_t _size_meta_data() {
    return sizeof(MallocMetadata);
}