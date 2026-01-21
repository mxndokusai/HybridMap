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
// class alignas(std::hardware_destructive_interference_size) FlatHashMap {
class FlatHashMap {
private:
  struct alignas(64) Slot {
    size_t hash;
    alignas(alignof(Key)) std::byte key_storage[sizeof(Key)];
    alignas(alignof(Value)) std::byte value_storage[sizeof(Value)];
    Slot() : hash(0) {}

    // Getter pointers to stored objects after construction.
    Key *key_ptr() {
      return std::launder(reinterpret_cast<Key *>(key_storage));
    }
    Value *value_ptr() {
      return std::launder(reinterpret_cast<Value *>(value_storage));
    }
    const Key *key_ptr() const {
      return std::launder(reinterpret_cast<const Key *>(key_storage));
    }
    const Value *value_ptr() const {
      return std::launder(reinterpret_cast<const Value *>(value_storage));
    }
  };
  // Verify slot fits in cache line (assume 64 bytes).
  static_assert(sizeof(Slot) == 64,
                "Slot should fit in a cache line (64 bytes)");
  static_assert(sizeof(Key) + sizeof(Value) + sizeof(size_t) <= 64,
                "Slot contents must fit in 64 bytes");
  std::vector<Slot> table_;
  size_t size_;
  size_t capacity_;
  size_t tombstone_count_;
  Hash hash_fn_;

  // Slot state checks.
  static bool is_empty(const Slot &slot) { return slot.hash == EMPTY_HASH; }
  static bool is_tombstone(const Slot &slot) {
    return slot.hash == TOMBSTONE_HASH;
  }

  // Ensure hash is never 0 or 1. 0 = empty, 1 = tombstone.
  static size_t make_hash(size_t h) {
    return (h == EMPTY_HASH || h == TOMBSTONE_HASH) ? 2 : h;
  }

  // Linear probing for better cache performance.
  size_t probe(size_t index, size_t i) const {
    return (index + i) & (capacity_ - 1);
  }

  // Destroy key value pair in slot.
  void destroy_slot(Slot &slot) {
    if (!is_empty(slot) && !is_tombstone(slot)) {
      std::destroy_at(slot.key_ptr());
      std::destroy_at(slot.value_ptr());
    }
  }

  void rehash(size_t new_capacity) {
    std::vector<Slot> old_table = std::move(table_);
    table_.clear();
    table_.resize(new_capacity);
    capacity_ = new_capacity;
    tombstone_count_ = 0;

    // Reinsert all valid entries.
    for (auto &slot : old_table) {
      if (!is_empty(slot) && !is_tombstone(slot))
        insert_internal(slot.hash, std::move(*slot.key_ptr()),
                        std::move(*slot.value_ptr()));
      destroy_slot(slot);
    }
  }

  // Slightly different from NodeHashMap version.
  template <typename K, typename V>
  void insert_internal(size_t hash, K &&key, V &&value) {
    size_t index = hash & (capacity_ - 1);
    for (size_t i = 0; i < capacity_; ++i) {
      size_t pos = probe(index, i);
      if (is_empty(table_[pos]) || is_tombstone(table_[pos])) {
        if (is_tombstone(table_[pos]))
          --tombstone_count_;
        table_[pos].hash = hash;
        std::construct_at(table_[pos].key_ptr(), std::forward<K>(key));
        std::construct_at(table_[pos].value_ptr(), std::forward<V>(value));
        return;
      }
    }
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
      if (table_[pos].hash == hash && *table_[pos].key_ptr() == key)
        return pos;
    }
    return capacity_;
  }

