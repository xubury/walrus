#pragma once

#include "type.h"

bool walrus_str_equal(char const *s1, char const *s2);

// Allocate from a null-termiate string
char *walrus_str_alloc(char const *str);

// Allocate a substr from a null-termiate string
char *walrus_str_alloc_sub(char const *str, u64 len);

// Allocate an empty null-termiate string of capacity size
char *walrus_str_alloc_empty(u64 size);

// Free a allocated string, return false if it is malformed
bool walrus_str_free(char *str);

// Get the length of a string, the string must be either allocated by walrus_alloc_str() or NULL-terminated
u64 walrus_str_len(char const *str);

// Resize the capacity of the string
bool walrus_str_resize(char **pstr, u64 len);

// Append entire src to the end of dst, dst must be allocated by walrus_alloc_str()
void walrus_str_append(char **pdst, const char *src);

// Append substring of src to the end of dst, dst must be allocated by walrus_alloc_str()
void walrus_str_nappend(char **pdst, const char *src, u64 src_len);
