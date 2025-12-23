#include "hybrid_map.hpp"
#include <benchmark/benchmark.h>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

// Integer keys, sequential insert.
static void BM_HybridMap_Insert_Sequential(benchmark::State &state) {
  const int num_elements = state.range(0);
  for (auto _ : state) {
    yhy::HashMap<int, int> map(num_elements);
    for (int i = 0; i < num_elements; ++i)
      map.insert(i, i * 2);
    benchmark::DoNotOptimize(map);
  }
  state.SetItemsProcessed(state.iterations() * num_elements);
}
BENCHMARK(BM_HybridMap_Insert_Sequential)->Range(1 << 10, 1 << 18);

static void BM_StdMap_Insert_Sequential(benchmark::State &state) {
  const int num_elements = state.range(0);
  for (auto _ : state) {
    std::unordered_map<int, int> map;
    map.reserve(num_elements);
    for (int i = 0; i < num_elements; ++i)
      map[i] = i * 2;
    benchmark::DoNotOptimize(map);
  }
  state.SetItemsProcessed(state.iterations() * num_elements);
}
BENCHMARK(BM_StdMap_Insert_Sequential)->Range(1 << 10, 1 << 18);

// Integer keys, random insert.
static void BM_HybridMap_Insert_Random(benchmark::State &state) {
  const int num_elements = state.range(0);
  // Pre-generate random keys.
  std::vector<int> keys(num_elements);
  std::mt19937 gen(42);
  std::uniform_int_distribution<> dis(0, num_elements * 10);
  for (int i = 0; i < num_elements; ++i)
    keys[i] = dis(gen);
  for (auto _ : state) {
    yhy::HashMap<int, int> map(num_elements);
    for (int i = 0; i < num_elements; ++i)
      map.insert(keys[i], i);
    benchmark::DoNotOptimize(map);
  }
  state.SetItemsProcessed(state.iterations() * num_elements);
}
BENCHMARK(BM_HybridMap_Insert_Random)->Range(1 << 10, 1 << 18);

static void BM_StdMap_Insert_Random(benchmark::State &state) {
  const int num_elements = state.range(0);
  // Pre-generate random keys.
  std::vector<int> keys(num_elements);
  std::mt19937 gen(42);
  std::uniform_int_distribution<> dis(0, num_elements * 10);
  for (int i = 0; i < num_elements; ++i)
    keys[i] = dis(gen);
  for (auto _ : state) {
    std::unordered_map<int, int> map;
    map.reserve(num_elements);
    for (int i = 0; i < num_elements; ++i)
      map[keys[i]] = i;
    benchmark::DoNotOptimize(map);
  }
  state.SetItemsProcessed(state.iterations() * num_elements);
}
BENCHMARK(BM_StdMap_Insert_Random)->Range(1 << 10, 1 << 18);

// Integer keys, lookup hit.
static void BM_HybridMap_Lookup_Hit(benchmark::State &state) {
  const int num_elements = state.range(0);
  yhy::HashMap<int, int> map(num_elements);
  for (int i = 0; i < num_elements; ++i)
    map.insert(i, i * 2);
  // Pre-generate lookup keys.
  std::vector<int> lookup_keys(1000);
  std::mt19937 gen(42);
  std::uniform_int_distribution<> dis(0, num_elements - 1);
  for (int i = 0; i < 1000; ++i)
    lookup_keys[i] = dis(gen);
  int idx = 0;
  for (auto _ : state) {
    auto *val = map.find(lookup_keys[idx++ % 1000]);
    benchmark::DoNotOptimize(val);
  }
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_HybridMap_Lookup_Hit)->Range(1 << 10, 1 << 18);

static void BM_StdMap_Lookup_Hit(benchmark::State &state) {
  const int num_elements = state.range(0);
  std::unordered_map<int, int> map;
  map.reserve(num_elements);
  for (int i = 0; i < num_elements; ++i)
    map[i] = i * 2;
  // Pre-generate lookup keys.
  std::vector<int> lookup_keys(1000);
  std::mt19937 gen(42);
  std::uniform_int_distribution<> dis(0, num_elements - 1);
  for (int i = 0; i < 1000; ++i)
    lookup_keys[i] = dis(gen);
  int idx = 0;
  for (auto _ : state) {
    auto it = map.find(lookup_keys[idx++ % 1000]);
    benchmark::DoNotOptimize(it);
  }
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_StdMap_Lookup_Hit)->Range(1 << 10, 1 << 18);

// Integer keys, lookup miss.
static void BM_HybridMap_Lookup_Miss(benchmark::State &state) {
  const int num_elements = state.range(0);
  yhy::HashMap<int, int> map(num_elements);
  for (int i = 0; i < num_elements; ++i)
    map.insert(i, i * 2);
  // Pre-generate lookup keys that do not exist.
  std::vector<int> lookup_keys(1000);
  std::mt19937 gen(42);
  std::uniform_int_distribution<> dis(num_elements, num_elements * 2);
  for (int i = 0; i < 1000; ++i)
    lookup_keys[i] = dis(gen);
  int idx = 0;
  for (auto _ : state) {
    auto *val = map.find(lookup_keys[idx++ % 1000]);
    benchmark::DoNotOptimize(val);
  }
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_HybridMap_Lookup_Miss)->Range(1 << 10, 1 << 18);

