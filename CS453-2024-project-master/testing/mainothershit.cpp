#include <iostream>
#include <cstdint>
#include "tm.hpp"

int main() {
    std::cout << "TL2 Transactional Memory Test\n";

    // Create a shared memory region
    size_t size = 1024;
    size_t align = 8;
    shared_t shared = tm_create(size, align);
    if (shared == invalid_shared) {
        std::cerr << "Failed to create shared memory\n";
        return 1;
    }

    // Get and print shared memory information
    void* start = tm_start(shared);
    size_t actual_size = tm_size(shared);
    size_t actual_align = tm_align(shared);
    std::cout << "Shared memory: start=" << start << ", size=" << actual_size << ", align=" << actual_align << "\n";

    // Begin a transaction
    tx_t tx = tm_begin(shared, false);
    if (tx == invalid_tx) {
        std::cerr << "Failed to begin transaction\n";
        tm_destroy(shared);
        return 1;
    }

    // Perform a write operation
    std::cout << "Performing write operation 1\n" << std::endl;
    double value = 42;
    void* addr = start;
    if (!tm_write(shared, tx, &value, sizeof(value), addr)) {
        std::cerr << "Write operation failed\n";
        tm_destroy(shared);
        return 1;
    }

    if (!tm_end(shared, tx)) {
        std::cerr << "Failed to commit transaction\n";
        tm_destroy(shared);
        return 1;
    }

    // Check the write worked
    tx = tm_begin(shared, true);
    if (tx == invalid_tx) {
        std::cerr << "Failed to begin read-only transaction\n";
        return 1;
    }

    double read_value;
    if (!tm_read(shared, tx, addr, sizeof(read_value), &read_value)) {
        std::cerr << "Read operation failed in here\n";
        tm_destroy(shared);
        return 1;
    }
    std::cout << "Read value from diff transaction: " << read_value << "\n";

    // Perform a read operation
    int32_t read_value;
    if (!tm_read(shared, tx, addr, sizeof(read_value), &read_value)) {
        std::cerr << "Read operation failed\n";
        tm_destroy(shared);
        return 1;
    }
    std::cout << "Read value: " << read_value << "\n";

    // Allocate memory within the transaction
    void* new_addr;
    Alloc alloc_result = tm_alloc(shared, tx, sizeof(int64_t), &new_addr);
    if (alloc_result != Alloc::success) {
        std::cerr << "Allocation failed\n";
        tm_destroy(shared);
        return 1;
    }

    // Write to the newly allocated memory
    std::cout << "Performing write operation 2\n" << std::endl;
    int64_t long_value = 1234567890;
    if (!tm_write(shared, tx, &long_value, sizeof(long_value), new_addr)) {
        std::cerr << "Write to allocated memory failed\n";
        tm_destroy(shared);
        return 1;
    }

    // End the transaction
    if (!tm_end(shared, tx)) {
        std::cerr << "Failed to commit transaction\n";
        tm_destroy(shared);
        return 1;
    }

    // Begin a new read-only transaction
    tx_t ro_tx = tm_begin(shared, true);
    if (ro_tx == invalid_tx) {
        std::cerr << "Failed to begin read-only transaction\n";
        tm_destroy(shared);
        return 1;
    }

    // Read from the previously allocated memory
    std::cout << "About to read old allocated memory\n" << std::endl;
    int64_t read_long_value;
    if (!tm_read(shared, ro_tx, new_addr, sizeof(read_long_value), &read_long_value)) {
        std::cerr << "Read from allocated memory failed as expected\n";
        // tm_destroy(shared);
    }

    // End the read-only transaction
    if (!tm_end(shared, ro_tx)) {
        std::cerr << "Failed to commit read-only transaction\n";
        tm_destroy(shared);
        return 1;
    }

    // Free the allocated memory
    tx_t free_tx = tm_begin(shared, false);
    if (free_tx == invalid_tx) {
        std::cerr << "Failed to begin free transaction\n";
        tm_destroy(shared);
        return 1;
    }
    if (!tm_free(shared, free_tx, new_addr)) {
        std::cerr << "Failed to free allocated memory\n";
        tm_destroy(shared);
        return 1;
    }
    if (!tm_end(shared, free_tx)) {
        std::cerr << "Failed to commit free transaction\n";
        tm_destroy(shared);
        return 1;
    }

    // // Destroy the shared memory region
    // tm_destroy(shared);

    std::cout << "Test completed successfully\n";
    return 0;
}
