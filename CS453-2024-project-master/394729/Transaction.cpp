#include "Transaction.hpp"

Transaction::Transaction(uint64_t read_version, bool is_read_only)
    : read_version(read_version), write_version(0), is_read_only(is_read_only), active(true) {
    }

Transaction::~Transaction() {
    for (auto entry : read_set) {
        delete entry;
    }
    for (auto entry : write_set) {
        delete entry.second;
    }
}
void Transaction::add_read(const void* addr, uint64_t version) {
    read_set.push_back(new ReadSetEntry{addr, version});
}

void Transaction::add_write(void* addr, void* value, uint64_t version, size_t size_to_write) {
    // Delete the old entry if it exists
    if (write_set.find(addr) != write_set.end()) {
        delete write_set[addr];
    }
    write_set[addr] = new WriteSetEntry{addr, value, version, size_to_write};
}

bool Transaction::is_active() const {
    return active;
}

bool Transaction::is_read_only_tx() const {
    return is_read_only;
}

const std::unordered_map<void*, WriteSetEntry*> Transaction::get_write_set() const {
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
