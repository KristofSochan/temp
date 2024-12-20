#include "VersionedLock.hpp"

bool VersionedLock::lock() {
        uint64_t l = lock_and_version.load();
        
        // Check if lock is already taken
        if (l & 0x1) {
            return false;
        }

        // Try to take lock
        return lock_and_version.compare_exchange_strong(l, l | 0x1);
    }

void VersionedLock::unlock() {
    lock_and_version.fetch_sub(1);
}

void VersionedLock::update_version(uint64_t new_version) {
    lock_and_version.store(new_version << 1); // Shift the version back into place and clear the lock bit
}

uint64_t VersionedLock::load() const {
    return lock_and_version.load();
}
