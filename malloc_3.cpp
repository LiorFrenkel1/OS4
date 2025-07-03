#include <unistd.h>
#include <cstring>

struct MallocMetadata {
    size_t size; // size of the allocated data (not including MallocMetadata)
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;
};


MallocMetadata* metaByOrderArr[11] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};

bool isFirstAlloc() {
    for (int i = 0; i <= 10; i++) {
        if (metaByOrderArr[i] != NULL) {
            return false;
        }
    }
    return true;
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
        if (current->is_free && (smallestAddr == NULL ||current < smallestAddr)) {
            smallestAddr = current;
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

void* smalloc(size_t size) {
    if (size == 0 || size > 100000000) { //TODO CHECK IF STILL NEEDED
        return NULL;
    }
    void* ptrToAlloc = NULL;
    if (isFirstAlloc()) { //Alloc the first 32 128KB blocks if needed
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
    int order = getDesiredOrderBySize(size);
    if (order > 10) { //If size bigger than 128KB
        //TODO large allocs
    } else {
        int blockOrder = order;
        MallocMetadata* blockForAlloc = NULL;
        while (blockForAlloc == NULL) { //Find the smallest fitting block with the smallest address
            if (metaByOrderArr[blockOrder] == NULL) {
                blockOrder++;
            } else {
                bool freeBlockExists = false;
                MallocMetadata* current = metaByOrderArr[blockOrder];
                while (current != NULL) {
                    if (current->is_free == true) {
                        freeBlockExists = true;
                        break;
                    }
                    current = current->next;
                }
                if (freeBlockExists) {
                    blockForAlloc = getSmallestAddressBlockByOrder(order);
                } else {
                    blockOrder++;
                }
            }
        }

        removePtrFromOrderList(blockForAlloc, blockOrder);
        while (blockOrder != order) { //Split the block until it's in the right size
            blockOrder--;
            MallocMetadata* buddy = (MallocMetadata*)((char*)blockForAlloc + 128*getPowerOfTwo(blockOrder));
            buddy->size = 128*getPowerOfTwo(blockOrder) - sizeof(MallocMetadata);
            buddy->is_free = true;
            addMetaToOrderList(buddy, blockOrder);
        }
        blockForAlloc->size = 128*getPowerOfTwo(blockOrder) - sizeof(MallocMetadata);
        blockForAlloc->is_free = false;
        addMetaToOrderList(blockForAlloc, blockOrder);
        ptrToAlloc = (void*)((char*)blockForAlloc + sizeof(MallocMetadata));
    }
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

void sfree(void* p) {
    if(p == NULL)
        return;
    MallocMetadata* metadata = (MallocMetadata*)((char*)p - sizeof(MallocMetadata)); //p's metadata
    if(metadata->is_free) //already free
        return;
    MallocMetadata* p_next = metadata->next;
    MallocMetadata* p_prev = metadata->prev;

    metadata->is_free = true; //now p space is free
//    if (metadata->prev) //disconnect p from linked list
//        metadata->prev->next = metadata->next;
//    if (metadata->next)
//        metadata->next->prev = metadata->prev;
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
        NULL;
    }
    memmove(newBlock, oldp, oldMeta->size);
    sfree((void*)oldMeta);
    return newBlock;
}

size_t _num_free_blocks() {
    MallocMetadata* current = firstMeta;
    size_t countFreeBlocks = 0;
    while(current != NULL) {
        if(current->is_free) {
            countFreeBlocks++;
        }
        current = current->next;
    }
    return countFreeBlocks;
}

size_t _num_free_bytes() {
    MallocMetadata* current = firstMeta;
    size_t numOfFreeBytes = 0;
    while (current != NULL) {
        if (current->is_free) {
            numOfFreeBytes += current->size;
        }
        current = current->next;
    }
    return numOfFreeBytes;
}

size_t _num_allocated_blocks() {
    MallocMetadata* current = firstMeta;
    size_t countOverAllBlocks = 0;
    while(current != NULL) {
        countOverAllBlocks++;
        current = current->next;
    }
    return countOverAllBlocks;
}

size_t _num_allocated_bytes() {
    MallocMetadata* current = firstMeta;
    size_t numOfBytes = 0;
    while (current != NULL) {
        numOfBytes += current->size;
        current = current->next;
    }
    return numOfBytes;
}

size_t _num_meta_data_bytes() {
    size_t numOfBlocksOnHeap = _num_allocated_blocks();
    return numOfBlocksOnHeap * sizeof(MallocMetadata);
}

size_t _size_meta_data() {
    return sizeof(MallocMetadata);
}
