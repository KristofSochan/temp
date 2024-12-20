/**
 * @file   tm.cpp
 * @author [...]
 *
 * @section LICENSE
 *
 * [...]
 *
 * @section DESCRIPTION
 *
 * Implementation of your own transaction manager.
 * You can completely rewrite this file (and create more files) as you wish.
 * Only the interface (i.e. exported symbols and semantic) must be preserved.
**/

// External headers

// Internal headers
#include <tm.hpp>

#include "macros.h"

// Added headers

#include <cstdlib>
#include <cstring>
#include <mutex>
#include "tm.hpp"
#include "SharedMemory.hpp"
#include "Transaction.hpp"

#include <iostream>

// ADDED UTILS

// Stop at stop_addr, if stop_addr is NULL, unlock the entire write set
void utils_unlock_set(SharedMemory* shared_mem, Transaction* transaction, void* stop_addr) {
    // Unlock all locks in the write set up to addr
    for (const auto& [addr, entry] : transaction->get_write_set()) {
        if (!addr) {
            std::cerr << "Unlocking NULL address" << std::endl;
            exit(1);
        }
        if (addr == stop_addr) break;
        shared_mem->get_lock(addr)->unlock();
    }
}

//
// End added headers
/** Create (i.e. allocate + init) a new shared memory region, with one first non-free-able allocated segment of the requested size and alignment.
 * @param size  Size of the first shared segment of memory to allocate (in bytes), must be a positive multiple of the alignment
 * @param align Alignment (in bytes, must be a power of 2) that the shared memory region must support
 * @return Opaque shared memory region handle, 'invalid_shared' on failure
**/
shared_t tm_create(size_t size, size_t align) noexcept {
    // Allocate and initialize the shared memory region
    try {
        return static_cast<shared_t>(new SharedMemory(size, align));
    } catch (const std::exception& e) {
        return invalid_shared;
    }
}

/** Destroy (i.e. clean-up + free) a given shared memory region.
 * @param shared Shared memory region to destroy, with no running transaction
**/
void tm_destroy(shared_t shared) noexcept {
    SharedMemory* shared_mem = static_cast<SharedMemory*>(shared);
    delete shared_mem;
}

/** [thread-safe] Return the start address of the first allocated segment in the shared memory region.
 * @param shared Shared memory region to query
 * @return Start address of the first allocated segment
**/
void* tm_start(shared_t shared) noexcept {
    SharedMemory* shared_mem = static_cast<SharedMemory*>(shared);
    return shared_mem->get_start();
}

/** [thread-safe] Return the size (in bytes) of the first allocated segment of the shared memory region.
 * @param shared Shared memory region to query
 * @return First allocated segment size
**/
size_t tm_size(shared_t shared) noexcept {
    SharedMemory* shared_mem = static_cast<SharedMemory*>(shared);
    return shared_mem->get_size();
}

/** [thread-safe] Return the alignment (in bytes) of the memory accesses on the given shared memory region.
 * @param shared Shared memory region to query
 * @return Alignment used globally
**/
size_t   tm_align(shared_t shared) noexcept {
    SharedMemory* shared_mem = static_cast<SharedMemory*>(shared);
    return shared_mem->get_align();
}

/** [thread-safe] Begin a new transaction on the given shared memory region.
 * @param shared Shared memory region to start a transaction on
 * @param is_ro  Whether the transaction is read-only
 * @return Opaque transaction ID, 'invalid_tx' on failure
**/
int counter = 1;
tx_t tm_begin(shared_t shared, bool is_ro) noexcept {
    SharedMemory* shared_mem = static_cast<SharedMemory*>(shared);
    // Create a new Transaction object
    Transaction* tx = new Transaction(shared_mem->get_version_clock(), is_ro);

    // Return the opaque handle (convert Transaction* to tx_t)
    return reinterpret_cast<tx_t>(tx);
}

/** [thread-safe] End the given transaction.
 * @param shared Shared memory region associated with the transaction
 * @param tx     Transaction to end
 * @return Whether the whole transaction committed
**/
bool tm_end(shared_t shared, tx_t tx) noexcept {
    SharedMemory* shared_mem = static_cast<SharedMemory*>(shared);
    Transaction* transaction = reinterpret_cast<Transaction*>(tx);

    // If it's a read-only transaction, we can commit immediately
    if (transaction->is_read_only_tx()) {
        delete transaction;
        return true;
    }

    // Acquire locks for all locations in the write set
    for (const auto& [addr, entry] : transaction->get_write_set()) {
        VersionedLock* lock = shared_mem->get_lock(addr);
        if (!lock->lock()) {
            // If we fail to acquire any lock, release all acquired locks and abort
            utils_unlock_set(shared_mem, transaction, addr);
            
            delete transaction;
            return false;
        }
    }

    // Increment the global version clock
    transaction->set_wv(shared_mem->increment_version_clock());

    if (transaction->get_read_version() + 1 != transaction->get_wv()) { // Checking special case where read set validation not needed
        // Validate the read set
        for (const auto& read_set_entry : transaction->get_read_set()) {
            VersionedLock* lock = shared_mem->get_lock(read_set_entry->address);
            uint64_t l = lock->load();
            if (l & 0x1 || (l >> 1) > transaction->get_read_version()) {
                // If validation fails, release all locks and abort
                utils_unlock_set(shared_mem, transaction, nullptr);

                delete transaction;
                return false;
            }
        }

    }

    // Commit: write values and release locks
    for (const auto& [addr, entry] : transaction->get_write_set()) {
        memcpy((void*)addr, entry->new_value, entry->size_to_write);

        VersionedLock* lock = shared_mem->get_lock(addr);
        lock->update_version(transaction->get_wv());
    }

    // Clean up
    delete transaction;
    return true;
}

