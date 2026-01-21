#include "hybrid_map.hpp"
#include <algorithm>
#include <array>
#include <gtest/gtest.h>
#include <numeric>
#include <random>
#include <string>
#include <unordered_set>
#include <vector>

using namespace yhy;

// Basic operations.
TEST(HybridMapTest, DefaultConstruction) {
  yhy::HashMap<int, int> map;
  EXPECT_EQ(map.size(), 0);
  EXPECT_TRUE(map.empty());
  EXPECT_GT(map.capacity(), 0);
}

TEST(HybridMapTest, ConstructionWithCapacity) {
  yhy::HashMap<int, int> map(1000);
  EXPECT_EQ(map.size(), 0);
  EXPECT_TRUE(map.empty());
  EXPECT_GE(map.capacity(), 1000);
}

TEST(HybridMapTest, InsertSingleElement) {
  yhy::HashMap<int, std::string> map;
  auto result = map.insert(42, "answer");
  EXPECT_NE(result.first, nullptr);
  EXPECT_TRUE(result.second);
  EXPECT_EQ(*result.first, "answer");
  EXPECT_EQ(map.size(), 1);
}

TEST(HybridMapTest, InsertMultipleElements) {
  yhy::HashMap<int, int> map;
  for (int i = 0; i < 100; ++i) {
    auto result = map.insert(i, i * 2);
    EXPECT_TRUE(result.second);
    EXPECT_EQ(*result.first, i * 2);
  }
  EXPECT_EQ(map.size(), 100);
}

TEST(HybridMapTest, InsertDuplicate) {
  yhy::HashMap<int, std::string> map;
  auto result1 = map.insert(42, "first");
  EXPECT_TRUE(result1.second);
  EXPECT_EQ(*result1.first, "first");
  auto result2 = map.insert(42, "second");
  EXPECT_FALSE(result2.second);
  EXPECT_EQ(*result2.first, "second");
  EXPECT_EQ(map.size(), 1);
}

// Lookup tests.
TEST(HybridMapTest, FindExistingKey) {
  yhy::HashMap<int, std::string> map;
  map.insert(42, "answer");
  auto *val = map.find(42);
  ASSERT_NE(val, nullptr);
  EXPECT_EQ(*val, "answer");
}

TEST(HybridMapTest, FindNonExistentKey) {
  yhy::HashMap<int, std::string> map;
  map.insert(42, "answer");
  auto *val = map.find(999);
  EXPECT_EQ(val, nullptr);
}

TEST(HybridMapTest, FindAfterMultipleInserts) {
  yhy::HashMap<int, int> map;
  for (int i = 0; i < 1000; ++i) {
    map.insert(i, i * 2);
  }
  for (int i = 0; i < 1000; ++i) {
    auto *val = map.find(i);
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(*val, i * 2);
  }
}

TEST(HybridMapTest, FindConst) {
  yhy::HashMap<int, std::string> map;
  map.insert(42, "answer");
  const auto &const_map = map;
  const auto *val = const_map.find(42);
  ASSERT_NE(val, nullptr);
  EXPECT_EQ(*val, "answer");
}

TEST(HybridMapTest, Contains) {
  yhy::HashMap<int, std::string> map;
  map.insert(42, "answer");

  EXPECT_TRUE(map.contains(42));
  EXPECT_FALSE(map.contains(999));
}

// Operator[] tests.
TEST(HybridMapTest, OperatorBracketInsert) {
  yhy::HashMap<int, std::string> map;
  map[42] = "answer";
  EXPECT_EQ(map.size(), 1);
  EXPECT_EQ(map[42], "answer");
}

TEST(HybridMapTest, OperatorBracketUpdate) {
  yhy::HashMap<int, std::string> map;
  map[42] = "first";
  map[42] = "second";
  EXPECT_EQ(map.size(), 1);
  EXPECT_EQ(map[42], "second");
}

TEST(HybridMapTest, OperatorBracketDefaultValue) {
  yhy::HashMap<int, int> map;
  int val = map[42]; // Should create with default value.
  EXPECT_EQ(val, 0);
  EXPECT_EQ(map.size(), 1);
}

