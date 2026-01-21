#pragma once

#include <cstddef>

namespace yhy {

// Constants.
static constexpr size_t INITIAL_CAPACITY = 16;
static constexpr double MAX_LOAD_FACTOR = 0.75;
static constexpr double MAX_TOMBSTONE_RATIO = 0.25;
static constexpr size_t EMPTY_HASH = 0;
static constexpr size_t TOMBSTONE_HASH = 1;

// Find the next power of 2.
static size_t next_power_of_2(size_t n) {
  if (n-- == 0)
    return 1;
  n |= n >> 1;
  n |= n >> 2;
  n |= n >> 4;
  n |= n >> 8;
  n |= n >> 16;
  n |= n >> 32;
  return n + 1;
}

}; // namespace yhy
