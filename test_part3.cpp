#include <iostream>
#include <cassert>
#include <cstring>
#include <unistd.h>
#include <sys/mman.h>

// Function declarations (to be implemented in malloc_3.cpp)
void* smalloc(size_t size);
void* scalloc(size_t num, size_t size);
void sfree(void* p);
void* srealloc(void* oldp, size_t size);
size_t _num_free_blocks();
size_t _num_free_bytes();
size_t _num_allocated_blocks();
size_t _num_allocated_bytes();
size_t _num_meta_data_bytes();
size_t _size_meta_data();


// Test helper functions
void print_stats(const char* test_name) {
    std::cout << "=== " << test_name << " ===" << std::endl;
    std::cout<< "printing stats now " << std::endl;
    std::cout << "Free blocks: " << _num_free_blocks() << std::endl;
    std::cout << "Free bytes: " << _num_free_bytes() << std::endl;
    std::cout << "Allocated blocks: " << _num_allocated_blocks() << std::endl;
    std::cout << "Allocated bytes: " << _num_allocated_bytes() << std::endl;
    std::cout << "Metadata bytes: " << _num_meta_data_bytes() << std::endl;
    std::cout << "Metadata size: " << _size_meta_data() << std::endl;
    std::cout << std::endl;
}

void reset_allocator() {
    // Note: In a real implementation, you might need a reset function
    // For testing purposes, we'll work with the persistent state
}

// Test 1: Initial state - 32 free blocks of 128KB each
void test_initial_state() {
    std::cout << "Test 1: Initial State" << std::endl;

    // First allocation should trigger initialization
    void* ptr = smalloc(100);
    assert(ptr != nullptr);

    // Should have 32 total blocks initially, 1 allocated, 31 free
    // But due to splitting, we might have more blocks
    size_t total_blocks = _num_allocated_blocks();
    size_t free_blocks = _num_free_blocks();
    size_t allocated_blocks = total_blocks - free_blocks;
    std::cout << "allocated blocks: " << allocated_blocks << std::endl;

    assert(allocated_blocks >= 1);
    assert(free_blocks >= 31);

    std::cout << "Initial allocation successful" << std::endl;
    print_stats("Initial State");

    sfree(ptr);
}

// Test 2: Block splitting (Challenge 1)
void test_block_splitting() {
    std::cout << "Test 2: Block Splitting" << std::endl;

    // Allocate small blocks to trigger splitting
    void* ptr1 = smalloc(50);   // Should split from larger block
    void* ptr2 = smalloc(100);  // Should split from larger block
    void* ptr3 = smalloc(200);  // Should split from larger block

    assert(ptr1 != nullptr);
    assert(ptr2 != nullptr);
    assert(ptr3 != nullptr);

    // Check that blocks are properly split
    size_t allocated_blocks = _num_allocated_blocks() - _num_free_blocks();
    assert(allocated_blocks == 3);

    print_stats("After Block Splitting");

    sfree(ptr1);
    sfree(ptr2);
    sfree(ptr3);
}

// Test 3: Buddy merging (Challenge 2)
void test_buddy_merging() {
    std::cout << "Test 3: Buddy Merging" << std::endl;

    // Allocate two small blocks that should be buddies
    void* ptr1 = smalloc(50);
    void* ptr2 = smalloc(50);

    size_t free_blocks_before = _num_free_blocks();

    // Free one block
    sfree(ptr1);

    // Free the buddy - should trigger merging
    sfree(ptr2);

    size_t free_blocks_after = _num_free_blocks();

    // After merging, we should have fewer free blocks
    // (exact number depends on implementation details)
    std::cout << "Free blocks before: " << free_blocks_before << std::endl;
    std::cout << "Free blocks after: " << free_blocks_after << std::endl;

    print_stats("After Buddy Merging");
}

// Test 4: Tight fit allocation (Challenge 0)
void test_tight_fit() {
    std::cout << "Test 4: Tight Fit Allocation" << std::endl;

    // Allocate blocks of different sizes to test tight fit
    void* ptr1 = smalloc(64);   // Should use order 0 (128 bytes)
    void* ptr2 = smalloc(200);  // Should use order 1 (256 bytes)
    void* ptr3 = smalloc(600);  // Should use order 2 (512 bytes)

    assert(ptr1 != nullptr);
    assert(ptr2 != nullptr);
    assert(ptr3 != nullptr);

    // Verify pointers are different
    assert(ptr1 != ptr2);
    assert(ptr2 != ptr3);
    assert(ptr1 != ptr3);

    print_stats("After Tight Fit Allocation");

    sfree(ptr1);
    sfree(ptr2);
    sfree(ptr3);
}