// Erase tests.
TEST(HybridMapTest, EraseExisting) {
  yhy::HashMap<int, std::string> map;
  map.insert(42, "answer");
  EXPECT_TRUE(map.erase(42));
  EXPECT_EQ(map.size(), 0);
  EXPECT_FALSE(map.contains(42));
}

TEST(HybridMapTest, EraseNonExistent) {
  yhy::HashMap<int, std::string> map;
  map.insert(42, "answer");
  EXPECT_FALSE(map.erase(999));
  EXPECT_EQ(map.size(), 1);
}

TEST(HybridMapTest, EraseMultiple) {
  yhy::HashMap<int, int> map;
  for (int i = 0; i < 100; ++i) {
    map.insert(i, i * 2);
  }
  // Erase every other element.
  for (int i = 0; i < 100; i += 2) {
    EXPECT_TRUE(map.erase(i));
  }
  EXPECT_EQ(map.size(), 50);
  // Check remaining elements.
  for (int i = 1; i < 100; i += 2) {
    EXPECT_TRUE(map.contains(i));
  }
  // Check erased elements
  for (int i = 0; i < 100; i += 2) {
    EXPECT_FALSE(map.contains(i));
  }
}

TEST(HybridMapTest, EraseAndReinsert) {
  yhy::HashMap<int, std::string> map;
  map.insert(42, "first");
  EXPECT_TRUE(map.erase(42));
  EXPECT_FALSE(map.contains(42));
  map.insert(42, "second");
  EXPECT_TRUE(map.contains(42));
  EXPECT_EQ(*map.find(42), "second");
}

// Clear tests.
TEST(HybridMapTest, Clear) {
  yhy::HashMap<int, int> map;
  for (int i = 0; i < 100; ++i) {
    map.insert(i, i * 2);
  }
  EXPECT_EQ(map.size(), 100);
  map.clear();
  EXPECT_EQ(map.size(), 0);
  EXPECT_TRUE(map.empty());
  map.insert(42, 84);
  EXPECT_EQ(map.size(), 1);
  EXPECT_EQ(*map.find(42), 84);
}

// Collision handling.
TEST(HybridMapTest, CollisionHandling) {
  // Create map with small capacity to force collisions.
  yhy::HashMap<int, int> map(8);
  // Insert many elements to force collisions.
  for (int i = 0; i < 100; ++i) {
    map.insert(i, i * 2);
  }
  // Verify all elements are present.
  for (int i = 0; i < 100; ++i) {
    auto *val = map.find(i);
    ASSERT_NE(val, nullptr) << "Key " << i << " not found";
    EXPECT_EQ(*val, i * 2);
  }
}

TEST(HybridMapTest, ProbeChainAfterErase) {
  yhy::HashMap<int, int> map(8);
  // Insert elements that will hash to same bucket.
  map.insert(0, 0);
  map.insert(8, 8);   // Likely to collide with 0.
  map.insert(16, 16); // Likely to collide with 0 and 8.
  // Erase middle element.
  map.erase(8);
  // Should still find the last element.
  EXPECT_TRUE(map.contains(0));
  EXPECT_FALSE(map.contains(8));
  EXPECT_TRUE(map.contains(16));
}

// Rehashing tests.
TEST(HybridMapTest, AutomaticRehashing) {
  yhy::HashMap<int, int> map(16);
  size_t initial_capacity = map.capacity();
  // Insert enough elements to trigger rehash.
  for (int i = 0; i < 1000; ++i) {
    map.insert(i, i * 2);
  }
  EXPECT_GT(map.capacity(), initial_capacity);
  EXPECT_EQ(map.size(), 1000);
  // Verify all elements are still present after rehash.
  for (int i = 0; i < 1000; ++i) {
    auto *val = map.find(i);
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(*val, i * 2);
  }
}

TEST(HybridMapTest, LoadFactorMaintained) {
  yhy::HashMap<int, int> map;
  // Insert many elements.
  for (int i = 0; i < 10000; ++i) {
    map.insert(i, i);
  }
  // Load factor should be maintained <= 0.75.
  double load_factor = static_cast<double>(map.size()) / map.capacity();
  EXPECT_LE(load_factor, 0.75);
}

