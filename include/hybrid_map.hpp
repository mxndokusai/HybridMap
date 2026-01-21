#pragma once

#include "flat_hash_map.hpp"
#include "node_hash_map.hpp"
#include <concepts>

namespace yhy {

template <typename Key, typename Value, typename Hash = std::hash<Key>>
using HashMap = std::conditional_t<
    // Must fit in cache line with hash (8 bytes).
    (sizeof(Key) + sizeof(Value) <= 56) &&
        // Must be nothrow move constructible for safe rehashing.
        std::is_nothrow_move_constructible_v<Key> &&
        std::is_nothrow_move_constructible_v<Value>,
    FlatHashMap<Key, Value, Hash>, NodeHashMap<Key, Value, Hash>>;

}; // namespace yhy
