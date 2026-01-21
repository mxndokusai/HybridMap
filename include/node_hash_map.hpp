#pragma once

#include "constants.hpp"
#include <cstddef>
#include <functional>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

namespace yhy {

template <typename Key, typename Value, typename Hash = std::hash<Key>>
// class alignas(std::hardware_destructive_interference_size) NodeHashMap {
class NodeHashMap {
private:
  struct Entry {
    std::pair<const Key, Value> data;
    template <typename K, typename V>
    // Entry(K &&k, V &&v) : data(std::forward<K>(k), std::forward<V>(v)) {}
    Entry(K &&k, V &&v)
        : data(std::piecewise_construct,
               std::forward_as_tuple(std::forward<K>(k)),
               std::forward_as_tuple(std::forward<V>(v))) {}
  };

  // Main hash table contains precomputed hash + pointer per slot.
  struct alignas(16) Slot {
    size_t hash;
    Entry *entry;
    Slot() : hash(0), entry(nullptr) {}
  };

  static_assert(sizeof(Slot) == 16, "Slot should be 16 bytes");

  std::vector<Slot> table_;
  std::vector<std::unique_ptr<Entry>> entries_;
  size_t size_;
  size_t capacity_;
  size_t tombstone_count_;
  Hash hash_fn_;

  // Sentinel pointer value to mark tombstones.
  static Entry *tombstone_ptr() {
    return reinterpret_cast<Entry *>(static_cast<uintptr_t>(1));
  }

  // Checks if slot is tombstone.
  static bool is_tombstone(const Slot &slot) {
    return slot.entry == tombstone_ptr();
  }

  // Check if a slot is empty.
  static bool is_empty(const Slot &slot) { return slot.entry == nullptr; }

  // Ensure hash is never 0 or 1, 0 marks empty, 1 marks tombstone.
  static size_t make_hash(size_t h) {
    return (h == EMPTY_HASH || h == TOMBSTONE_HASH) ? 2 : h;
  }

  // Linear probing for better cache performance.
  size_t probe(size_t index, size_t i) const {
    return (index + i) & (capacity_ - 1); // Fast modulo for power of 2.
  }

  void rehash(size_t new_capacity) {
    std::vector<Slot> old_table = std::move(table_);
    table_.clear();
    table_.resize(new_capacity);
    capacity_ = new_capacity;
    tombstone_count_ = 0;

    // Reinsert all valid entries.
    for (auto &slot : old_table)
      if (!is_empty(slot) && !is_tombstone(slot))
        insert_internal(slot.hash, slot.entry);
  }

  void insert_internal(size_t hash, Entry *entry) {
    size_t index = hash & (capacity_ - 1);
    // Open addressing to find empty slot.
    for (size_t i = 0; i < capacity_; ++i) {
      size_t pos = probe(index, i);
      if (is_empty(table_[pos]) || is_tombstone(table_[pos])) {
        if (is_tombstone(table_[pos]))
          --tombstone_count_;
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
      if (is_empty(table_[pos]))
        return capacity_;
      // Skip tombstones and continue probing.
      if (is_tombstone(table_[pos]))
        continue;
      // Hash match, check actual key.
      if (table_[pos].hash == hash && table_[pos].entry->data.first == key) {
        return pos;
      }
    }
    return capacity_;
  }

public:
  // Used for type trait tests.
  using key_type = Key;
  using mapped_type = Value;
  NodeHashMap()
      : size_(0), capacity_(INITIAL_CAPACITY), tombstone_count_(0), hash_fn_() {
    table_.resize(capacity_);
  }

  explicit NodeHashMap(size_t expected_size)
      : size_(0), capacity_(next_power_of_2(
                      static_cast<size_t>(expected_size / MAX_LOAD_FACTOR))),
        tombstone_count_(0), hash_fn_() {
    table_.resize(capacity_);
  }

  ~NodeHashMap() = default;

  // Prevent copying.
  NodeHashMap(const NodeHashMap &) = delete;
  NodeHashMap &operator=(const NodeHashMap &) = delete;

  // Move operations.
  NodeHashMap(NodeHashMap &&) noexcept = default;
  NodeHashMap &operator=(NodeHashMap &&) noexcept = default;

  size_t size() const noexcept { return size_; }
  bool empty() const noexcept { return size_ == 0; }
  size_t capacity() const noexcept { return capacity_; }

  void clear() {
    table_.clear();
    table_.resize(capacity_);
    entries_.clear();
    size_ = 0;
    tombstone_count_ = 0;
  }

  // Insert or update.
  template <typename K, typename V>
  std::pair<Value *, bool> insert(K &&key, V &&value) {
    // Check if need rehashing.
    double load = static_cast<double>(size_ + 1) / capacity_;
    double ratio = static_cast<double>(tombstone_count_) / capacity_;
    if (load > MAX_LOAD_FACTOR || ratio > MAX_TOMBSTONE_RATIO)
      rehash(capacity_ * 2);
    size_t hash = make_hash(hash_fn_(key));
    size_t index = hash & (capacity_ - 1);
    // Check if key exists or find first tombstone for reuse.
    size_t first_tombstone = capacity_; // Track first tombstone seen.

    for (size_t i = 0; i < capacity_; ++i) {
      size_t pos = probe(index, i);
      // Empty slot, use tombstone if seen, otherwise use this.
      if (is_empty(table_[pos])) {
        size_t insert_pos =
            (first_tombstone != capacity_) ? first_tombstone : pos;
        // Reuse tombstone.
        if (first_tombstone != capacity_)
          --tombstone_count_;
        // Insert new entry.
        auto entry = std::make_unique<Entry>(std::forward<K>(key),
                                             std::forward<V>(value));
        Entry *entry_ptr = entry.get();
        entries_.push_back(std::move(entry));

        table_[insert_pos].hash = hash;
        table_[insert_pos].entry = entry_ptr;
        ++size_;
        return {&entry_ptr->data.second, true};
      }

      // Track first tombstone.
      if (is_tombstone(table_[pos]) && first_tombstone == capacity_) {
        first_tombstone = pos;
        continue;
      }

      // Check if key already exists (skip tombstones).
      if (!is_tombstone(table_[pos]) && table_[pos].hash == hash &&
          table_[pos].entry->data.first == key) {
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

  // Erase.
  bool erase(const Key &key) {
    size_t hash = make_hash(hash_fn_(key));
    size_t pos = find_slot(key, hash);
    // Key not found.
    if (pos == capacity_)
      return false;
    // Mark as tombstone using sentinel pointer.
    table_[pos].hash = TOMBSTONE_HASH;
    table_[pos].entry = tombstone_ptr();
    ++tombstone_count_;
    --size_;
    // Consider rehashing if too many tombstones accumulate.
    double tombstone_ratio = static_cast<double>(tombstone_count_) / capacity_;
    if (tombstone_ratio > MAX_TOMBSTONE_RATIO && size_ > 0)
      rehash(capacity_);
    return true;
  }
};

} // namespace yhy