static void BM_StdMap_Lookup_Miss(benchmark::State &state) {
  const int num_elements = state.range(0);
  std::unordered_map<int, int> map;
  map.reserve(num_elements);
  for (int i = 0; i < num_elements; ++i)
    map[i] = i * 2;
  // Pre-generate lookup keys that do not exist.
  std::vector<int> lookup_keys(1000);
  std::mt19937 gen(42);
  std::uniform_int_distribution<> dis(num_elements, num_elements * 2);
  for (int i = 0; i < 1000; ++i)
    lookup_keys[i] = dis(gen);
  int idx = 0;
  for (auto _ : state) {
    auto it = map.find(lookup_keys[idx++ % 1000]);
    benchmark::DoNotOptimize(it);
  }
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_StdMap_Lookup_Miss)->Range(1 << 10, 1 << 18);

// String keys, insert and lookup.
static std::vector<std::string> generate_string_keys(int count) {
  std::vector<std::string> keys;
  keys.reserve(count);
  std::mt19937 gen(42);
  std::uniform_int_distribution<> dis(5, 20);
  for (int i = 0; i < count; ++i) {
    int len = dis(gen);
    std::string key;
    key.reserve(len);
    for (int j = 0; j < len; ++j)
      key += 'a' + (gen() % 26);
    keys.push_back(std::move(key));
  }
  return keys;
}

static void BM_HybridMap_String_Insert(benchmark::State &state) {
  const int num_elements = state.range(0);
  auto keys = generate_string_keys(num_elements);
  for (auto _ : state) {
    yhy::HashMap<std::string, int> map(num_elements);
    for (int i = 0; i < num_elements; ++i)
      map.insert(keys[i], i);
    benchmark::DoNotOptimize(map);
  }
  state.SetItemsProcessed(state.iterations() * num_elements);
}
BENCHMARK(BM_HybridMap_String_Insert)->Range(1 << 10, 1 << 16);

static void BM_StdMap_String_Insert(benchmark::State &state) {
  const int num_elements = state.range(0);
  auto keys = generate_string_keys(num_elements);
  for (auto _ : state) {
    std::unordered_map<std::string, int> map;
    map.reserve(num_elements);
    for (int i = 0; i < num_elements; ++i)
      map[keys[i]] = i;
    benchmark::DoNotOptimize(map);
  }
  state.SetItemsProcessed(state.iterations() * num_elements);
}
BENCHMARK(BM_StdMap_String_Insert)->Range(1 << 10, 1 << 16);

static void BM_HybridMap_String_Lookup(benchmark::State &state) {
  const int num_elements = state.range(0);
  auto keys = generate_string_keys(num_elements);
  yhy::HashMap<std::string, int> map(num_elements);
  for (int i = 0; i < num_elements; ++i)
    map.insert(keys[i], i);
  std::mt19937 gen(42);
  std::uniform_int_distribution<> dis(0, num_elements - 1);
  std::vector<std::string> lookup_keys(1000);
  for (int i = 0; i < 1000; ++i)
    lookup_keys[i] = keys[dis(gen)];
  int idx = 0;
  for (auto _ : state) {
    auto *val = map.find(lookup_keys[idx++ % 1000]);
    benchmark::DoNotOptimize(val);
  }
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_HybridMap_String_Lookup)->Range(1 << 10, 1 << 16);

static void BM_StdMap_String_Lookup(benchmark::State &state) {
  const int num_elements = state.range(0);
  auto keys = generate_string_keys(num_elements);
  std::unordered_map<std::string, int> map;
  map.reserve(num_elements);
  for (int i = 0; i < num_elements; ++i)
    map[keys[i]] = i;
  std::mt19937 gen(42);
  std::uniform_int_distribution<> dis(0, num_elements - 1);
  std::vector<std::string> lookup_keys(1000);
  for (int i = 0; i < 1000; ++i)
    lookup_keys[i] = keys[dis(gen)];
  int idx = 0;
  for (auto _ : state) {
    auto it = map.find(lookup_keys[idx++ % 1000]);
    benchmark::DoNotOptimize(it);
  }
  state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_StdMap_String_Lookup)->Range(1 << 10, 1 << 16);

// Mixed operations.
static void BM_HybridMap_Mixed_Ops(benchmark::State &state) {
  const int num_elements = state.range(0);
  std::mt19937 gen(42);
  std::uniform_int_distribution<> dis(0, num_elements - 1);
  for (auto _ : state) {
    yhy::HashMap<int, int> map(num_elements);
    // Insert phase.
    for (int i = 0; i < num_elements; ++i)
      map.insert(i, i * 2);
    // Mixed operations.
    for (int i = 0; i < 1000; ++i) {
      int key = dis(gen);
      auto *val = map.find(key);
      benchmark::DoNotOptimize(val);
      if (i % 10 == 0)
        map.insert(key + num_elements, i);
    }
    benchmark::DoNotOptimize(map);
  }
}
BENCHMARK(BM_HybridMap_Mixed_Ops)->Range(1 << 10, 1 << 16);

static void BM_StdMap_Mixed_Ops(benchmark::State &state) {
  const int num_elements = state.range(0);
  std::mt19937 gen(42);
  std::uniform_int_distribution<> dis(0, num_elements - 1);
  for (auto _ : state) {
    std::unordered_map<int, int> map;
    map.reserve(num_elements * 1.2);
    // Insert phase.
    for (int i = 0; i < num_elements; ++i)
      map[i] = i * 2;
    // Mixed operations.
    for (int i = 0; i < 1000; ++i) {
      int key = dis(gen);
      auto it = map.find(key);
      benchmark::DoNotOptimize(it);
      if (i % 10 == 0)
        map[key + num_elements] = i;
    }
    benchmark::DoNotOptimize(map);
  }
}
BENCHMARK(BM_StdMap_Mixed_Ops)->Range(1 << 10, 1 << 16);

BENCHMARK_MAIN();
