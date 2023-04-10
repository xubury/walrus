#pragma once

#include "type.h"

typedef struct _Walrus_HashTable Walrus_HashTable;

typedef u32 (*Walrus_HashFunc)(void const *);
typedef bool (*Walrus_EqualFunc)(void const *, void const *);
typedef void (*Walrus_KeyDestroyFunc)(void *);
typedef void (*Walrus_ValueDestroyFunc)(void *);

Walrus_HashTable *walrus_hash_table_create(Walrus_HashFunc hash, Walrus_EqualFunc equal);
Walrus_HashTable *walrus_hash_table_create_full(Walrus_HashFunc hash, Walrus_EqualFunc equal,
                                                Walrus_KeyDestroyFunc key_destroy, Walrus_ValueDestroyFunc val_destroy);
void              walrus_hash_table_destroy(Walrus_HashTable *table);

bool walrus_hash_table_contains(Walrus_HashTable *table, void *key);

void *walrus_hash_table_lookup(Walrus_HashTable *table, void *key);

// Use hash table as a set
bool walrus_hash_table_add(Walrus_HashTable *table, void *key);

// Insert a key value pair to hash table, return false if key already exists
bool walrus_hash_table_insert(Walrus_HashTable *table, void *key, void *value);

// Remove a key value pair from hash table
bool walrus_hash_table_remove(Walrus_HashTable *table, void *key);
void walrus_hash_table_remove_all(Walrus_HashTable *table);

u32 walrus_direct_hash(void const *p);

u32 walrus_str_hash(void const *p);

u32 walrus_i32_hash(void const *p);
u32 walrus_i64_hash(void const *p);
u32 walrus_double_hash(void const *p);
