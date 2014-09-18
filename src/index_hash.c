#include <mixutil/index_hash.h>

#ifdef UNIT_TEST
#define static 
#endif

typedef struct index_hash_item_t item_t;
#define NIL INDEX_HASH_KEY_NIL

#ifdef INDEX_HASH_DEBUG
# define INIT_DEBUG(hash) hash->nget = hash->nite = 0
# define INC_GET(hash) hash->nget++
# define INC_ITE(hash) hash->nite++
#else
# define INIT_DEBUG(hash)
# define INC_GET(hash)
# define INC_ITE(hash)
#endif

#define HASH(key, size) (key & (size-1))
#define NEXT_SLOT(idx, size) (idx+3 >= size ? 0 : idx+3)

static uint32_t power_floor(uint32_t size);

index_hash_t *index_hash_init(uint32_t size)
{
  size = power_floor(size);
  index_hash_t *hash = malloc(sizeof(index_hash_t));
  hash->nitem = 0;
  hash->nslot = size;
  
  hash->slot = malloc(sizeof(item_t) * size);
  for (uint32_t i = 0; i < size; ++i) {
    hash->slot[i].key = NIL;
  }
  
  INIT_DEBUG(hash);
  return hash;
}

uintptr_t index_hash_get(index_hash_t *hash, uint32_t key)
{
  INC_GET(hash);
  
  uint32_t idx = HASH(key, hash->nslot);
  for (uint32_t i = 0; i < hash->nslot; i++) {
    INC_ITE(hash);
    if (hash->slot[idx].key == key) {
      return hash->slot[idx].val;
    } else {
      idx = NEXT_SLOT(idx, hash->nslot);
    }
  }
  return INDEX_HASH_VAL_NIL;
}

#define SWAP(type, a, b) do {   \
  type tmp = a; a = b; b = tmp; \
} while(0);

static void index_hash_put_internal(index_hash_t *hash, uint32_t key, uintptr_t val)
{
  uint32_t idx = HASH(key, hash->nslot);
  if (hash->slot[idx].key == NIL) {
    hash->slot[idx].key = key;
    hash->slot[idx].val = val;
    return;
  } else if (HASH(hash->slot[idx].key, hash->nslot) != idx) {
    SWAP(uint32_t, hash->slot[idx].key, key);
    SWAP(uintptr_t, hash->slot[idx].val, val);
    return index_hash_put_internal(hash, key, val);
  } else {
    idx = NEXT_SLOT(idx, hash->nslot);
    for (uint32_t i = 1; i < hash->nslot; ++i) {
      if (hash->slot[idx].key == NIL) {
        hash->slot[idx].key = key;
        hash->slot[idx].val = val;
        return;
      } else {
        idx = NEXT_SLOT(idx, hash->nslot);
      }
    }
  }
}

void index_hash_put(index_hash_t *hash, uint32_t key, uintptr_t val)
{
  hash->nitem++;
  if (hash->nitem * 3 > hash->nslot * 2) {
    if (hash->nslot > (1 << 23)) abort();

    item_t *slot   = hash->slot;
    uint32_t nslot = hash->nslot;
    
    hash->nslot *= 2;
    hash->slot = malloc(sizeof(item_t) * hash->nslot);
    for (uint32_t i = 0; i < hash->nslot; ++i) {
      hash->slot[i].key = NIL;
    }

    for (uint32_t i = 0; i < nslot; ++i) {
      if (slot[i].key != NIL) {
        index_hash_put_internal(hash, slot[i].key, slot[i].val);
      }
    }
  }
  index_hash_put_internal(hash, key, val);
}

void index_hash_del(index_hash_t *hash, uint32_t key)
{
  uint32_t idx = HASH(key, hash->nslot);
  for (uint32_t i = 0; i < hash->nslot; i++) {
    if (hash->slot[idx].key == key) {
      hash->nitem--;
      hash->slot[idx].key = NIL;
      return;
    } else {
      NEXT_SLOT(idx, hash->nslot);
    }
  }
}

void index_hash_destroy(index_hash_t *hash)
{
  free(hash->slot);
  free(hash);
}

index_hash_ite index_hash_begin(index_hash_t *hash)
{
  for (uint32_t i = 0; i < hash->nslot; ++i) {
    if (hash->slot[i].key != NIL) return i;
  }
  return hash->nslot;
}

index_hash_ite index_hash_next(index_hash_t *hash, index_hash_ite ite)
{
  for (uint32_t i = ite+1; i < hash->nslot; ++i) {
    if (hash->slot[i].key != NIL) return i;
  }
  return hash->nslot;
}

static uint32_t power_floor(uint32_t size)
{
  uint32_t power = 0;
  for (size_t i = 0; i < sizeof(size) * 8; ++i) {
    if (size & 1) {
      power = i;
    }
    size >>= 1;
  }
  return 1 << power;
}
