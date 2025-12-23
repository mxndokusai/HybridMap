#include "hybrid_map.hpp"
#include <algorithm>
#include <benchmark/benchmark.h>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

// ============================================================================
// Benchmark 1: Construction & Destruction
// ============================================================================
// Measures how fast the hashmap can be constructed and destructed.
// Some maps perform lazy initialization (allocate on first insert),
// others immediately initialize their data structures.

static void BM_HybridMap_ConstructDestruct(benchmark::State &state) {
  size_t result = 0;

  for (auto _ : state) {
    yhy::HashMap<int, int> map;
    result += map.size();
  }

  benchmark::DoNotOptimize(result);
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_HybridMap_ConstructDestruct);

static void BM_StdMap_ConstructDestruct(benchmark::State &state) {
  size_t result = 0;

  for (auto _ : state) {
    std::unordered_map<int, int> map;
    result += map.size();
  }

  benchmark::DoNotOptimize(result);
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_StdMap_ConstructDestruct);

// ============================================================================
// Benchmark 2: Construction + Insert 1 Element + Destruction
// ============================================================================
// Same as above, but inserts a single element.
// Maps with lazy initialization are now forced to actually allocate storage.
// Hash function is called once to find a slot.

static void BM_HybridMap_ConstructInsert1Destruct(benchmark::State &state) {
  size_t result = 0;
  int n = 0;

  for (auto _ : state) {
    yhy::HashMap<int, int> map;
    map[n++]; // Insert single element
    result += map.size();
  }

  benchmark::DoNotOptimize(result);
  // Verify result should equal number of iterations
  if (result != state.iterations()) {
    state.SkipWithError("Result mismatch!");
  }
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_HybridMap_ConstructInsert1Destruct);

static void BM_StdMap_ConstructInsert1Destruct(benchmark::State &state) {
  size_t result = 0;
  int n = 0;

  for (auto _ : state) {
    std::unordered_map<int, int> map;
    map[n++]; // Insert single element
    result += map.size();
  }

  benchmark::DoNotOptimize(result);
  // Verify result should equal number of iterations
  if (result != state.iterations()) {
    state.SkipWithError("Result mismatch!");
  }
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_StdMap_ConstructInsert1Destruct);

// ============================================================================
// SFC64 Random Number Generator (Fast & High Quality)
// ============================================================================
// Extremely fast and high quality RNG as used in the reference benchmarks

class sfc64 {
public:
  using result_type = uint64_t;

  sfc64() : sfc64(0) {}
  explicit sfc64(uint64_t seed) {
    state_[0] = seed;
    state_[1] = seed;
    state_[2] = seed;
    counter_ = 1;
    for (int i = 0; i < 12; ++i) {
      operator()();
    }
  }

  uint64_t operator()() {
    uint64_t tmp = state_[0] + state_[1] + counter_++;
    state_[0] = state_[1] ^ (state_[1] >> 11);
    state_[1] = state_[2] + (state_[2] << 3);
    state_[2] = ((state_[2] << 24) | (state_[2] >> 40)) + tmp;
    return tmp;
  }

  static constexpr uint64_t min() { return 0; }
  static constexpr uint64_t max() { return UINT64_MAX; }

private:
  uint64_t state_[3];
  uint64_t counter_;
};

// ============================================================================
// Benchmark 3: Insert and Erase 100M int
// ============================================================================
// This benchmark tests:
// 1. Insert 100M random ints
// 2. Clear all entries
// 3. Reinsert 100M random ints
// 4. Remove all entries one by one
// 5. Destruct empty map
//
// NOTE: 100M int-int pairs = 1526 MB minimum
// Reduced to 10M for faster testing

