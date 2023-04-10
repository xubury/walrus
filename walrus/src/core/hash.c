#include <core/hash.h>
#include <core/macro.h>
#include <core/memory.h>

#include <stdlib.h>
#include <string.h>

#define HASH_TABLE_MIN_SHIFT 3 /* 1 << 3 == 8 buckets */

#define BIG_ENTRY_SIZE   8
#define SMALL_ENTRY_SIZE 4

#define UNUSED_HASH_VALUE     0
#define TOMBSTONE_HASH_VALUE  1
#define HASH_IS_UNUSED(h_)    ((h_) == UNUSED_HASH_VALUE)
#define HASH_IS_TOMBSTONE(h_) ((h_) == TOMBSTONE_HASH_VALUE)
#define HASH_IS_REAL(h_)      ((h_) >= 2)

struct _Walrus_HashTable {
    u64 size;

    u32 mod;
    u32 mask;
    u32 nnodes;
    u32 noccupied;

    u32 has_big_keys : 1;
    u32 has_big_values : 1;

    void* keys;
    void* values;
    u32*  hashes;

    Walrus_HashFunc  hash_fn;
    Walrus_EqualFunc key_equal_fn;

    Walrus_KeyDestroyFunc   key_destroy_fn;
    Walrus_ValueDestroyFunc val_destroy_fn;
};

static void* hash_table_realloc(void* p, u64 size, bool is_big)
{
    p = realloc(p, size * (is_big ? BIG_ENTRY_SIZE : SMALL_ENTRY_SIZE));
    return p;
}

static void realloc_arrays(Walrus_HashTable* hash_table, bool is_a_set)
{
    hash_table->hashes = realloc(hash_table->hashes, hash_table->size * sizeof(u32));
    hash_table->keys   = hash_table_realloc(hash_table->keys, hash_table->size, hash_table->has_big_keys);

    if (is_a_set)
        hash_table->values = hash_table->keys;
    else
        hash_table->values = hash_table_realloc(hash_table->values, hash_table->size, hash_table->has_big_values);
}

static inline u32 hash_table_hash_to_index(Walrus_HashTable* table, u32 hash_value)
{
    /* Multiply the hash by a small prime before applying the modulo. This
     * prevents the table from becoming densely packed, even with a poor hash
     * function. A densely packed table would have poor performance on
     * workloads with many failed lookups or a high degree of churn. */
    return (hash_value * 11) % table->mod;
}

static inline void* hash_table_fetch(void* p, u32 id, bool is_big)
{
    return is_big ? *((void**)(p) + id) : walrus_u32_to_ptr(*((u32*)(p) + id));
}

static inline void hash_table_assign(void* a, u32 index, bool is_big, void* v)
{
    if (is_big) {
        *(((void**)a) + index) = v;
    }
    else {
        *(((u32*)a) + index) = walrus_ptr_to_u32(v);
    }
}

static inline void* hash_table_evict(void* a, u32 index, bool is_big, void* v)
{
    if (is_big) {
        void* r              = *((void**)a + index);
        *((void**)a + index) = v;
        return r;
    }
    else {
        void* r            = walrus_u32_to_ptr(*((u32*)a + index));
        *((u32*)a + index) = walrus_ptr_to_u32(v);
        return r;
    }
}

static inline bool entry_is_big(void* v)
{
    return (((u64)v) >> ((BIG_ENTRY_SIZE - SMALL_ENTRY_SIZE) * 8)) != 0;
}

static inline void set_status_bit(u32* bitmap, u32 index)
{
    bitmap[index / 32] |= 1U << (index % 32);
}

static inline bool get_status_bit(const u32* bitmap, u32 index)
{
    return (bitmap[index / 32] >> (index % 32)) & 1;
}
/* By calling dedicated resize functions for sets and maps, we avoid 2x
 * test-and-branch per key in the inner loop. This yields a small
 * performance improvement at the cost of a bit of macro gunk. */

