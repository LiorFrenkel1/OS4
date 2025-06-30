#include <unistd.h>

struct MallocMetadata {
    size_t size; // size of the allocated data (not including MallocMetadata)
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;
};

MallocMetadata* firstMeta = NULL;

size_t _size_meta_data();

void* smalloc(size_t size) {
    if (size == 0 || size > 100000000) {
        return NULL;
    }
    void* ptrToAlloc = NULL;
    MallocMetadata* current = firstMeta;
    while (current->next != NULL) {
        if (current->is_free && current->size >= size) {
            ptrToAlloc = (void*)((char*)current + _size_meta_data());
            current->is_free = false;
        }
        current = current->next;
    }
    if (current->is_free && current->size >= size) {
        ptrToAlloc = (void*)((char*)current + _size_meta_data());
        current->is_free = false;
    }
    if (ptrToAlloc == NULL) {
        current->next = sbrk(_size_meta_data());
        current->next->size = size;
        current->next->is_free = false;
        current->next->next = NULL;
        current->next->prev = current;
        ptrToAlloc = sbrk(size);
    }
    return ptrToAlloc;
}

size_t _size_meta_data() {
    return sizeof(MallocMetadata);
}
