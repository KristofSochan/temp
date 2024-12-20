#include <iostream>
#include <cstdint>
#include "tm.hpp"
#include <cassert>

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

    // New transaction where we try to allocate multiple sections of memory
    tx_t tx2 = tm_begin(shared, false);
    if (tx2 == invalid_tx) {
        std::cerr << "Failed to begin transaction\n";
        tm_destroy(shared);
        return 1;
    }

    // Allocate memory within the transaction
    void* new_addr;
    double value2 = 369;
    Alloc alloc_result = tm_alloc(shared, tx2, sizeof(value2), &new_addr);

    if (alloc_result != Alloc::success) {
        std::cerr << "Allocation failed\n";
        tm_destroy(shared);
        return 1;
    }

    // Write to the newly allocated memory
    std::cout << "Performing write operation 2\n" << std::endl;
    if (!tm_write(shared, tx2, &value2, sizeof(value2), new_addr)) {
        std::cerr << "Write operation failed\n";
        tm_destroy(shared);
        return 1;
    }

    // commit transaction 2
    if (!tm_end(shared, tx2)) {
        std::cerr << "Failed to commit transaction\n";
        tm_destroy(shared);
        return 1;
    }
    
    // Create new transaction to read the value
    tx_t tx3 = tm_begin(shared, false);
    if (tx3 == invalid_tx) {
        std::cerr << "Failed to begin transaction\n";
        tm_destroy(shared);
        return 1;
    }

    // Read the value back from old transaction
    double read_value2;
    if (!tm_read(shared, tx3, new_addr, sizeof(read_value2), &read_value2)) {
        std::cerr << "Read operation failed here\n";
        tm_destroy(shared);
        return 1;
    }
    std::cout << "Read value from same transaction nutstack: " << read_value2 << "\n";

    assert(read_value2 == value2);
}