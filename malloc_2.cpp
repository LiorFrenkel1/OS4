#include <unistd.h>

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

void sfree(void* p) {
    //TODO
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

size_t _num_allocated_bytes() {
    MallocMetadata* current = firstMeta;
    size_t numOfBytes = 0;
    while (current != NULL) {
        numOfBytes += current->size;
        current = current->next;
    }
    return numOfBytes;
}

size_t _size_meta_data() {
    return sizeof(MallocMetadata);
}
