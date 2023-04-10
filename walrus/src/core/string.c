#include <core/string.h>
#include <core/memory.h>
#include <core/macro.h>

#include <stdlib.h>
#include <string.h>

bool walrus_str_equal(char const *s1, char const *s2)
{
    return strcmp(s1, s2) == 0;
}

#define STR_MAGIC_NUMBER 0xfeabcd

typedef struct {
    u64 len;           // length of the string, excluding NULL
    u64 capacity;      // the max character capacity of the string
    u32 magic_number;  // validation magic number
} Walrus_StringHeader;

static Walrus_StringHeader const *get_const_header(char const *str)
{
    return (Walrus_StringHeader const *)(str - sizeof(Walrus_StringHeader));
}

static Walrus_StringHeader *get_header(char const *str)
{
    return (Walrus_StringHeader *)(str - sizeof(Walrus_StringHeader));
}

static bool check_str_valid(char const *str)
{
    return str && get_const_header(str)->magic_number == STR_MAGIC_NUMBER;
}

char *walrus_str_dup(char const *str)
{
    return walrus_str_substr(str, 0, walrus_str_len(str));
}

char *walrus_str_substr(char const *str, i32 start, u64 len)
{
    char *alloc_str = walrus_str_alloc(len);
    walrus_str_nappend(&alloc_str, str + start, len);

    return alloc_str;
}

static char *str_storage_realloc(void *ptr, u64 size)
{
    u64 len = 0;

    if (ptr) {
        len = walrus_min(size, ((Walrus_StringHeader *)(ptr))->len);
    }
    char                *mem    = walrus_realloc(ptr, (size + 1) * sizeof(char) + sizeof(Walrus_StringHeader));
    Walrus_StringHeader *header = (Walrus_StringHeader *)mem;
    char                *str    = mem + sizeof(Walrus_StringHeader);

    str[len]             = 0;
    header->len          = len;
    header->capacity     = size;
    header->magic_number = STR_MAGIC_NUMBER;

    return str;
}

char *walrus_str_alloc(u64 size)
{
    return str_storage_realloc(NULL, size);
}

bool walrus_str_free(char *str)
{
    if (check_str_valid(str)) {
        walrus_free(str - sizeof(Walrus_StringHeader));

        return true;
    }

    return false;
}

u64 walrus_str_len(char const *str)
{
    if (check_str_valid(str)) {
        return get_const_header(str)->len;
    }
    else {
        return strlen(str);
    }
}

bool walrus_str_resize(char **pstr, u64 size)
{
    char *str = *pstr;
    if (!check_str_valid(str)) {
        return false;
    }

    Walrus_StringHeader const *header = get_const_header(str);
    char                      *ptr    = (char *)header;

    if (header->capacity != size) {
        str = str_storage_realloc(ptr, size);
    }

    *pstr = str;

    return true;
}

void walrus_str_append(char **pdst, char const *src)
{
    u64 const src_len = walrus_str_len(src);
    walrus_str_nappend(pdst, src, src_len);
}

void walrus_str_nappend(char **pdst, char const *src, u64 src_len)
{
    char *dst = *pdst;
    if (!check_str_valid(dst)) {
        return;
    }

    Walrus_StringHeader const *dst_header = get_const_header(dst);
    u64 const                  dst_len    = dst_header->len;
    u64 const                  new_len    = dst_len + src_len;

    bool can_append = new_len <= dst_header->capacity;
    if (!can_append) {
        can_append   = walrus_str_resize(&dst, new_len);
        dst[new_len] = 0;
    }

    if (can_append) {
        memcpy(dst + dst_len, src, src_len);
        get_header(dst)->len = new_len;
    }

    *pdst = dst;
}

Walrus_StringView walrus_str_substrview(char const *str, i32 start, u64 len)
{
    Walrus_StringView string_view;
    string_view.str = NULL;
    string_view.len = 0;
    if (start > 0) {
        i32 str_len   = strlen(str);
        start         = walrus_min(start, str_len);
        u64 avail_len = str_len - start;

        string_view.str = str + start;
        string_view.len = walrus_min(avail_len, len);
    }

    return string_view;
}
