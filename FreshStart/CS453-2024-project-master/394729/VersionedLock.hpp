#ifndef VERSIONED_LOCK_H
#define VERSIONED_LOCK_H

#include <atomic>

class VersionedLock {
private:
    std::atomic<uint64_t> lock_and_version{0}; // Initialize to 0

public:
    VersionedLock() = default;
    bool lock();
    void unlock();
    void update_version(uint64_t new_version);
    uint64_t load() const;
};

#endif // VERSIONED_LOCK_H