// String tests.
TEST(HybridMapTest, StringKeys) {
  yhy::HashMap<std::string, int> map;
  map.insert("hello", 1);
  map.insert("world", 2);
  map.insert("foo", 3);
  map.insert("bar", 4);
  EXPECT_EQ(map.size(), 4);
  EXPECT_EQ(*map.find("hello"), 1);
  EXPECT_EQ(*map.find("world"), 2);
  EXPECT_EQ(*map.find("foo"), 3);
  EXPECT_EQ(*map.find("bar"), 4);
}

TEST(HybridMapTest, StringKeyCollisions) {
  yhy::HashMap<std::string, int> map;
  // Insert many strings.
  for (int i = 0; i < 100; ++i) {
    std::string key = "key_" + std::to_string(i);
    map.insert(key, i);
  }
  // Verify all are present.
  for (int i = 0; i < 100; ++i) {
    std::string key = "key_" + std::to_string(i);
    auto *val = map.find(key);
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(*val, i);
  }
}

// Large objects.
struct LargeValue {
  std::array<int, 100> data;
  LargeValue() : data{} {}
  explicit LargeValue(int val) : data{} { data.fill(val); }
  bool operator==(const LargeValue &other) const { return data == other.data; }
};

TEST(HybridMapTest, LargeValues) {
  yhy::HashMap<int, LargeValue> map;
  for (int i = 0; i < 100; ++i) {
    map.insert(i, LargeValue(i));
  }
  EXPECT_EQ(map.size(), 100);
  for (int i = 0; i < 100; ++i) {
    auto *val = map.find(i);
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(*val, LargeValue(i));
  }
}

// Large cases.
TEST(HybridMapTest, InsertZeroKey) {
  yhy::HashMap<int, std::string> map;
  map.insert(0, "zero");
  EXPECT_TRUE(map.contains(0));
  EXPECT_EQ(*map.find(0), "zero");
}

TEST(HybridMapTest, InsertNegativeKeys) {
  yhy::HashMap<int, int> map;
  for (int i = -50; i < 50; ++i) {
    map.insert(i, i * 2);
  }
  for (int i = -50; i < 50; ++i) {
    auto *val = map.find(i);
    ASSERT_NE(val, nullptr);
    EXPECT_EQ(*val, i * 2);
  }
}

TEST(HybridMapTest, EmptyStringKey) {
  yhy::HashMap<std::string, int> map;
  map.insert("", 42);
  EXPECT_TRUE(map.contains(""));
  EXPECT_EQ(*map.find(""), 42);
}

TEST(HybridMapTest, VeryLongStringKey) {
  yhy::HashMap<std::string, int> map;
  std::string long_key(10000, 'a');
  map.insert(long_key, 42);
  EXPECT_TRUE(map.contains(long_key));
  EXPECT_EQ(*map.find(long_key), 42);
}

// Move semantics.
TEST(HybridMapTest, MoveConstruction) {
  yhy::HashMap<int, std::string> map1;
  map1.insert(42, "answer");
  map1.insert(73, "value");
  yhy::HashMap<int, std::string> map2(std::move(map1));
  EXPECT_EQ(map2.size(), 2);
  EXPECT_TRUE(map2.contains(42));
  EXPECT_TRUE(map2.contains(73));
}

TEST(HybridMapTest, MoveAssignment) {
  yhy::HashMap<int, std::string> map1;
  map1.insert(42, "answer");
  map1.insert(73, "value");
  yhy::HashMap<int, std::string> map2;
  map2.insert(99, "old");
  map2 = std::move(map1);
  EXPECT_EQ(map2.size(), 2);
  EXPECT_TRUE(map2.contains(42));
  EXPECT_TRUE(map2.contains(73));
  EXPECT_FALSE(map2.contains(99));
}

// Custom hash functions.
struct Point {
  int x, y;
  bool operator==(const Point &other) const {
    return x == other.x && y == other.y;
  }
};

struct PointHash {
  size_t operator()(const Point &p) const {
    return std::hash<int>{}(p.x) ^ (std::hash<int>{}(p.y) << 1);
  }
};

