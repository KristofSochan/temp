#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

#include <vector>
#include <array>
#include <cstddef>
#include <mutex>
#include "VersionedLock.hpp"

class Segment {
public:
    void* start;
    size_t size;
};


class SharedMemory {
private:   
    void* start;
    size_t size;
    size_t align;

    // Keep track of allocated segments
    std::vector<Segment> segments;
    std::mutex segmentListMutex;

    std::array<VersionedLock*, 10000> locks;
    std::atomic<uint64_t> version_clock;
    std::mutex global_lock;

public:

    SharedMemory(size_t size, size_t align);
    ~SharedMemory();

    void* get_start() const;
    size_t get_size() const;
    size_t get_align() const;

    VersionedLock* get_lock(const void* index);
    uint64_t increment_version_clock();
    uint64_t get_version_clock() const;

    // addr = Address to stop unlocking at, set NULL if entire write set should be unlocked
    void unlock_set(void* addr);
};

#endif // SHARED_MEMORY_H