#define DEFINE_RESIZE_FUNC(fname)                                                                  \
    static void fname(Walrus_HashTable* hash_table, u32 old_size, u32* reallocated_buckets_bitmap) \
    {                                                                                              \
        u32 i;                                                                                     \
                                                                                                   \
        for (i = 0; i < old_size; i++) {                                                           \
            u32   node_hash = hash_table->hashes[i];                                               \
            void *key, *value;                                                                     \
            walrus_unused(value);                                                                  \
                                                                                                   \
            if (!HASH_IS_REAL(node_hash)) {                                                        \
                /* Clear tombstones */                                                             \
                hash_table->hashes[i] = UNUSED_HASH_VALUE;                                         \
                continue;                                                                          \
            }                                                                                      \
                                                                                                   \
            /* Skip entries relocated through eviction */                                          \
            if (get_status_bit(reallocated_buckets_bitmap, i)) continue;                           \
                                                                                                   \
            hash_table->hashes[i] = UNUSED_HASH_VALUE;                                             \
            EVICT_KEYVAL(hash_table, i, NULL, NULL, key, value);                                   \
                                                                                                   \
            for (;;) {                                                                             \
                u32 hash_val;                                                                      \
                u32 replaced_hash;                                                                 \
                u32 step = 0;                                                                      \
                                                                                                   \
                hash_val = hash_table_hash_to_index(hash_table, node_hash);                        \
                                                                                                   \
                while (get_status_bit(reallocated_buckets_bitmap, hash_val)) {                     \
                    step++;                                                                        \
                    hash_val += step;                                                              \
                    hash_val &= hash_table->mask;                                                  \
                }                                                                                  \
                                                                                                   \
                set_status_bit(reallocated_buckets_bitmap, hash_val);                              \
                                                                                                   \
                replaced_hash                = hash_table->hashes[hash_val];                       \
                hash_table->hashes[hash_val] = node_hash;                                          \
                if (!HASH_IS_REAL(replaced_hash)) {                                                \
                    ASSIGN_KEYVAL(hash_table, hash_val, key, value);                               \
                    break;                                                                         \
                }                                                                                  \
                                                                                                   \
                node_hash = replaced_hash;                                                         \
                EVICT_KEYVAL(hash_table, hash_val, key, value, key, value);                        \
            }                                                                                      \
        }                                                                                          \
    }

#define ASSIGN_KEYVAL(ht, index, key, value)                                     \
    WR_STMT_BEGIN                                                                \
    {                                                                            \
        hash_table_assign((ht)->keys, (index), (ht)->has_big_keys, (key));       \
        hash_table_assign((ht)->values, (index), (ht)->has_big_values, (value)); \
    }                                                                            \
    WR_STMT_END

#define EVICT_KEYVAL(ht, index, key, value, outkey, outvalue)                                \
    WR_STMT_BEGIN                                                                            \
    {                                                                                        \
        (outkey)   = hash_table_evict((ht)->keys, (index), (ht)->has_big_keys, (key));       \
        (outvalue) = hash_table_evict((ht)->values, (index), (ht)->has_big_values, (value)); \
    }                                                                                        \
    WR_STMT_END

DEFINE_RESIZE_FUNC(resize_map)

#undef ASSIGN_KEYVAL
#undef EVICT_KEYVAL

#define ASSIGN_KEYVAL(ht, index, key, value)                               \
    WR_STMT_BEGIN                                                          \
    {                                                                      \
        hash_table_assign((ht)->keys, (index), (ht)->has_big_keys, (key)); \
    }                                                                      \
    WR_STMT_END

#define EVICT_KEYVAL(ht, index, key, value, outkey, outvalue)                        \
    WR_STMT_BEGIN                                                                    \
    {                                                                                \
        (outkey) = hash_table_evict((ht)->keys, (index), (ht)->has_big_keys, (key)); \
    }                                                                                \
    WR_STMT_END

DEFINE_RESIZE_FUNC(resize_set)

#undef ASSIGN_KEYVAL
#undef EVICT_KEYVAL

