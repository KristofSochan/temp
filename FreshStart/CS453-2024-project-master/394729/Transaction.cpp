#include "Transaction.hpp"
#include <cstdlib>
#include <cstring>

Transaction::Transaction(uint64_t read_version, bool is_read_only)
    : read_version(read_version), write_version(0), is_read_only(is_read_only), active(true) {
    }

Transaction::~Transaction() {
    for (auto entry : read_set) {
        delete entry;
    }

    for (auto entry : write_set) {
        free(entry.second->new_value);
        delete(entry.second);
    }
}
void Transaction::add_read(const void* addr, uint64_t version) {
    read_set.push_back(new ReadSetEntry{addr, version});
}

void Transaction::add_write(void* addr, void* ptr_to_value, uint64_t version, size_t size_to_write) {
    // Delete the old entry if it exists
    auto it = write_set.find(addr);
    if (it != write_set.end()) {
        free(it->second->new_value);
        delete it->second;
    }

    void* new_val = malloc(size_to_write);
    memcpy(new_val, ptr_to_value, size_to_write);

    write_set[addr] = new WriteSetEntry{addr, new_val, version, size_to_write};
}

bool Transaction::is_active() const {
    return active;
}

bool Transaction::is_read_only_tx() const {
    return is_read_only;
}

const std::unordered_map<void*, WriteSetEntry*>& Transaction::get_write_set() const {
    return write_set;
}

const std::vector<ReadSetEntry*> Transaction::get_read_set() const {
    return read_set;
}

void Transaction::commit(uint64_t write_version) {
    this->write_version = write_version;
    active = false;
}

void Transaction::set_wv(uint64_t wv) {
    write_version = wv;
}

uint64_t Transaction::get_wv() {
    return write_version;
}

void Transaction::abort() {
    active = false;
}

uint64_t Transaction::get_read_version() {
    return read_version;
}