static void BM_HybridMap_InsertClearReinsertErase(benchmark::State &state) {
  const size_t n = state.range(0);

  for (auto _ : state) {
    state.PauseTiming();
    sfc64 rng(123456);
    std::vector<int> keys(n);
    for (size_t i = 0; i < n; ++i) {
      keys[i] = static_cast<int>(rng());
    }
    state.ResumeTiming();

    yhy::HashMap<int, int> map;

    // 1. Insert n random ints
    for (size_t i = 0; i < n; ++i) {
      map.insert(keys[i], static_cast<int>(i));
    }
    benchmark::DoNotOptimize(map.size());

    // 2. Clear all entries
    map.clear();
    benchmark::DoNotOptimize(map.size());

    // 3. Reinsert n random ints (test memory reuse)
    for (size_t i = 0; i < n; ++i) {
      map.insert(keys[i], static_cast<int>(i));
    }
    benchmark::DoNotOptimize(map.size());

    // 4. Remove all entries one by one
    for (size_t i = 0; i < n; ++i) {
      map.erase(keys[i]);
    }
    benchmark::DoNotOptimize(map.size());

    // 5. Destruct (handled automatically)
  }

  state.SetItemsProcessed(state.iterations() * n * 4); // 4 operations per key
}
BENCHMARK(BM_HybridMap_InsertClearReinsertErase)
    ->Arg(1000)
    ->Arg(10000)
    ->Arg(100000)
    ->Arg(1000000);

static void BM_StdMap_InsertClearReinsertErase(benchmark::State &state) {
  const size_t n = state.range(0);

  for (auto _ : state) {
    state.PauseTiming();
    sfc64 rng(123456);
    std::vector<int> keys(n);
    for (size_t i = 0; i < n; ++i) {
      keys[i] = static_cast<int>(rng());
    }
    state.ResumeTiming();

    std::unordered_map<int, int> map;

    // 1. Insert n random ints
    for (size_t i = 0; i < n; ++i) {
      map[keys[i]] = static_cast<int>(i);
    }
    benchmark::DoNotOptimize(map.size());

    // 2. Clear all entries
    map.clear();
    benchmark::DoNotOptimize(map.size());

    // 3. Reinsert n random ints
    for (size_t i = 0; i < n; ++i) {
      map[keys[i]] = static_cast<int>(i);
    }
    benchmark::DoNotOptimize(map.size());

    // 4. Remove all entries one by one
    for (size_t i = 0; i < n; ++i) {
      map.erase(keys[i]);
    }
    benchmark::DoNotOptimize(map.size());
  }

  state.SetItemsProcessed(state.iterations() * n * 4);
}
BENCHMARK(BM_StdMap_InsertClearReinsertErase)
    ->Arg(1000)
    ->Arg(10000)
    ->Arg(100000)
    ->Arg(1000000);

// ============================================================================
// Benchmark 4: Insert and Access with Varying Probability
// ============================================================================
// Adapted from attractivechaos' benchmark
// Different max_rng creates different access patterns:
// - 5% distinct: many duplicates, mostly accesses
// - 25% distinct: more inserts, less modifications
// - 50% distinct: balanced
// - 100% distinct: mostly insertions