static inline bool hash_table_maybe_make_big(void** a_p, void* v, u32 size)
{
    if (entry_is_big(v)) {
        u32*   a = (u32*)*a_p;
        void** a_new;

        a_new = walrus_malloc(sizeof(void*) * size);

        for (u32 i = 0; i < size; i++) {
            a_new[i] = walrus_u32_to_ptr(a[i]);
        }

        walrus_free(a);
        *a_p = a_new;
        return true;
    }

    return false;
}

static u32 const prime_mod[] = {
    // clang-format off
  1,          /* For 1 << 0 */
  2,
  3,
  7,
  13,
  31,
  61,
  127,
  251,
  509,
  1021,
  2039,
  4093,
  8191,
  16381,
  32749,
  65521,      /* For 1 << 16 */
  131071,
  262139,
  524287,
  1048573,
  2097143,
  4194301,
  8388593,
  16777213,
  33554393,
  67108859,
  134217689,
  268435399,
  536870909,
  1073741789,
  2147483647  /* For 1 << 31 */
    // clang-format on
};

static void hash_table_set_shift(Walrus_HashTable* table, u32 shift)
{
    table->size = 1 << shift;
    table->mod  = prime_mod[shift];

    /* hash_table->size is always a power of two, so we can calculate the mask
     * by simply subtracting 1 from it. The leading assertion ensures that
     * we're really dealing with a power of two. */

    walrus_assert((table->size & (table->size - 1)) == 0);
    table->mask = table->size - 1;
}

static u32 hash_table_find_closest_shift(u32 n)
{
    u32 i;

    for (i = 0; n; ++i) n >>= 1;

    return i;
}

static void hash_table_set_shift_from_size(Walrus_HashTable* hash_table, u32 size)
{
    u32 shift;

    shift = hash_table_find_closest_shift(size);
    shift = walrus_max(shift, HASH_TABLE_MIN_SHIFT);

    hash_table_set_shift(hash_table, shift);
}

static void hash_table_setup_storage(Walrus_HashTable* table)
{
    hash_table_set_shift(table, HASH_TABLE_MIN_SHIFT);
    table->has_big_keys   = false;
    table->has_big_values = false;

    table->keys   = hash_table_realloc(table->keys, table->size, table->has_big_keys);
    table->values = table->keys;
    table->hashes = walrus_malloc0(sizeof(u32) * table->size);
}

void hash_table_remove_all_nodes(Walrus_HashTable* table, bool notify, bool destruction)
{
    i32    i;
    void*  key;
    void*  value;
    i32    old_size;
    void** old_keys;
    void** old_values;
    u32*   old_hashes;
    bool   old_have_big_keys;
    bool   old_have_big_values;
    /* If the hash table is already empty, there is nothing to be done. */
    if (table->nnodes == 0) return;

    table->nnodes    = 0;
    table->noccupied = 0;

    /* Easy case: no callbacks, so we just zero out the arrays */
    if (!notify || (table->key_destroy_fn == NULL && table->val_destroy_fn == NULL)) {
        if (!destruction) {
            memset(table->hashes, 0, table->size * sizeof(u32));

            memset(table->keys, 0, table->size * (table->has_big_keys ? BIG_ENTRY_SIZE : SMALL_ENTRY_SIZE));
            memset(table->values, 0, table->size * (table->has_big_values ? BIG_ENTRY_SIZE : SMALL_ENTRY_SIZE));
        }

        return;
    }
    old_size            = table->size;
    old_have_big_keys   = table->has_big_keys;
    old_have_big_values = table->has_big_values;
    old_keys            = table->keys;
    old_values          = table->values;
    old_hashes          = table->hashes;
    table->keys         = NULL;
    table->values       = NULL;
    table->hashes       = NULL;

    if (!destruction) /* Any accesses will see an empty table */
        hash_table_setup_storage(table);
    else
        /* Will cause a quick crash on any attempted access */
        table->size = table->mod = table->mask = 0;

    /* Now do the actual destroy notifies */
    for (i = 0; i < old_size; i++) {
        if (HASH_IS_REAL(old_hashes[i])) {
            key   = hash_table_fetch(old_keys, i, old_have_big_keys);
            value = hash_table_fetch(old_values, i, old_have_big_values);

            old_hashes[i] = UNUSED_HASH_VALUE;

            hash_table_assign(old_keys, i, old_have_big_keys, NULL);
            hash_table_assign(old_values, i, old_have_big_values, NULL);

            if (table->key_destroy_fn) table->key_destroy_fn(key);

            if (table->val_destroy_fn) table->val_destroy_fn(value);
        }
    }

    /* Destroy old storage space. */
    if (old_keys != old_values) walrus_free(old_values);

    walrus_free(old_keys);
    walrus_free(old_hashes);
}