public:
  // Used for type trait tests.
  using key_type = Key;
  using mapped_type = Value;
  FlatHashMap()
      : size_(0), capacity_(INITIAL_CAPACITY), tombstone_count_(0), hash_fn_() {
    table_.resize(capacity_);
  }

  explicit FlatHashMap(size_t expected_size)
      : size_(0), capacity_(next_power_of_2(
                      static_cast<size_t>(expected_size / MAX_LOAD_FACTOR))),
        tombstone_count_(0), hash_fn_() {
    table_.resize(capacity_);
  }

  ~FlatHashMap() {
    // Destroy all valid entries.
    for (auto &slot : table_)
      destroy_slot(slot);
  }

  // Prevent copying.
  FlatHashMap(const FlatHashMap &) = delete;
  FlatHashMap &operator=(const FlatHashMap &) = delete;

  // Move operations.
  FlatHashMap(FlatHashMap &&other) noexcept
      : table_(std::move(other.table_)), size_(other.size_),
        capacity_(other.capacity_), tombstone_count_(other.tombstone_count_),
        hash_fn_(std::move(other.hash_fn_)) {
    other.size_ = 0;
    other.capacity_ = 0;
    other.tombstone_count_ = 0;
  }

  FlatHashMap &operator=(FlatHashMap &&other) noexcept {
    if (this != &other) {
      // Destroy current contents.
      for (auto &slot : table_)
        destroy_slot(slot);
      table_ = std::move(other.table_);
      size_ = other.size_;
      capacity_ = other.capacity_;
      tombstone_count_ = other.tombstone_count_;
      hash_fn_ = std::move(other.hash_fn_);

      other.size_ = 0;
      other.capacity_ = 0;
      other.tombstone_count_ = 0;
    }
    return *this;
  }

  size_t size() const noexcept { return size_; }
  bool empty() const noexcept { return size_ == 0; }
  size_t capacity() const noexcept { return capacity_; }

  void clear() {
    for (auto &slot : table_) {
      destroy_slot(slot);
      slot.hash = EMPTY_HASH;
    }
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
    size_t first_tombstone = capacity_;

    for (size_t i = 0; i < capacity_; ++i) {
      size_t pos = probe(index, i);
      // Empty slot, use tombstone if found, otherwise use this.
      if (is_empty(table_[pos])) {
        size_t insert_pos =
            (first_tombstone != capacity_) ? first_tombstone : pos;
        if (first_tombstone != capacity_)
          --tombstone_count_;
        table_[insert_pos].hash = hash;
        std::construct_at(table_[insert_pos].key_ptr(), std::forward<K>(key));
        std::construct_at(table_[insert_pos].value_ptr(),
                          std::forward<V>(value));
        ++size_;
        return {table_[insert_pos].value_ptr(), true};
      }

      // Track first tombstone.
      if (is_tombstone(table_[pos]) && first_tombstone == capacity_) {
        first_tombstone = pos;
        continue;
      }

      // Key exists, update.
      if (!is_tombstone(table_[pos]) && table_[pos].hash == hash &&
          *table_[pos].key_ptr() == key) {
        *table_[pos].value_ptr() = std::forward<V>(value);
        return {table_[pos].value_ptr(), false};
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
    return table_[pos].value_ptr();
  }

  const Value *find(const Key &key) const {
    size_t hash = make_hash(hash_fn_(key));
    size_t pos = find_slot(key, hash);
    if (pos == capacity_)
      return nullptr;
    return table_[pos].value_ptr();
  }

  Value &operator[](const Key &key) {
    size_t hash = make_hash(hash_fn_(key));
    size_t pos = find_slot(key, hash);
    if (pos != capacity_)
      return *table_[pos].value_ptr();
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
    if (pos == capacity_)
      return false;

    // Destroy objects.
    destroy_slot(table_[pos]);
    table_[pos].hash = TOMBSTONE_HASH;
    ++tombstone_count_;
    --size_;

    // Consider rehashing if too many tombstones.
    double tombstone_ratio = static_cast<double>(tombstone_count_) / capacity_;
    if (tombstone_ratio > MAX_TOMBSTONE_RATIO && size_ > 0)
      rehash(capacity_);
    return true;
  }
};

} // namespace yhy
