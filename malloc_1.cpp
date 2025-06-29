#include <unistd.h>

void* smalloc(size_t size) {
    if (size == 0 || size > 100000000) {
        return NULL;
    }
    intptr_t sizeIntPtr;
    sizeIntPtr = (intptr_t)size;
    void* pointerToMalloc = sbrk(sizeIntPtr);
    if (pointerToMalloc == (void*)(-1)) {
        return NULL;
    }
    return pointerToMalloc;
}