Walrus_HashTable* walrus_hash_table_create(Walrus_HashFunc hash, Walrus_EqualFunc equal)
{
    return walrus_hash_table_create_full(hash, equal, NULL, NULL);
}

Walrus_HashTable* walrus_hash_table_create_full(Walrus_HashFunc hash, Walrus_EqualFunc equal,
                                                Walrus_KeyDestroyFunc key_destroy, Walrus_ValueDestroyFunc val_destroy)
{
    Walrus_HashTable* table = walrus_malloc(sizeof(Walrus_HashTable));
    if (table) {
        table->keys           = NULL;
        table->values         = NULL;
        table->hashes         = NULL;
        table->nnodes         = 0;
        table->noccupied      = 0;
        table->hash_fn        = hash;
        table->key_equal_fn   = equal;
        table->key_destroy_fn = key_destroy;
        table->val_destroy_fn = val_destroy;
        hash_table_setup_storage(table);
    }

    return table;
}

static void hash_table_unref(Walrus_HashTable* hash_table)
{
    {
        hash_table_remove_all_nodes(hash_table, true, true);
        if (hash_table->keys != hash_table->values) walrus_free(hash_table->values);
        walrus_free(hash_table->keys);
        walrus_free(hash_table->hashes);
        walrus_free(hash_table);
    }
}
void walrus_hash_table_destroy(Walrus_HashTable* table)
{
    walrus_hash_table_remove_all(table);
    hash_table_unref(table);
}

static void hash_table_ensure_keyval_fits(Walrus_HashTable* hash_table, void* key, void* value)
{
    bool is_a_set = hash_table->keys == hash_table->values;
    /* Convert set to map */
    if (is_a_set) {
        if (hash_table->has_big_keys) {
            if (key != value) {
                hash_table->values = walrus_memdup(hash_table->keys, sizeof(void*) * hash_table->size);
            }
            /* Keys and values are both big now, so no need for further checks */
            return;
        }
        else {
            if (key != value) {
                hash_table->values = walrus_memdup(hash_table->keys, sizeof(u32) * hash_table->size);
                is_a_set           = false;
            }
        }
    }
    /* Try make keys big */
    if (!hash_table->has_big_keys) {
        hash_table->has_big_keys = hash_table_maybe_make_big(&hash_table->keys, key, hash_table->size);
        if (is_a_set) {
            hash_table->values         = hash_table->keys;
            hash_table->has_big_values = hash_table->has_big_keys;
        }
    }
    /* Try make values big */
    if (!is_a_set && !hash_table->has_big_values) {
        hash_table->has_big_values = hash_table_maybe_make_big(&hash_table->values, value, hash_table->size);
    }
}

static void hash_table_resize(Walrus_HashTable* hash_table)
{
    u32* reallocated_buckets_bitmap;
    u64  old_size;
    bool is_a_set;

    old_size = hash_table->size;
    is_a_set = hash_table->keys == hash_table->values;

    hash_table_set_shift_from_size(hash_table, hash_table->nnodes * 1.333);
    if (hash_table->size > old_size) {
        realloc_arrays(hash_table, is_a_set);
        memset(&hash_table->hashes[old_size], 0, (hash_table->size - old_size) * sizeof(u32));

        reallocated_buckets_bitmap = walrus_malloc0(sizeof(u32) * (hash_table->size + 31) / 32);
    }
    else {
        reallocated_buckets_bitmap = walrus_malloc0(sizeof(u32) * (old_size + 31) / 32);
    }

    if (is_a_set) {
        resize_set(hash_table, old_size, reallocated_buckets_bitmap);
    }
    else {
        resize_map(hash_table, old_size, reallocated_buckets_bitmap);
    }

    free(reallocated_buckets_bitmap);

    if (hash_table->size < old_size) {
        realloc_arrays(hash_table, is_a_set);
    }
    hash_table->noccupied = hash_table->nnodes;
}