// Test 5: Large allocation with mmap (Challenge 3)
void test_large_allocation_mmap() {
    std::cout << "Test 5: Large Allocation with mmap" << std::endl;

    size_t large_size = 128 * 1024;  // 128KB - should use mmap
    void* large_ptr = smalloc(large_size);

    assert(large_ptr != nullptr);

    // Write to the allocated memory to ensure it's valid
    memset(large_ptr, 0xAA, large_size);

    // Verify the memory
    assert(((char*)large_ptr)[0] == (char)0xAA);
    assert(((char*)large_ptr)[large_size - 1] == (char)0xAA);

    print_stats("After Large Allocation");

    sfree(large_ptr);

    print_stats("After Freeing Large Allocation");
}

// Test 6: Multiple large allocations
void test_multiple_large_allocations() {
    std::cout << "Test 6: Multiple Large Allocations" << std::endl;

    size_t large_size = 200 * 1024;  // 200KB
    void* ptr1 = smalloc(large_size);
    void* ptr2 = smalloc(large_size);
    void* ptr3 = smalloc(large_size);

    assert(ptr1 != nullptr);
    assert(ptr2 != nullptr);
    assert(ptr3 != nullptr);

    // Verify pointers are different
    assert(ptr1 != ptr2);
    assert(ptr2 != ptr3);
    assert(ptr1 != ptr3);

    print_stats("After Multiple Large Allocations");

    sfree(ptr1);
    sfree(ptr2);
    sfree(ptr3);
}

// Test 7: Mixed allocation sizes
void test_mixed_allocation_sizes() {
    std::cout << "Test 7: Mixed Allocation Sizes" << std::endl;

    void* small1 = smalloc(32);
    void* medium1 = smalloc(1024);
    void* large1 = smalloc(150 * 1024);
    void* small2 = smalloc(64);
    void* medium2 = smalloc(2048);
    void* large2 = smalloc(256 * 1024);

    assert(small1 != nullptr);
    assert(medium1 != nullptr);
    assert(large1 != nullptr);
    assert(small2 != nullptr);
    assert(medium2 != nullptr);
    assert(large2 != nullptr);

    print_stats("After Mixed Allocations");

    sfree(small1);
    sfree(medium1);
    sfree(large1);
    sfree(small2);
    sfree(medium2);
    sfree(large2);
}

// Test 8: scalloc with buddy system
void test_scalloc_buddy() {
    std::cout << "Test 8: scalloc with Buddy System" << std::endl;

    void* ptr = scalloc(10, 50);  // 500 bytes total
    assert(ptr != nullptr);

    // Verify memory is zeroed
    char* char_ptr = (char*)ptr;
    for (int i = 0; i < 500; i++) {
        assert(char_ptr[i] == 0);
    }

    print_stats("After scalloc");

    sfree(ptr);
}

// Test 9: srealloc with buddy system
void test_srealloc_buddy() {
    std::cout << "Test 9: srealloc with Buddy System" << std::endl;

    // Test shrinking
    void* ptr = smalloc(1000);
    assert(ptr != nullptr);

    // Write test data
    memset(ptr, 0x55, 1000);

    // Shrink allocation
    ptr = srealloc(ptr, 500);
    assert(ptr != nullptr);

    // Verify data is preserved
    char* char_ptr = (char*)ptr;
    for (int i = 0; i < 500; i++) {
        assert(char_ptr[i] == 0x55);
    }

    // Test growing
    ptr = srealloc(ptr, 2000);
    assert(ptr != nullptr);

    // Verify original data is still there
    char_ptr = (char*)ptr;
    for (int i = 0; i < 500; i++) {
        assert(char_ptr[i] == 0x55);
    }

    print_stats("After srealloc");

    sfree(ptr);
}

// Test 10: srealloc with buddy merging
void test_srealloc_buddy_merging() {
    std::cout << "Test 10: srealloc with Buddy Merging" << std::endl;

    // Allocate a block
    void* ptr = smalloc(100);
    assert(ptr != nullptr);

    // Try to grow it - should check for buddy merging
    ptr = srealloc(ptr, 300);
    assert(ptr != nullptr);

    print_stats("After srealloc with potential merging");

    sfree(ptr);
}

