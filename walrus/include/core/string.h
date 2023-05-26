#pragma once

#include "type.h"

typedef struct {
    char const *str;
    u64         len;
} Walrus_StringView;

bool walrus_str_equal(void const *s1, void const *s2);

// Allocate from a null-termiate string
char *walrus_str_dup(char const *str);

// Allocate a substr from a null-termiate string
char *walrus_str_substr(char const *str, u32 start, u64 len);

// Allocate an empty null-termiate string of len
char *walrus_str_alloc(u64 len);

// Free a allocated string
void walrus_str_free(char *str);

// Get the length of a string, the string must be either allocated by walrus_alloc_str() or NULL-terminated
u64 walrus_str_len(char const *str);

// Resize the capacity of the string
bool walrus_str_resize(char **pstr, u64 len);

// Increase the len of str
bool walrus_str_skip(char *str, u64 len);

// Append entire src to the end of dst, dst must be allocated by walrus_alloc_str()
void walrus_str_append(char **pdst, char const *src);

// Append substring of src to the end of dst, dst must be allocated by walrus_alloc_str()
void walrus_str_nappend(char **pdst, char const *src, u64 src_len);

Walrus_StringView walrus_str_substrview(char const *str, u32 start, u64 len);
