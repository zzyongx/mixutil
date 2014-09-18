#include <iostream>
#include <ext/hash_map>
#include <ctime>
#include <gtest/gtest.h>
#include <mixutil/index_hash.h>

extern "C" {
uint32_t power_floor(uint32_t);
}

TEST(IndexHashTest, power_floor) {
  ASSERT_EQ(power_floor(9), 8);
  ASSERT_EQ(power_floor(8), 8);
  ASSERT_EQ(power_floor(7), 4);
}

TEST(IndexHashTest, api) {
  index_hash_t *hash = index_hash_init(19);

  index_hash_put(hash, 0, 0);
  index_hash_put(hash, 16, 160);
  index_hash_put(hash, 3, 30);
  index_hash_put(hash, 4, 40);

  ASSERT_EQ(index_hash_get(hash, 0), 0);
  ASSERT_EQ(index_hash_get(hash, 16), 160);
  ASSERT_EQ(index_hash_get(hash, 3), 30);
  ASSERT_EQ(index_hash_get(hash, 4), 40);
  
  {
    for (index_hash_ite ite = index_hash_begin(hash);
         ite != index_hash_end(hash);
         ite = index_hash_next(hash, ite)) {
      std::cout << "ite: " << ite << " "
                << "key: " << index_hash_ite_key(hash, ite) << " "
                << "val: " << index_hash_ite_val(hash, ite) << "\n";
    }
  }

  index_hash_ite ite = index_hash_begin(hash);
  ASSERT_EQ(index_hash_ite_key(hash, ite), 0);
  ASSERT_EQ(index_hash_ite_val(hash, ite), 0);

  ite = index_hash_next(hash, ite);
  ASSERT_EQ(index_hash_ite_key(hash, ite), 3);
  ASSERT_EQ(index_hash_ite_val(hash, ite), 30);

  index_hash_del(hash, ite);
  ASSERT_EQ(index_hash_get(hash, 3), INDEX_HASH_VAL_NIL);

  ite = index_hash_next(hash, ite);
  ASSERT_EQ(index_hash_ite_key(hash, ite), 4);
  ASSERT_EQ(index_hash_ite_val(hash, ite), 40);

  index_hash_ite_put(hash, ite, 400);
  ASSERT_EQ(index_hash_get(hash, 4), 400);

  ite = index_hash_next(hash, ite);
  ASSERT_EQ(index_hash_ite_key(hash, ite), 16);
  ASSERT_EQ(index_hash_ite_val(hash, ite), 160);

  ite = index_hash_next(hash, ite);
  ASSERT_EQ(ite, index_hash_end(hash));
}

TEST(IndexHashTest, hashexpand) {
  const int N = 8;
  int *array = new int[N];
  for (int i = 0; i < N; ++i) {
    array[i] = rand();
  }

  index_hash_t *hash = index_hash_init(4);
  for (int i = 0; i < N; ++i) {
    index_hash_put(hash, array[i], array[i]);
  }
  for (int i = 0; i < N; ++i) {
    ASSERT_EQ(index_hash_get(hash, array[i]), array[i]);
  }
}
  
TEST(IndexHashTest, performance) {
  const int N = 1e7;
  int *array = new int[N];
  for (int i = 0; i < N; ++i) {
    array[i] = rand();
  }

  time_t start = time(0);
  __gnu_cxx::hash_map<int, int> hash_map;
  for (int i = 0; i < N; ++i) {
    hash_map[array[i]] = array[i];
  }
  std::cout << "hash_map put: " << time(0) - start << "s\n";

  start = time(0);
  index_hash_t *hash = index_hash_init(32);
  for (int i = 0; i < N; ++i) {
    index_hash_put(hash, array[i], array[i]);
  }
  std::cout << "index_hash put: " << time(0) - start << "s\n";

  start = time(0);
  for (int i = 0; i < 5; ++i) {
    for (int j = 0; j < N; ++j) {
      hash_map.find(array[j]);
    }
  }
  std::cout << "hash_map get: " << time(0) - start << "s\n";

  start = time(0);
  for (int i = 0; i < 5; ++i) {
    for (int j = 0; j < N; ++j) {
      index_hash_get(hash, array[i]);
    }
  }
  std::cout << "index_hash get: " << time(0) - start << "s\n";
  
  delete[] array;
}
    
