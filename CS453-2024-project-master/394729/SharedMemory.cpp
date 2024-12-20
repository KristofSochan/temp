#include "SharedMemory.hpp"
#include <cstdlib>
#include <cstring>
#include <stdexcept>

SharedMemory::SharedMemory(size_t size, size_t align) : size(size), align(align), version_clock(0) {
    start = aligned_alloc(align, size);
    if (!start) {
        throw std::runtime_error("Failed to allocate shared memory.");
    }

    // Initialize the first segment with zeroes
    std::memset(start, 0, size);

    // Lock segment list mutex then add the first segment, then unlock
    Segment segment = {start, size};
    segmentListMutex.lock();
    segments.push_back(segment);
    segmentListMutex.unlock();

    for (VersionedLock*& lock : locks) {
        lock = new VersionedLock();
    }
}

SharedMemory::~SharedMemory() {

    // Iterate over all segments and free in a thread-safe manner
    segmentListMutex.lock();
    for (const Segment& segment : segments) {
        free(segment.start);
    }
    segmentListMutex.unlock();

    // Delete all locks
    for (VersionedLock* lock : locks) {
        delete lock;
    }
}

void* SharedMemory::get_start() const {
    return start;
}

size_t SharedMemory::get_size() const {
    return size;
}

size_t SharedMemory::get_align() const {
    return align;
}

// Get the lock for the given address
VersionedLock* SharedMemory::get_lock(const void* address) {
    return locks[uint64_t(address) % 10000];
}

uint64_t SharedMemory::increment_version_clock() {
    return version_clock.fetch_add(1) + 1;
}

uint64_t SharedMemory::get_version_clock() const {
    return version_clock.load();
}

