#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif
  
struct index_hash_item_t {
  uint32_t   key;
  uintptr_t  val;
};

typedef struct {
  uint32_t   nitem;
  uint32_t   depth;
  uint32_t   nslot;
  struct index_hash_item_t *slot;

#ifdef INDEX_HASH_DEBUG
  uint32_t  nget;
  uint32_t  nite;
#endif  
} index_hash_t;

#define INDEX_HASH_KEY_NIL ((uint32_t) -1)
#define INDEX_HASH_VAL_NIL ((uintptr_t) -1)

index_hash_t *index_hash_init(uint32_t size);
uintptr_t     index_hash_get(index_hash_t *hash, uint32_t key);
void          index_hash_put(index_hash_t *hash, uint32_t key, uintptr_t val);
void          index_hash_del(index_hash_t *hash, uint32_t key);
void          index_hash_destroy(index_hash_t *hash);

#define index_hash_size(hash) hash->nitem
uint32_t *index_hash_keys(index_hash_t *hash, uint32_t *keys);

typedef uint32_t index_hash_ite;

index_hash_ite index_hash_begin(index_hash_t *hash);
index_hash_ite index_hash_next(index_hash_t *hash, index_hash_ite ite);
#define index_hash_end(hash) hash->nslot

#define index_hash_ite_key(hash, ite) hash->slot[ite].key
#define index_hash_ite_val(hash, ite) hash->slot[ite].val
#define index_hash_ite_put(hash, ite, newval) hash->slot[ite].val = newval;
#define index_hash_ite_del(hash, ite) hash->slot[ite].key = INDEX_HASH_KEY_NIL

/* traverse the hash
   for (index_hash_ite ite = index_hash_begin(hash);
        ite != index_hash_end(hash);
        ite = index_hash_next(hash, ite)) {
     uint32_t key = index_hash_ite_key(hash, ite);
     uintptr_t val = index_hash_ite_val(hash, ite);
     index_hash_ite_put(hash, ite, newval);
     index_hash_ite_del(hash, ite);
   }
*/

#ifdef __cplusplus
}
#endif
