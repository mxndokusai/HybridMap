#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <new>
#include <stdexcept>
#include <utility>
#include <vector>

namespace yhy {

template <typename Key, typename Value, typename Hash = std::hash<Key>>
class alignas(std::hardware_destructive_interference_size) HashMap {
private:
  struct Entry {
    std::pair<const Key, Value> data;
    template <typename K, typename V>
    Entry(K &&k, V &&v) : data(std::forward<K>(k), std::forward<V>(v)) {}
  };

  // Main hash table contains precomputed hash + pointer per slot.
  struct Slot {
    size_t hash;
    Entry *entry;
    Slot() : hash(0), entry(nullptr) {}
  };

  static_assert(sizeof(Slot) == 16, "Slot should be 16 bytes.");

  std::vector<Slot> table_;
  std::vector<std::unique_ptr<Entry>> entries_;
  size_t size_;
  size_t capacity_;
  Hash hash_fn_;

  static constexpr size_t INITIAL_CAPACITY = 16;
  static constexpr double MAX_LOAD_FACTOR = 0.75;
  static constexpr size_t EMPTY_HASH = 0;

  // Find the next power of 2.
  static size_t next_power_of_2(size_t n) {
    if (n == 0)
      return 1;
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;
    return n + 1;
  }

  // Ensure hash is never 0, use 0 to mark empty slots.
  static size_t make_hash(size_t h) { return h == EMPTY_HASH ? 1 : h; }

  // Quadratic probing for better cache performance.
  size_t probe(size_t index, size_t i) const {
    return (index + i) & (capacity_ - 1); // Fast modulo for power of 2.
  }

  void rehash(size_t new_capacity) {
    std::vector<Slot> old_table = std::move(table_);
    table_.clear();
    table_.resize(new_capacity);
    capacity_ = new_capacity;

    // Reinsert all entries.
    for (auto &slot : old_table)
      if (slot.entry != nullptr)
        insert_internal(slot.hash, slot.entry);
  }

  void insert_internal(size_t hash, Entry *entry) {
    size_t index = hash & (capacity_ - 1);
    // Open addressing to find empty slot.
    for (size_t i = 0; i < capacity_; ++i) {
      size_t pos = probe(index, i);
      if (table_[pos].entry == nullptr) {
        table_[pos].hash = hash;
        table_[pos].entry = entry;
        return;
      }
    }
    // Should never reach here if load factor is maintained.
    throw std::runtime_error("Hash table is full");
  }

  size_t find_slot(const Key &key, size_t hash) const {
    size_t index = hash & (capacity_ - 1);
    for (size_t i = 0; i < capacity_; ++i) {
      size_t pos = probe(index, i);
      // Empty slot, key not found. Invalid index.
      if (table_[pos].entry == nullptr)
        return capacity_;
      // Hash match, check actual key.
      if (table_[pos].hash == hash && table_[pos].entry->data.first == key) {
        return pos;
      }
    }
    return capacity_;
  }

public:
  HashMap() : size_(0), capacity_(INITIAL_CAPACITY), hash_fn_() {
    table_.resize(capacity_);
  }

  HashMap(size_t expected_size)
      : size_(0), capacity_(next_power_of_2(
                      static_cast<size_t>(expected_size / MAX_LOAD_FACTOR))),
        hash_fn_() {
    table_.resize(capacity_);
  }

  ~HashMap() = default;

  // Prevent copying.
  HashMap(const HashMap &) = delete;
  HashMap &operator=(const HashMap &) = delete;

  // Move operations.
  HashMap(HashMap &&) noexcept = default;
  HashMap &operator=(HashMap &&) noexcept = default;

  size_t size() const noexcept { return size_; }
  bool empty() const noexcept { return size_ == 0; }
  size_t capacity() const noexcept { return capacity_; }

  void clear() {
    table_.clear();
    table_.resize(capacity_);
    entries_.clear();
    size_ = 0;
  }

  // Insert or update.
  template <typename K, typename V>
  std::pair<Value *, bool> insert(K &&key, V &&value) {
    // Check if need rehashing.
    if (static_cast<double>(size_ + 1) / capacity_ > MAX_LOAD_FACTOR)
      rehash(capacity_ * 2);
    size_t hash = make_hash(hash_fn_(key));
    size_t index = hash & (capacity_ - 1);
    // Check if key exists.
    for (size_t i = 0; i < capacity_; ++i) {
      size_t pos = probe(index, i);
      if (table_[pos].entry == nullptr) {
        // Empty slot, so insert new entry.
        auto entry = std::make_unique<Entry>(std::forward<K>(key),
                                             std::forward<V>(value));
        Entry *entry_ptr = entry.get();
        entries_.push_back(std::move(entry));

        table_[pos].hash = hash;
        table_[pos].entry = entry_ptr;
        ++size_;
        return {&entry_ptr->data.second, true};
      }

      if (table_[pos].hash == hash && table_[pos].entry->data.first == key) {
        // Key exists, update value.
        table_[pos].entry->data.second = std::forward<V>(value);
        return {&table_[pos].entry->data.second, false};
      }
    }

    throw std::runtime_error("Hash table is full");
  }

  // Lookup.
  Value *find(const Key &key) {
    size_t hash = make_hash(hash_fn_(key));
    size_t pos = find_slot(key, hash);
    if (pos == capacity_)
      return nullptr;
    return &table_[pos].entry->data.second;
  }

  const Value *find(const Key &key) const {
    size_t hash = make_hash(hash_fn_(key));
    size_t pos = find_slot(key, hash);
    if (pos == capacity_)
      return nullptr;
    return &table_[pos].entry->data.second;
  }

  Value &operator[](const Key &key) {
    size_t hash = make_hash(hash_fn_(key));
    size_t pos = find_slot(key, hash);
    if (pos != capacity_)
      return table_[pos].entry->data.second;
    // Insert default value.
    auto result = insert(key, Value{});
    return *result.first;
  }

  // Check if key exists.
  bool contains(const Key &key) const { return find(key) != nullptr; }

  // Erase, does not shrink table.
  bool erase(const Key &key) {
    size_t hash = make_hash(hash_fn_(key));
    size_t pos = find_slot(key, hash);
    if (pos == capacity_)
      return false;

    // Mark slot as deleted by setting entry to nullptr
    // TODO: Use tombstones or backward shifting for better performance.
    table_[pos].entry = nullptr;
    table_[pos].hash = EMPTY_HASH;
    --size_;
    return true;
  }
};

} // namespace yhy