static void hash_table_maybe_resize(Walrus_HashTable* hash_table)
{
    u32 noccupied = hash_table->noccupied;
    u64 size      = hash_table->size;

    if ((size > hash_table->nnodes * 4 && size > 1 << HASH_TABLE_MIN_SHIFT) || (size <= noccupied + (noccupied / 16)))
        hash_table_resize(hash_table);
}

static bool hash_table_insert_node(Walrus_HashTable* table, u32 node_id, u32 key_hash, void* new_key, void* new_value,
                                   bool keep_new_key, bool reusing_key)
{
    bool  already_exists;
    u32   old_hash;
    void* key_to_free   = NULL;
    void* key_to_keep   = NULL;
    void* value_to_free = NULL;

    old_hash       = table->hashes[node_id];
    already_exists = HASH_IS_REAL(old_hash);

    /* Proceed in three steps.  First, deal with the key because it is the
     * most complicated.  Then consider if we need to split the table in
     * two (because writing the value will result in the set invariant
     * becoming broken).  Then deal with the value.
     *
     * There are three cases for the key:
     *
     *  - entry already exists in table, reusing key:
     *    free the just-passed-in new_key and use the existing value
     *
     *  - entry already exists in table, not reusing key:
     *    free the entry in the table, use the new key
     *
     *  - entry not already in table:
     *    use the new key, free nothing
     *
     * We update the hash at the same time...
     */
    if (already_exists) {
        /* Note: we must record the old value before writing the new key
         * because we might change the value in the event that the two
         * arrays are shared.
         */
        value_to_free = hash_table_fetch(table->values, node_id, table->has_big_values);
        if (keep_new_key) {
            key_to_free = hash_table_fetch(table->keys, node_id, table->has_big_keys);
            key_to_keep = new_key;
        }
        else {
            key_to_free = new_key;
            key_to_keep = hash_table_fetch(table->keys, node_id, table->has_big_keys);
        }
    }
    else {
        table->hashes[node_id] = key_hash;
        key_to_keep            = new_key;
    }
    /* Resize key/value arrays and split table as necessary */
    hash_table_ensure_keyval_fits(table, key_to_keep, new_value);
    hash_table_assign(table->keys, node_id, table->has_big_keys, key_to_keep);

    /* Step 3: Actually do the write */
    hash_table_assign(table->values, node_id, table->has_big_values, new_value);

    if (!already_exists) {
        ++table->nnodes;

        if (HASH_IS_UNUSED(old_hash)) {
            ++table->noccupied;
            hash_table_maybe_resize(table);
        }
    }

    if (already_exists) {
        if (table->key_destroy_fn && !reusing_key && key_to_free != key_to_keep) {
            table->key_destroy_fn(key_to_free);
        }
        if (table->val_destroy_fn && value_to_free != new_value) {
            table->val_destroy_fn(value_to_free);
        }
    }
    return !already_exists;
}

static u32 hash_table_lookup_node(Walrus_HashTable* table, void const* key, u32* out_hash)
{
    u32  node_id;
    u32  node_hash;
    u32  hash_val;
    u32  first_tombstone = 0;
    u32  step            = 0;
    bool have_tombstone  = false;

    hash_val = table->hash_fn(key);
    if (!(HASH_IS_REAL(hash_val))) {
        hash_val = 2;
    }
    if (out_hash) {
        *out_hash = hash_val;
    }

    node_id   = hash_table_hash_to_index(table, hash_val);
    node_hash = table->hashes[node_id];
    while (!HASH_IS_UNUSED(node_hash)) {
        if (node_hash == hash_val) {
            void* node_key = hash_table_fetch(table->keys, node_id, table->has_big_keys);
            if (table->key_equal_fn) {
                if (table->key_equal_fn(node_key, key)) {
                    return node_id;
                }
            }
            else if (node_key == key) {
                return node_id;
            }
        }
        else if (HASH_IS_TOMBSTONE(node_hash) && !have_tombstone) {
            first_tombstone = node_id;
            have_tombstone  = true;
        }

        ++step;
        node_id += step;
        node_id &= table->mask;
        node_hash = table->hashes[node_id];
    }

    if (have_tombstone) {
        return first_tombstone;
    }
    return node_id;
}