TEST(HybridMapTest, CustomHashFunction) {
  yhy::HashMap<Point, std::string, PointHash> map;
  map.insert(Point{1, 2}, "point1");
  map.insert(Point{3, 4}, "point2");
  map.insert(Point{5, 6}, "point3");
  EXPECT_EQ(map.size(), 3);
  EXPECT_TRUE(map.contains(Point{1, 2}));
  EXPECT_TRUE(map.contains(Point{3, 4}));
  EXPECT_TRUE(map.contains(Point{5, 6}));
  EXPECT_FALSE(map.contains(Point{7, 8}));
  EXPECT_EQ(*map.find(Point{1, 2}), "point1");
  EXPECT_EQ(*map.find(Point{3, 4}), "point2");
  EXPECT_EQ(*map.find(Point{5, 6}), "point3");
}

TEST(HybridMapTest, LargeScaleInsertFind) {
  yhy::HashMap<int, int> map(10000);
  const int N = 50000;
  // Insert.
  for (int i = 0; i < N; ++i) {
    map.insert(i, i * 3);
  }
  EXPECT_EQ(map.size(), N);
  // Find all.
  for (int i = 0; i < N; ++i) {
    auto *val = map.find(i);
    ASSERT_NE(val, nullptr) << "Key " << i << " not found";
    EXPECT_EQ(*val, i * 3);
  }
}

TEST(HybridMapTest, RandomOperations) {
  yhy::HashMap<int, int> map;
  std::unordered_set<int> inserted_keys;
  // Random inserts.
  for (int i = 0; i < 1000; ++i) {
    int key = rand() % 10000;
    map.insert(key, key * 2);
    inserted_keys.insert(key);
  }
  // Verify all inserted keys are present.
  for (int key : inserted_keys) {
    EXPECT_TRUE(map.contains(key));
  }
  // Random erases.
  int erase_count = 0;
  for (int key : inserted_keys) {
    if (erase_count++ % 2 == 0) {
      map.erase(key);
    }
  }
  // Verify erased keys are gone.
  erase_count = 0;
  for (int key : inserted_keys) {
    if (erase_count++ % 2 == 0) {
      EXPECT_FALSE(map.contains(key));
    } else {
      EXPECT_TRUE(map.contains(key));
    }
  }
}

TEST(HybridMapTest, AlternatingInsertErase) {
  yhy::HashMap<int, int> map;
  for (int i = 0; i < 100; ++i) {
    // Insert.
    map.insert(i, i * 2);
    EXPECT_EQ(map.size(), 1);
    // Verify.
    EXPECT_TRUE(map.contains(i));
    EXPECT_EQ(*map.find(i), i * 2);
    // Erase.
    map.erase(i);
    EXPECT_EQ(map.size(), 0);
    EXPECT_FALSE(map.contains(i));
  }
}

TEST(HybridMapTest, FindMissPerformanceCharacteristic) {
  yhy::HashMap<int, int> map;
  // Insert elements.
  for (int i = 0; i < 10000; ++i) {
    map.insert(i * 2, i);
  }
  // Find all odd numbers (misses).
  for (int i = 1; i < 20000; i += 2) {
    auto *val = map.find(i);
    EXPECT_EQ(val, nullptr);
  }
}

TEST(HybridMapTest, SequentialVsRandomInsert) {
  // Sequential.
  yhy::HashMap<int, int> seq_map;
  for (int i = 0; i < 1000; ++i) {
    seq_map.insert(i, i);
  }
  EXPECT_EQ(seq_map.size(), 1000);
  // Random.
  yhy::HashMap<int, int> rand_map;
  std::vector<int> keys(1000);
  std::iota(keys.begin(), keys.end(), 0);
  std::random_device rd;
  std::mt19937 g(rd());
  std::shuffle(keys.begin(), keys.end(), g);
  for (int key : keys) {
    rand_map.insert(key, key);
  }
  EXPECT_EQ(rand_map.size(), 1000);
  // Both should contain same elements.
  for (int i = 0; i < 1000; ++i) {
    EXPECT_TRUE(seq_map.contains(i));
    EXPECT_TRUE(rand_map.contains(i));
  }
}

// Small types should use FlatHashMap.
struct SmallKey {
  int id;
  bool operator==(const SmallKey &other) const { return id == other.id; }
};