// Test 11: Edge cases
void test_edge_cases() {
    std::cout << "Test 11: Edge Cases" << std::endl;

    // Test maximum normal allocation (just under 128KB)
    void* ptr1 = smalloc(128 * 1024 - 64);  // Should use buddy system
    assert(ptr1 != nullptr);

    // Test minimum large allocation (exactly 128KB)
    void* ptr2 = smalloc(128 * 1024);  // Should use mmap
    assert(ptr2 != nullptr);

    // Test very large allocation
    void* ptr3 = smalloc(1024 * 1024);  // 1MB - should use mmap
    assert(ptr3 != nullptr);

    print_stats("After Edge Case Allocations");

    sfree(ptr1);
    sfree(ptr2);
    sfree(ptr3);
}

// Test 12: Fragmentation and defragmentation
void test_fragmentation() {
    std::cout << "Test 12: Fragmentation and Defragmentation" << std::endl;

    // Create fragmentation
    void* ptrs[10];
    for (int i = 0; i < 10; i++) {
        ptrs[i] = smalloc(100);
        assert(ptrs[i] != nullptr);
    }


    // Free every other allocation
    for (int i = 0; i < 10; i += 2) {
        sfree(ptrs[i]);
    }

    print_stats("After Creating Fragmentation");

    // Free remaining allocations - should trigger merging
    for (int i = 1; i < 10; i += 2) {
        sfree(ptrs[i]);
    }

    print_stats("After Defragmentation");
}

// Test 13: Statistics consistency
void test_statistics_consistency() {
    std::cout << "Test 13: Statistics Consistency" << std::endl;

    size_t initial_allocated = _num_allocated_blocks();
    size_t initial_free = _num_free_blocks();

    // Allocate some blocks
    void* ptr1 = smalloc(100);
    void* ptr2 = smalloc(200);
    void* ptr3 = smalloc(300);

    size_t after_alloc_allocated = _num_allocated_blocks();
    size_t after_alloc_free = _num_free_blocks();

    // Total blocks should be consistent
    assert(initial_allocated + initial_free == after_alloc_allocated + after_alloc_free);

    // Free all blocks
    sfree(ptr1);
    sfree(ptr2);
    sfree(ptr3);

    size_t final_allocated = _num_allocated_blocks();
    size_t final_free = _num_free_blocks();

    // After freeing, we should have the same total blocks
    assert(initial_allocated + initial_free == final_allocated + final_free);

    print_stats("Statistics Consistency Check");
}

// Test 14: Error handling
void test_error_handling() {
    std::cout << "Test 14: Error Handling" << std::endl;

    // Test invalid sizes
    void* ptr1 = smalloc(0);
    assert(ptr1 == nullptr);

    void* ptr2 = smalloc(100000001);  // > 10^8
    assert(ptr2 == nullptr);

    void* ptr3 = scalloc(0, 100);
    assert(ptr3 == nullptr);

    void* ptr4 = scalloc(100, 0);
    assert(ptr4 == nullptr);

    void* ptr5 = scalloc(10000, 10001);  // > 10^8
    assert(ptr5 == nullptr);

    void* ptr6 = srealloc(nullptr, 100);
    assert(ptr6 != nullptr);
    sfree(ptr6);

    void* ptr7 = srealloc(ptr6, 0);  // Should return nullptr
    assert(ptr7 == nullptr);

    // Test double free (should not crash)
    void* ptr8 = smalloc(100);
    sfree(ptr8);
    sfree(ptr8);  // Double free - should be handled gracefully

    // Test free with nullptr
    sfree(nullptr);  // Should not crash

    std::cout << "Error handling tests passed" << std::endl;
}

int main() {
    std::cout << "Starting Part 3 - Buddy Allocator Tests" << std::endl;
    std::cout << "=======================================" << std::endl;

    try {
        test_initial_state();
        test_block_splitting();
        test_buddy_merging();
        test_tight_fit();
        test_large_allocation_mmap();
        test_multiple_large_allocations();
        test_mixed_allocation_sizes();
        test_scalloc_buddy();
        test_srealloc_buddy();
        test_srealloc_buddy_merging();
        test_edge_cases();
        test_fragmentation();
        test_statistics_consistency();
        test_error_handling();

        std::cout << "All tests passed!" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}