static bool hash_table_insert_internal(Walrus_HashTable* table, void* key, void* value, bool keep_new_key)
{
    u32 key_hash = 0;
    u32 node     = 0;
    node         = hash_table_lookup_node(table, key, &key_hash);
    return hash_table_insert_node(table, node, key_hash, key, value, keep_new_key, false);
}

static void hash_table_remove_node(Walrus_HashTable* hash_table, u32 i, bool notify)
{
    void* key;
    void* value;
    key   = hash_table_fetch(hash_table->keys, i, hash_table->has_big_keys);
    value = hash_table_fetch(hash_table->values, i, hash_table->has_big_values);

    hash_table->hashes[i] = TOMBSTONE_HASH_VALUE;

    hash_table_assign(hash_table->keys, i, hash_table->has_big_keys, NULL);
    hash_table_assign(hash_table->values, i, hash_table->has_big_values, NULL);

    --hash_table->nnodes;

    if (notify && hash_table->key_destroy_fn) {
        hash_table->key_destroy_fn(key);
    }

    if (notify && hash_table->val_destroy_fn) {
        hash_table->val_destroy_fn(value);
    }
}

bool walrus_hash_table_contains(Walrus_HashTable* table, void* key)
{
    u32 node_id;
    node_id = hash_table_lookup_node(table, key, NULL);

    return HASH_IS_REAL(table->hashes[node_id]);
}

void* walrus_hash_table_lookup(Walrus_HashTable* table, void* key)
{
    u32 node_id;
    node_id = hash_table_lookup_node(table, key, NULL);

    return HASH_IS_REAL(table->hashes[node_id]) ? hash_table_fetch(table->values, node_id, table->has_big_values)
                                                : NULL;
}

bool walrus_hash_table_add(Walrus_HashTable* table, void* key)
{
    return hash_table_insert_internal(table, key, key, true);
}

bool walrus_hash_table_insert(Walrus_HashTable* table, void* key, void* value)
{
    return hash_table_insert_internal(table, key, value, false);
}

static bool hash_table_remove_internal(Walrus_HashTable* table, void* key, bool notify)
{
    u32 node_id;

    node_id = hash_table_lookup_node(table, key, NULL);

    if (!HASH_IS_REAL(table->hashes[node_id])) {
        return false;
    }

    hash_table_remove_node(table, node_id, notify);
    hash_table_maybe_resize(table);

    return true;
}

bool walrus_hash_table_remove(Walrus_HashTable* table, void* key)
{
    return hash_table_remove_internal(table, key, true);
}

void walrus_hash_table_remove_all(Walrus_HashTable* table)
{
    hash_table_remove_all_nodes(table, true, false);
    hash_table_maybe_resize(table);
}

u32 walrus_direct_hash(void const* p)
{
    return walrus_ptr_to_u32(p);
}

u32 walrus_str_hash(void const* p)
{
    u32 h = 5381;

    for (char const* c = p; *c != '\0'; ++c) {
        h = (h << 5) + h + *c;
    }

    return h;
}

u32 walrus_i32_hash(const void* p)
{
    return *(u32*)p;
}

u32 walrus_i64_hash(void const* p)
{
    const u64* bits = p;
    return ((*bits >> 32) ^ (*bits & 0xffffffffU));
}

u32 walrus_double_hash(void const* p)
{
    const u64* bits = p;
    return ((*bits >> 32) ^ (*bits & 0xffffffffU));
}