struct SmallValue {
  double data;
};

// Large types should use NodeHashMap.
struct LargeKey {
  char data[128];
  bool operator==(const LargeKey &other) const {
    return std::string(data) == std::string(other.data);
  }
};

struct LargeValue2 {
  char data[256];
};

namespace std {
template <> struct hash<SmallKey> {
  size_t operator()(const SmallKey &k) const { return std::hash<int>{}(k.id); }
};
template <> struct hash<LargeKey> {
  size_t operator()(const LargeKey &k) const {
    return std::hash<std::string>{}(std::string(k.data));
  }
};
} // namespace std

// Generic test fixture template.
template <typename Map> class HashMapTest : public ::testing::Test {
protected:
  Map map;
};

using MapTypes =
    ::testing::Types<NodeHashMap<int, int>, FlatHashMap<int, int>,
                     HashMap<int, int>, HashMap<SmallKey, SmallValue>,
                     HashMap<std::string, int>>;

TYPED_TEST_SUITE(HashMapTest, MapTypes);

TYPED_TEST(HashMapTest, InsertAndFind) {
  auto &map = this->map;

  // For int->int or int->string.
  if constexpr (std::is_same_v<typename TypeParam::key_type, int> ||
                std::is_same_v<std::decay_t<decltype(map)>,
                               NodeHashMap<int, int>> ||
                std::is_same_v<std::decay_t<decltype(map)>,
                               FlatHashMap<int, int>> ||
                std::is_same_v<std::decay_t<decltype(map)>,
                               HashMap<int, int>>) {
    auto [ptr, inserted] = map.insert(42, 100);
    EXPECT_TRUE(inserted);
    EXPECT_NE(ptr, nullptr);

    auto *found = map.find(42);
    EXPECT_NE(found, nullptr);

    auto *not_found = map.find(999);
    EXPECT_EQ(not_found, nullptr);
  }
}

TYPED_TEST(HashMapTest, UpdateExisting) {
  auto &map = this->map;

  if constexpr (std::is_same_v<std::decay_t<decltype(map)>,
                               FlatHashMap<int, int>> ||
                std::is_same_v<std::decay_t<decltype(map)>,
                               HashMap<int, int>>) {
    map.insert(1, 10);
    auto [ptr, inserted] = map.insert(1, 20);
    EXPECT_FALSE(inserted); // Key exists, should update.
    EXPECT_EQ(*ptr, 20);

    auto *found = map.find(1);
    EXPECT_EQ(*found, 20);
  }
}

TYPED_TEST(HashMapTest, EraseKey) {
  auto &map = this->map;

  if constexpr (std::is_same_v<std::decay_t<decltype(map)>,
                               FlatHashMap<int, int>> ||
                std::is_same_v<std::decay_t<decltype(map)>,
                               HashMap<int, int>>) {
    map.insert(5, 50);
    EXPECT_TRUE(map.contains(5));

    bool erased = map.erase(5);
    EXPECT_TRUE(erased);
    EXPECT_FALSE(map.contains(5));

    bool erased_again = map.erase(5);
    EXPECT_FALSE(erased_again);
  }
}

TYPED_TEST(HashMapTest, Clear) {
  auto &map = this->map;

  if constexpr (std::is_same_v<std::decay_t<decltype(map)>,
                               FlatHashMap<int, int>> ||
                std::is_same_v<std::decay_t<decltype(map)>,
                               HashMap<int, int>>) {
    map.insert(1, 10);
    map.insert(2, 20);
    EXPECT_EQ(map.size(), 2);

    map.clear();
    EXPECT_EQ(map.size(), 0);
    EXPECT_TRUE(map.empty());
    EXPECT_FALSE(map.contains(1));
  }
}

TYPED_TEST(HashMapTest, OperatorBracket) {
  auto &map = this->map;

  if constexpr (std::is_same_v<std::decay_t<decltype(map)>,
                               FlatHashMap<int, int>> ||
                std::is_same_v<std::decay_t<decltype(map)>,
                               HashMap<int, int>>) {
    map[10] = 100;
    EXPECT_EQ(map[10], 100);

    // Access non-existent key should insert default.
    int val = map[999];
    EXPECT_EQ(val, 0);
    EXPECT_TRUE(map.contains(999));
  }
}