/** [thread-safe] Read operation in the given transaction, source in the shared region and target in a private region.
 * @param shared Shared memory region associated with the transaction
 * @param tx     Transaction to use
 * @param source Source start address (in the shared region)
 * @param size   Length to copy (in bytes), must be a positive multiple of the alignment
 * @param target Target start address (in a private region)
 * @return Whether the whole transaction can continue
**/
bool tm_read(shared_t shared, tx_t tx, void const* source, size_t size, void* target) noexcept {
    Transaction* transaction = reinterpret_cast<Transaction*>(tx);
    SharedMemory* shared_memory = static_cast<SharedMemory*>(shared);
    
    size_t align = shared_memory->get_align();

    if (transaction->is_read_only_tx()) {
        // For each word valid lock and version

        for (size_t i = 0; i < size / align; i++) {
            void* source_word = (char*)source + i * align;
            void* target_word = (char*)target + i * align;

            // Check that lock is free and version is < read_version
            VersionedLock* lock = shared_memory->get_lock(source_word);
            uint64_t l = lock->load();
            if (unlikely(l & 0x1 || (l >> 1) > transaction->get_read_version())) {
                delete transaction;
                return false;
            }

            // Copy the word
            memcpy(target_word, source_word, align);

            // Post-validation that version hasn't changed
            uint64_t afterl = lock->load();
            if (afterl != l) {
                delete transaction;
                return false;
            }
        }
    }
    else { // Standard case, Write Transaction
        for (size_t i = 0; i < size / align; i++) {
            void* target_word = (char*)target + i * align;
            void* source_word = (char*)source + i * align;

            // Check if source_word has already been modified by transaction
            std::unordered_map<void*, WriteSetEntry*> write_set = transaction->get_write_set();
            if (write_set.find(source_word) != write_set.end()) {
                memcpy(target_word, write_set[source_word]->new_value, align);
            }
            else {
                // Check that lock is free and version is <= read_version
                VersionedLock* lock = shared_memory->get_lock(source_word);
                uint64_t l = lock->load();
                if (l & 0x1 || (l >> 1) > transaction->get_read_version()) {
                    delete transaction;
                    return false;
                }

                // Copy the word
                memcpy(target_word, source_word, align);

                // Post-validation that version hasn't changed
                uint64_t afterl = lock->load();
                if (afterl != l) {
                    delete transaction;
                    return false;
                }

                // Add to read set
                transaction->add_read(source_word, l >> 1);
            }
        }
    }
    return true;
}

/** [thread-safe] Write operation in the given transaction, source in a private region and target in the shared region.
 * @param shared Shared memory region associated with the transaction
 * @param tx     Transaction to use
 * @param source Source start address (in a private region)
 * @param size   Length to copy (in bytes), must be a positive multiple of the alignment
 * @param target Target start address (in the shared region)
 * @return Whether the whole transaction can continue
**/
bool tm_write(shared_t shared, tx_t tx, void const* source, size_t size, void* target) noexcept {
    Transaction* transaction = reinterpret_cast<Transaction*>(tx);
    SharedMemory* shared_mem = static_cast<SharedMemory*>(shared);

    size_t align = shared_mem->get_align();
    for (size_t i = 0; i < size / align; i++) {
        void* target_word = (char*)target + i * align;
        void* source_word = (char*)source + i * align;

       // Add to write set
       transaction->add_write(target_word, source_word, shared_mem->get_version_clock(), size);
    }

    return true;
}

/** [thread-safe] Memory allocation in the given transaction.
 * @param shared Shared memory region associated with the transaction
 * @param tx     Transaction to use
 * @param size   Allocation requested size (in bytes), must be a positive multiple of the alignment
 * @param target Pointer in private memory receiving the address of the first byte of the newly allocated, aligned segment
 * @return Whether the whole transaction can continue (success/nomem), or not (abort_alloc)
**/
Alloc tm_alloc(shared_t shared, tx_t tx, size_t size, void** target) noexcept {
    Transaction* transaction = reinterpret_cast<Transaction*>(tx);
    SharedMemory* shared_mem = static_cast<SharedMemory*>(shared);

    if (!shared || !transaction || !transaction->is_active() || size % shared_mem->get_align() != 0) {
        std::cerr << "Invalid arguments" << std::endl;
        return Alloc::abort;
    }

    // Allocate memory
    void* new_location = aligned_alloc(shared_mem->get_align(), size);
    if (!new_location) return Alloc::nomem;

    // Add to segments list
    shared_mem->add_segment(new_location, size);

    // Initialize segment words with 0s
    memset(new_location, 0, size);

    // Write target
    *target = new_location;

    return Alloc::success;
}

/** [thread-safe] Memory freeing in the given transaction.
 * @param shared Shared memory region associated with the transaction
 * @param tx     Transaction to use
 * @param target Address of the first byte of the previously allocated segment to deallocate
 * @return Whether the whole transaction can continue
**/
// Will be cleaned at the end of the transaction
bool tm_free(shared_t shared, tx_t tx, void* target) noexcept {
    return true;
}