static void BM_HybridMap_InsertAccess(benchmark::State &state) {
  const size_t n = 50'000'000;
  const size_t max_rng = state.range(0); // Controls distinctness

  for (auto _ : state) {
    state.PauseTiming();
    sfc64 rng(987654);
    state.ResumeTiming();

    yhy::HashMap<int, int> map;
    size_t checksum = 0;

    for (size_t i = 0; i < n; ++i) {
      int key = static_cast<int>(rng() % max_rng);
      checksum += ++map[key];
    }

    benchmark::DoNotOptimize(checksum);
    benchmark::DoNotOptimize(map);
  }

  state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(BM_HybridMap_InsertAccess)
    ->Arg(250'000)        // 5% distinct
    ->Arg(12'500'000)     // 25% distinct
    ->Arg(25'000'000)     // 50% distinct
    ->Arg(2'147'483'647); // 100% distinct (full int range)

static void BM_StdMap_InsertAccess(benchmark::State &state) {
  const size_t n = 50'000'000;
  const size_t max_rng = state.range(0);

  for (auto _ : state) {
    state.PauseTiming();
    sfc64 rng(987654);
    state.ResumeTiming();

    std::unordered_map<int, int> map;
    size_t checksum = 0;

    for (size_t i = 0; i < n; ++i) {
      int key = static_cast<int>(rng() % max_rng);
      checksum += ++map[key];
    }

    benchmark::DoNotOptimize(checksum);
    benchmark::DoNotOptimize(map);
  }

  state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(BM_StdMap_InsertAccess)
    ->Arg(250'000)
    ->Arg(12'500'000)
    ->Arg(25'000'000)
    ->Arg(2'147'483'647);

// ============================================================================
// Benchmark 5: Insert and Erase uint64_t
// ============================================================================
// Each iteration: insert random element, erase random element
// Map reaches equilibrium size based on bitMask

static void BM_HybridMap_InsertEraseU64(benchmark::State &state) {
  const size_t n = 50'000'000;
  const uint64_t bitMask = state.range(0);

  for (auto _ : state) {
    state.PauseTiming();
    sfc64 rng(111222);
    state.ResumeTiming();

    yhy::HashMap<uint64_t, uint64_t> map;

    for (size_t i = 0; i < n; ++i) {
      map.insert(rng() & bitMask, i);
      map.erase(rng() & bitMask);
    }

    benchmark::DoNotOptimize(map);
  }

  state.SetItemsProcessed(state.iterations() * n * 2); // insert + erase
}
BENCHMARK(BM_HybridMap_InsertEraseU64)
    ->Arg(0x9000000000000108)  // 4 bits set, ~16 distinct, ~8 entries
    ->Arg(0x9000003000000508)  // 8 bits set, ~256 distinct, ~128 entries
    ->Arg(0x9000023010000D09)  // 12 bits set, ~4096 distinct, ~2048 entries
    ->Arg(0x9000023011000F29)  // 16 bits set, ~65k distinct, ~32k entries
    ->Arg(0xD060023091001F29)  // 20 bits set, ~1M distinct, ~524k entries
    ->Arg(0xD070123095005F2B); // 24 bits set, ~16.8M distinct, ~8.4M entries

static void BM_StdMap_InsertEraseU64(benchmark::State &state) {
  const size_t n = 50'000'000;
  const uint64_t bitMask = state.range(0);

  for (auto _ : state) {
    state.PauseTiming();
    sfc64 rng(111222);
    state.ResumeTiming();

    std::unordered_map<uint64_t, uint64_t> map;

    for (size_t i = 0; i < n; ++i) {
      map[rng() & bitMask] = i;
      map.erase(rng() & bitMask);
    }

    benchmark::DoNotOptimize(map);
  }

  state.SetItemsProcessed(state.iterations() * n * 2);
}
BENCHMARK(BM_StdMap_InsertEraseU64)
    ->Arg(0x9000000000000108)
    ->Arg(0x9000003000000508)
    ->Arg(0x9000023010000D09)
    ->Arg(0x9000023011000F29)
    ->Arg(0xD060023091001F29)
    ->Arg(0xD070123095005F2B);

// ============================================================================
// Benchmark 6: Insert and Erase std::string
// ============================================================================
// Continuously insert and erase random strings of varying lengths

static void randomize_string(std::string &str, sfc64 &rng) {
  const size_t len = str.length();
  // Only modify last few bytes for realistic comparison behavior
  const size_t modify_bytes = std::min(len, size_t(8));
  for (size_t i = len - modify_bytes; i < len; ++i) {
    str[i] = 'a' + (rng() % 26);
  }
}

static void BM_HybridMap_InsertEraseString(benchmark::State &state) {
  const size_t string_length = state.range(0);

  // Adapt iterations based on string length
  size_t max_n;
  if (string_length <= 13) {
    max_n = 20'000'000;
  } else if (string_length <= 100) {
    max_n = 12'000'000;
  } else {
    max_n = 6'000'000;
  }

  for (auto _ : state) {
    state.PauseTiming();
    sfc64 rng(333444);
    std::string str(string_length, 'x');
    state.ResumeTiming();

    yhy::HashMap<std::string, std::string> map;
    size_t verifier = 0;

    for (size_t i = 0; i < max_n; ++i) {
      // Insert
      randomize_string(str, rng);
      map[str];

      // Find and erase
      randomize_string(str, rng);
      auto *val = map.find(str);
      if (val != nullptr) {
        ++verifier;
        map.erase(str);
      }
    }

    benchmark::DoNotOptimize(verifier);
    benchmark::DoNotOptimize(map);
  }

  state.SetItemsProcessed(state.iterations() * max_n * 2);
}
BENCHMARK(BM_HybridMap_InsertEraseString)
    ->Arg(7)
    ->Arg(8)
    ->Arg(13)
    ->Arg(100)
    ->Arg(1000);

static void BM_StdMap_InsertEraseString(benchmark::State &state) {
  const size_t string_length = state.range(0);

  size_t max_n;
  if (string_length <= 13) {
    max_n = 20'000'000;
  } else if (string_length <= 100) {
    max_n = 12'000'000;
  } else {
    max_n = 6'000'000;
  }

  for (auto _ : state) {
    state.PauseTiming();
    sfc64 rng(333444);
    std::string str(string_length, 'x');
    state.ResumeTiming();

    std::unordered_map<std::string, std::string> map;
    size_t verifier = 0;

    for (size_t i = 0; i < max_n; ++i) {
      // Insert
      randomize_string(str, rng);
      map[str];

      // Find and erase
      randomize_string(str, rng);
      auto it = map.find(str);
      if (it != map.end()) {
        ++verifier;
        map.erase(it);
      }
    }

    benchmark::DoNotOptimize(verifier);
    benchmark::DoNotOptimize(map);
  }

  state.SetItemsProcessed(state.iterations() * max_n * 2);
}
BENCHMARK(BM_StdMap_InsertEraseString)
    ->Arg(7)
    ->Arg(8)
    ->Arg(13)
    ->Arg(100)
    ->Arg(1000);

// ============================================================================
// Additional Helper Benchmarks
// ============================================================================

// Benchmark just the insert phase
static void BM_HybridMap_InsertOnly(benchmark::State &state) {
  const size_t n = state.range(0);

  for (auto _ : state) {
    state.PauseTiming();
    sfc64 rng(555666);
    std::vector<int> keys(n);
    for (size_t i = 0; i < n; ++i) {
      keys[i] = static_cast<int>(rng());
    }
    state.ResumeTiming();

    yhy::HashMap<int, int> map;
    for (size_t i = 0; i < n; ++i) {
      map.insert(keys[i], static_cast<int>(i));
    }

    benchmark::DoNotOptimize(map);
  }

  state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(BM_HybridMap_InsertOnly)->Range(1 << 10, 1 << 20);

static void BM_StdMap_InsertOnly(benchmark::State &state) {
  const size_t n = state.range(0);

  for (auto _ : state) {
    state.PauseTiming();
    sfc64 rng(555666);
    std::vector<int> keys(n);
    for (size_t i = 0; i < n; ++i) {
      keys[i] = static_cast<int>(rng());
    }
    state.ResumeTiming();

    std::unordered_map<int, int> map;
    for (size_t i = 0; i < n; ++i) {
      map[keys[i]] = static_cast<int>(i);
    }

    benchmark::DoNotOptimize(map);
  }

  state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(BM_StdMap_InsertOnly)->Range(1 << 10, 1 << 20);

// Benchmark just the erase phase
static void BM_HybridMap_EraseOnly(benchmark::State &state) {
  const size_t n = state.range(0);

  for (auto _ : state) {
    state.PauseTiming();
    sfc64 rng(777888);
    std::vector<int> keys(n);
    yhy::HashMap<int, int> map;
    for (size_t i = 0; i < n; ++i) {
      keys[i] = static_cast<int>(rng());
      map.insert(keys[i], static_cast<int>(i));
    }
    state.ResumeTiming();

    for (size_t i = 0; i < n; ++i) {
      map.erase(keys[i]);
    }

    benchmark::DoNotOptimize(map);
  }

  state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(BM_HybridMap_EraseOnly)->Range(1 << 10, 1 << 20);

static void BM_StdMap_EraseOnly(benchmark::State &state) {
  const size_t n = state.range(0);

  for (auto _ : state) {
    state.PauseTiming();
    sfc64 rng(777888);
    std::vector<int> keys(n);
    std::unordered_map<int, int> map;
    for (size_t i = 0; i < n; ++i) {
      keys[i] = static_cast<int>(rng());
      map[keys[i]] = static_cast<int>(i);
    }
    state.ResumeTiming();

    for (size_t i = 0; i < n; ++i) {
      map.erase(keys[i]);
    }

    benchmark::DoNotOptimize(map);
  }

  state.SetItemsProcessed(state.iterations() * n);
}
BENCHMARK(BM_StdMap_EraseOnly)->Range(1 << 10, 1 << 20);

// ============================================================================
// Benchmark 7: Find 2000 uint64_t
// ============================================================================
// Tests find performance with:
// - Different success probabilities (0%, 25%, 50%, 75%, 100%)
// - Different map sizes (growing from 4 to 2000 elements)
// - Different bitmasks (not biased towards small numbers)
//
// Pattern: Insert 4 entries, perform many lookups, repeat until full

template <typename Map>
static void BM_Find_2000_uint64(benchmark::State &state) {
  const size_t max_size = 2000;
  const size_t lookups_per_insert =
      2'000'000 / (max_size / 4);             // 500k lookups per insert batch
  const int success_percent = state.range(0); // 0, 25, 50, 75, 100

  for (auto _ : state) {
    state.PauseTiming();

    // Two RNGs: one for inserts, one for lookups
    sfc64 rng_insert(12345);
    sfc64 rng_lookup(success_percent == 100 ? 12345 : 67890);

    Map map;
    std::vector<uint64_t> insert_keys(4);
    std::vector<uint64_t> lookup_keys(lookups_per_insert);

    size_t checksum = 0;
    state.ResumeTiming();

    // Insert 4 elements at a time until we reach max_size
    for (size_t size = 0; size < max_size; size += 4) {
      // Generate 4 keys to insert
      for (size_t i = 0; i < 4; ++i) {
        // Use different seeds to control success rate
        if (success_percent == 100 ||
            (success_percent > 0 && (i * 100 / 4) < success_percent)) {
          // Key will be found
          insert_keys[i] = rng_insert();
        } else {
          // Key won't be found (different RNG)
          insert_keys[i] = rng_insert();
        }
      }

      // Shuffle for randomness
      std::shuffle(insert_keys.begin(), insert_keys.end(), rng_insert);

      // Insert the 4 keys
      for (const auto &key : insert_keys) {
        map[key] = key;
      }

      // Perform lookups
      for (size_t i = 0; i < lookups_per_insert; ++i) {
        uint64_t lookup_key = rng_lookup();
        auto it = map.find(lookup_key);
        if (it != nullptr) { // For hybrid map
          checksum += *it;
        }
      }
    }

    benchmark::DoNotOptimize(checksum);
    benchmark::DoNotOptimize(map);
  }

  state.SetItemsProcessed(state.iterations() * 2'000'000); // Total lookups
}

static void BM_HybridMap_Find_2000_uint64(benchmark::State &state) {
  BM_Find_2000_uint64<yhy::HashMap<uint64_t, uint64_t>>(state);
}

static void BM_StdMap_Find_2000_uint64(benchmark::State &state) {
  const size_t max_size = 2000;
  const size_t lookups_per_insert = 2'000'000 / (max_size / 4);
  const int success_percent = state.range(0);

  for (auto _ : state) {
    state.PauseTiming();

    sfc64 rng_insert(12345);
    sfc64 rng_lookup(success_percent == 100 ? 12345 : 67890);

    std::unordered_map<uint64_t, uint64_t> map;
    std::vector<uint64_t> insert_keys(4);

    size_t checksum = 0;
    state.ResumeTiming();

    for (size_t size = 0; size < max_size; size += 4) {
      for (size_t i = 0; i < 4; ++i) {
        if (success_percent == 100 ||
            (success_percent > 0 && (i * 100 / 4) < success_percent)) {
          insert_keys[i] = rng_insert();
        } else {
          insert_keys[i] = rng_insert();
        }
      }

      std::shuffle(insert_keys.begin(), insert_keys.end(), rng_insert);

      for (const auto &key : insert_keys) {
        map[key] = key;
      }

      for (size_t i = 0; i < lookups_per_insert; ++i) {
        uint64_t lookup_key = rng_lookup();
        auto it = map.find(lookup_key);
        if (it != map.end()) {
          checksum += it->second;
        }
      }
    }

    benchmark::DoNotOptimize(checksum);
    benchmark::DoNotOptimize(map);
  }

  state.SetItemsProcessed(state.iterations() * 2'000'000);
}

BENCHMARK(BM_HybridMap_Find_2000_uint64)
    ->Arg(0)    // 0% success
    ->Arg(25)   // 25% success
    ->Arg(50)   // 50% success
    ->Arg(75)   // 75% success
    ->Arg(100); // 100% success

BENCHMARK(BM_StdMap_Find_2000_uint64)
    ->Arg(0)
    ->Arg(25)
    ->Arg(50)
    ->Arg(75)
    ->Arg(100);

// ============================================================================
// Benchmark 8: Find 500k uint64_t
// ============================================================================
// Same as above but with 500k elements
// 4000 lookups per 4 inserts

template <typename Map>
static void BM_Find_500k_uint64(benchmark::State &state) {
  const size_t max_size = 500'000;
  const size_t lookups_per_batch = 4000;
  const int success_percent = state.range(0);

  for (auto _ : state) {
    state.PauseTiming();

    sfc64 rng_insert(23456);
    sfc64 rng_lookup(success_percent == 100 ? 23456 : 78901);

    Map map;
    std::vector<uint64_t> insert_keys(4);

    size_t checksum = 0;
    state.ResumeTiming();

    for (size_t size = 0; size < max_size; size += 4) {
      for (size_t i = 0; i < 4; ++i) {
        if (success_percent == 100 ||
            (success_percent > 0 && (i * 100 / 4) < success_percent)) {
          insert_keys[i] = rng_insert();
        } else {
          insert_keys[i] = rng_insert();
        }
      }

      std::shuffle(insert_keys.begin(), insert_keys.end(), rng_insert);

      for (const auto &key : insert_keys) {
        map[key] = key;
      }

      for (size_t i = 0; i < lookups_per_batch; ++i) {
        uint64_t lookup_key = rng_lookup();
        auto it = map.find(lookup_key);
        if (it != nullptr) {
          checksum += *it;
        }
      }
    }

    benchmark::DoNotOptimize(checksum);
    benchmark::DoNotOptimize(map);
  }

  state.SetItemsProcessed(state.iterations() * (max_size / 4) *
                          lookups_per_batch);
}

static void BM_HybridMap_Find_500k_uint64(benchmark::State &state) {
  BM_Find_500k_uint64<yhy::HashMap<uint64_t, uint64_t>>(state);
}

static void BM_StdMap_Find_500k_uint64(benchmark::State &state) {
  const size_t max_size = 500'000;
  const size_t lookups_per_batch = 4000;
  const int success_percent = state.range(0);

  for (auto _ : state) {
    state.PauseTiming();

    sfc64 rng_insert(23456);
    sfc64 rng_lookup(success_percent == 100 ? 23456 : 78901);

    std::unordered_map<uint64_t, uint64_t> map;
    std::vector<uint64_t> insert_keys(4);

    size_t checksum = 0;
    state.ResumeTiming();

    for (size_t size = 0; size < max_size; size += 4) {
      for (size_t i = 0; i < 4; ++i) {
        if (success_percent == 100 ||
            (success_percent > 0 && (i * 100 / 4) < success_percent)) {
          insert_keys[i] = rng_insert();
        } else {
          insert_keys[i] = rng_insert();
        }
      }

      std::shuffle(insert_keys.begin(), insert_keys.end(), rng_insert);

      for (const auto &key : insert_keys) {
        map[key] = key;
      }

      for (size_t i = 0; i < lookups_per_batch; ++i) {
        uint64_t lookup_key = rng_lookup();
        auto it = map.find(lookup_key);
        if (it != map.end()) {
          checksum += it->second;
        }
      }
    }

    benchmark::DoNotOptimize(checksum);
    benchmark::DoNotOptimize(map);
  }

  state.SetItemsProcessed(state.iterations() * (max_size / 4) *
                          lookups_per_batch);
}

BENCHMARK(BM_HybridMap_Find_500k_uint64)
    ->Arg(0)
    ->Arg(25)
    ->Arg(50)
    ->Arg(75)
    ->Arg(100);

BENCHMARK(BM_StdMap_Find_500k_uint64)
    ->Arg(0)
    ->Arg(25)
    ->Arg(50)
    ->Arg(75)
    ->Arg(100);

// ============================================================================
// Benchmark 9: Find 100k std::string (100 bytes)
// ============================================================================
// 4000 lookups per 4 inserts, 100 byte strings

static std::string make_random_string(sfc64 &rng, size_t length) {
  std::string str(length, 'x');
  // Randomize last 8 bytes for variety
  size_t start = length > 8 ? length - 8 : 0;
  for (size_t i = start; i < length; ++i) {
    str[i] = 'a' + (rng() % 26);
  }
  return str;
}

template <typename Map>
static void BM_Find_100k_string(benchmark::State &state) {
  const size_t max_size = 100'000;
  const size_t lookups_per_batch = 4000;
  const size_t string_length = 100;
  const int success_percent = state.range(0);

  for (auto _ : state) {
    state.PauseTiming();

    sfc64 rng_insert(34567);
    sfc64 rng_lookup(success_percent == 100 ? 34567 : 89012);

    Map map;
    std::vector<std::string> insert_keys(4);

    size_t checksum = 0;
    state.ResumeTiming();

    for (size_t size = 0; size < max_size; size += 4) {
      for (size_t i = 0; i < 4; ++i) {
        if (success_percent == 100 ||
            (success_percent > 0 && (i * 100 / 4) < success_percent)) {
          insert_keys[i] = make_random_string(rng_insert, string_length);
        } else {
          insert_keys[i] = make_random_string(rng_insert, string_length);
        }
      }

      std::shuffle(insert_keys.begin(), insert_keys.end(), rng_insert);

      for (const auto &key : insert_keys) {
        map[key] = key;
      }

      for (size_t i = 0; i < lookups_per_batch; ++i) {
        std::string lookup_key = make_random_string(rng_lookup, string_length);
        auto it = map.find(lookup_key);
        if (it != nullptr) {
          checksum += it->length();
        }
      }
    }

    benchmark::DoNotOptimize(checksum);
    benchmark::DoNotOptimize(map);
  }

  state.SetItemsProcessed(state.iterations() * (max_size / 4) *
                          lookups_per_batch);
}

static void BM_HybridMap_Find_100k_string(benchmark::State &state) {
  BM_Find_100k_string<yhy::HashMap<std::string, std::string>>(state);
}

static void BM_StdMap_Find_100k_string(benchmark::State &state) {
  const size_t max_size = 100'000;
  const size_t lookups_per_batch = 4000;
  const size_t string_length = 100;
  const int success_percent = state.range(0);

  for (auto _ : state) {
    state.PauseTiming();

    sfc64 rng_insert(34567);
    sfc64 rng_lookup(success_percent == 100 ? 34567 : 89012);

    std::unordered_map<std::string, std::string> map;
    std::vector<std::string> insert_keys(4);

    size_t checksum = 0;
    state.ResumeTiming();

    for (size_t size = 0; size < max_size; size += 4) {
      for (size_t i = 0; i < 4; ++i) {
        if (success_percent == 100 ||
            (success_percent > 0 && (i * 100 / 4) < success_percent)) {
          insert_keys[i] = make_random_string(rng_insert, string_length);
        } else {
          insert_keys[i] = make_random_string(rng_insert, string_length);
        }
      }

      std::shuffle(insert_keys.begin(), insert_keys.end(), rng_insert);

      for (const auto &key : insert_keys) {
        map[key] = key;
      }

      for (size_t i = 0; i < lookups_per_batch; ++i) {
        std::string lookup_key = make_random_string(rng_lookup, string_length);
        auto it = map.find(lookup_key);
        if (it != map.end()) {
          checksum += it->second.length();
        }
      }
    }

    benchmark::DoNotOptimize(checksum);
    benchmark::DoNotOptimize(map);
  }

  state.SetItemsProcessed(state.iterations() * (max_size / 4) *
                          lookups_per_batch);
}

BENCHMARK(BM_HybridMap_Find_100k_string)
    ->Arg(0)
    ->Arg(25)
    ->Arg(50)
    ->Arg(75)
    ->Arg(100);

BENCHMARK(BM_StdMap_Find_100k_string)
    ->Arg(0)
    ->Arg(25)
    ->Arg(50)
    ->Arg(75)
    ->Arg(100);

// ============================================================================
// Benchmark 10: Find 1M std::string (13 bytes)
// ============================================================================
// 800 lookups per 4 inserts, 13 byte strings

template <typename Map> static void BM_Find_1M_string(benchmark::State &state) {
  const size_t max_size = 1'000'000;
  const size_t lookups_per_batch = 800;
  const size_t string_length = 13;
  const int success_percent = state.range(0);

  for (auto _ : state) {
    state.PauseTiming();

    sfc64 rng_insert(45678);
    sfc64 rng_lookup(success_percent == 100 ? 45678 : 90123);

    Map map;
    std::vector<std::string> insert_keys(4);

    size_t checksum = 0;
    state.ResumeTiming();

    for (size_t size = 0; size < max_size; size += 4) {
      for (size_t i = 0; i < 4; ++i) {
        if (success_percent == 100 ||
            (success_percent > 0 && (i * 100 / 4) < success_percent)) {
          insert_keys[i] = make_random_string(rng_insert, string_length);
        } else {
          insert_keys[i] = make_random_string(rng_insert, string_length);
        }
      }

      std::shuffle(insert_keys.begin(), insert_keys.end(), rng_insert);

      for (const auto &key : insert_keys) {
        map[key] = key;
      }

      for (size_t i = 0; i < lookups_per_batch; ++i) {
        std::string lookup_key = make_random_string(rng_lookup, string_length);
        auto it = map.find(lookup_key);
        if (it != nullptr) {
          checksum += it->length();
        }
      }
    }

    benchmark::DoNotOptimize(checksum);
    benchmark::DoNotOptimize(map);
  }

  state.SetItemsProcessed(state.iterations() * (max_size / 4) *
                          lookups_per_batch);
}

static void BM_HybridMap_Find_1M_string(benchmark::State &state) {
  BM_Find_1M_string<yhy::HashMap<std::string, std::string>>(state);
}

static void BM_StdMap_Find_1M_string(benchmark::State &state) {
  const size_t max_size = 1'000'000;
  const size_t lookups_per_batch = 800;
  const size_t string_length = 13;
  const int success_percent = state.range(0);

  for (auto _ : state) {
    state.PauseTiming();

    sfc64 rng_insert(45678);
    sfc64 rng_lookup(success_percent == 100 ? 45678 : 90123);

    std::unordered_map<std::string, std::string> map;
    std::vector<std::string> insert_keys(4);

    size_t checksum = 0;
    state.ResumeTiming();

    for (size_t size = 0; size < max_size; size += 4) {
      for (size_t i = 0; i < 4; ++i) {
        if (success_percent == 100 ||
            (success_percent > 0 && (i * 100 / 4) < success_percent)) {
          insert_keys[i] = make_random_string(rng_insert, string_length);
        } else {
          insert_keys[i] = make_random_string(rng_insert, string_length);
        }
      }

      std::shuffle(insert_keys.begin(), insert_keys.end(), rng_insert);

      for (const auto &key : insert_keys) {
        map[key] = key;
      }

      for (size_t i = 0; i < lookups_per_batch; ++i) {
        std::string lookup_key = make_random_string(rng_lookup, string_length);
        auto it = map.find(lookup_key);
        if (it != map.end()) {
          checksum += it->second.length();
        }
      }
    }

    benchmark::DoNotOptimize(checksum);
    benchmark::DoNotOptimize(map);
  }

  state.SetItemsProcessed(state.iterations() * (max_size / 4) *
                          lookups_per_batch);
}

BENCHMARK(BM_HybridMap_Find_1M_string)
    ->Arg(0)
    ->Arg(25)
    ->Arg(50)
    ->Arg(75)
    ->Arg(100);

BENCHMARK(BM_StdMap_Find_1M_string)
    ->Arg(0)
    ->Arg(25)
    ->Arg(50)
    ->Arg(75)
    ->Arg(100);

BENCHMARK_MAIN();