TYPED_TEST(HashMapTest, Rehashing) {
  auto &map = this->map;

  if constexpr (std::is_same_v<std::decay_t<decltype(map)>,
                               FlatHashMap<int, int>> ||
                std::is_same_v<std::decay_t<decltype(map)>,
                               HashMap<int, int>>) {
    // Insert many elements to trigger rehashing.
    for (int i = 0; i < 1000; ++i) {
      map.insert(i, i * 10);
    }

    EXPECT_EQ(map.size(), 1000);

    // Verify all elements are accessible.
    for (int i = 0; i < 1000; ++i) {
      auto *val = map.find(i);
      ASSERT_NE(val, nullptr);
      EXPECT_EQ(*val, i * 10);
    }
  }
}

template <typename Key, typename Value> struct is_flat_map_suitable {
  static constexpr bool value =
      // Must fit in cache line with hash (8 bytes for hash).
      (sizeof(Key) + sizeof(Value) <= 56) &&
      // Must be nothrow move constructible for safe rehashing.
      std::is_nothrow_move_constructible_v<Key> &&
      std::is_nothrow_move_constructible_v<Value>;
};

template <typename Key, typename Value>
inline constexpr bool is_flat_map_suitable_v =
    is_flat_map_suitable<Key, Value>::value;

// Type trait tests.
TEST(TypeTraitTest, FlatMapSuitability) {
  // Small types should be suitable.
  EXPECT_TRUE((is_flat_map_suitable_v<int, int>));
  EXPECT_TRUE((is_flat_map_suitable_v<SmallKey, SmallValue>));

  // Large types should not be suitable.
  EXPECT_FALSE((is_flat_map_suitable_v<LargeKey, LargeValue2>));

  // String is move-constructible but might be too large depending on SSO.
  // This tests the size constraint.
  EXPECT_TRUE((is_flat_map_suitable_v<int, std::string>));
}

// Test automatic type selection.
TEST(HybridMapTest, AutomaticSelection) {
  // Small types should resolve to FlatHashMap.
  using SmallMap = HashMap<int, int>;
  static_assert(std::is_same_v<SmallMap, FlatHashMap<int, int>>);

  // Large types should resolve to NodeHashMap.
  using LargeMap = HashMap<LargeKey, LargeValue2>;
  static_assert(std::is_same_v<LargeMap, NodeHashMap<LargeKey, LargeValue2>>);
}

// Specific FlatHashMap tests.
TEST(FlatHashMapTest, BasicOperations) {
  FlatHashMap<int, int> map;

  map.insert(1, 10);
  map.insert(2, 20);
  map.insert(3, 30);

  EXPECT_EQ(map.size(), 3);
  EXPECT_EQ(*map.find(2), 20);

  map.erase(2);
  EXPECT_EQ(map.size(), 2);
  EXPECT_EQ(map.find(2), nullptr);
}

// Specific NodeHashMap tests.
TEST(NodeHashMapTest, BasicOperations) {
  NodeHashMap<std::string, std::string> map;

  map.insert("key1", "value1");
  map.insert("key2", "value2");

  EXPECT_EQ(map.size(), 2);
  EXPECT_EQ(*map.find("key1"), "value1");

  map.erase("key1");
  EXPECT_EQ(map.size(), 1);
  EXPECT_EQ(map.find("key1"), nullptr);
}

// Test move semantics.
TEST(FlatHashMapTest, MoveConstructor) {
  FlatHashMap<int, int> map1;
  map1.insert(1, 100);
  map1.insert(2, 200);

  FlatHashMap<int, int> map2(std::move(map1));
  EXPECT_EQ(map2.size(), 2);
  EXPECT_EQ(*map2.find(1), 100);
  EXPECT_EQ(map1.size(), 0); // map1 should be empty after move.
}

TEST(NodeHashMapTest, MoveConstructor) {
  NodeHashMap<int, int> map1;
  map1.insert(1, 100);
  map1.insert(2, 200);

  NodeHashMap<int, int> map2(std::move(map1));
  EXPECT_EQ(map2.size(), 2);
  EXPECT_EQ(*map2.find(1), 100);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
