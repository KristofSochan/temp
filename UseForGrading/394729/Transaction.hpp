#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <unordered_map>
#include <vector>

#include "VersionedLock.hpp"

struct ReadSetEntry {
    const void* address;      // Address being read
    uint64_t version;   // Version of the lock at the time of the read
};

struct WriteSetEntry {
    const void* address;
    void* new_value;
    uint64_t version;
    size_t size_to_write;
};

class Transaction {
private:
    uint64_t read_version;
    uint64_t write_version;
    bool is_read_only;
    bool active;
    std::vector<ReadSetEntry*> read_set;
    std::unordered_map<void*, WriteSetEntry*> write_set;

public:
    Transaction(uint64_t read_version, bool is_read_only);
    ~Transaction();
    void add_read(const void* addr, uint64_t version);
    void add_write(void* addr, void* value, uint64_t version, size_t size_to_write);
    bool is_active() const;
    bool is_read_only_tx() const;
    const std::unordered_map<void*, WriteSetEntry*> get_write_set() const;
    const std::vector<ReadSetEntry*> get_read_set() const;
    uint64_t get_read_version();
    uint64_t get_wv();
    void set_wv(uint64_t wv);
    void commit(uint64_t write_version);
    void abort();
};

#endif // TRANSACTION_H
