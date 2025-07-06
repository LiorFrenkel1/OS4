#include <unistd.h>
#include <cstring>

struct MallocMetadata {
    size_t size; // size of the allocated data (not including MallocMetadata)
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;
};

MallocMetadata* firstMeta = NULL;

void* smalloc(size_t size) {
    if (size == 0 || size > 100000000) {
        return NULL;
    }
    void* ptrToAlloc = NULL;
    MallocMetadata* current = firstMeta;
    while (current != NULL) {
        if (current->is_free && current->size >= size) {
            ptrToAlloc = (void*)((char*)current + sizeof(MallocMetadata));
            current->is_free = false;
        }
        current = current->next;
    }
    if (ptrToAlloc == NULL) {
        void* metaAndData = sbrk(sizeof(MallocMetadata) + size);
        if (metaAndData == (void*)(-1)) {
            return NULL;
        }
        MallocMetadata* newMetaData = (MallocMetadata*)metaAndData;
        ptrToAlloc = (void*)((char*)metaAndData + sizeof(MallocMetadata));
        newMetaData->next = NULL;
        newMetaData->is_free = false;
        newMetaData->size = size;
        if (firstMeta == NULL) {
            newMetaData->prev = NULL;
            firstMeta = newMetaData;
        } else {
            current = firstMeta;
            while (current->next != NULL) {
                current = current->next;
            }
            newMetaData->prev = current;
            current->next = newMetaData;
        }
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
    metadata->is_free = true; //now p space is free
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
