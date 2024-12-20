#include <iostream>
#include <cstdint>
#include "tm.hpp"
#include <cassert>
#include <string>

int main() {
    int fkjdsl;
    std::cout << "Size: " << sizeof(fkjdsl) << std::endl;
    std::cout << "TL2 Transactional Memory Test\n";

    // Create a shared memory region
    size_t size = 104;
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

    // Test will create 4 transactions sequentially, the first is write-only, the second is read-only, the third is write and read, and the fourth is read-only
    tx_t tx1 = tm_begin(shared, false);
    int data = 124;
    tm_write(shared, tx1, &data, sizeof(data), start);
    tm_end(shared, tx1);

    tx_t tx2 = tm_begin(shared, true);
    int read_data;
    tm_read(shared, tx2, start, sizeof(data), &read_data);
    std::cout << "PRINT: TX 3 Read: " << read_data << std::endl;
    tm_end(shared, tx2);

    tx_t tx3 = tm_begin(shared, false);
    int read_data_2;

    tm_read(shared, tx2, start, sizeof(data), &read_data);
    std::cout << "PRINT: TX 3 Read: " << read_data_2 << std::endl;
    read_data_2 += 3;
    tm_write(shared, tx3, &read_data_2, sizeof(read_data_2), start);
    tm_end(shared, tx3);

    tx_t tx4 = tm_begin(shared, true);
    int read_data_3;
    tm_read(shared, tx4, start, sizeof(read_data_2), &read_data_3);
    std::cout << "PRINT: TX 4 Read: " << read_data_3 << std::endl;
    tm_end(shared, tx4);
}


// Strings
// int main() {
//     std::cout << "TL2 Transactional Memory Test\n";

//     // Create a shared memory region
//     size_t size = 104;
//     size_t align = 8;
//     shared_t shared = tm_create(size, align);
//     if (shared == invalid_shared) {
//         std::cerr << "Failed to create shared memory\n";
//         return 1;
//     }

//     // Get and print shared memory information
//     void* start = tm_start(shared);
//     size_t actual_size = tm_size(shared);
//     size_t actual_align = tm_align(shared);
//     std::cout << "Shared memory: start=" << start << ", size=" << actual_size << ", align=" << actual_align << "\n";

//     // Test will create 4 transactions sequentially, the first is write-only, the second is read-only, the third is write and read, and the fourth is read-only
//     tx_t tx1 = tm_begin(shared, false);
//     std::string data = "Hello";
//     tm_write(shared, tx1, &data, sizeof(data), start);
//     tm_end(shared, tx1);

//     tx_t tx2 = tm_begin(shared, true);
//     std::string read_data;
//     tm_read(shared, tx2, start, sizeof(data), &read_data);
//     std::cout << "PRINT: TX 3 Read: " << read_data << std::endl;
//     tm_end(shared, tx2);

//     tx_t tx3 = tm_begin(shared, false);
//     std::string read_data_2;
//     tm_read(shared, tx2, start, sizeof(data), &read_data);
//     std::cout << "PRINT: TX 3 Read: " << read_data_2 << std::endl;
//     read_data_2 += " World";
//     tm_write(shared, tx3, &read_data_2, sizeof(read_data_2), start);
//     tm_end(shared, tx3);

//     tx_t tx4 = tm_begin(shared, true);
//     std::string read_data_3;
//     tm_read(shared, tx4, start, sizeof(read_data_2), &read_data_3);
//     std::cout << "PRINT: TX 4 Read: " << read_data_3 << std::endl;
//     tm_end(shared, tx4);
// }
