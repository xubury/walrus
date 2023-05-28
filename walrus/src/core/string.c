#include <core/string.h>
#include <core/memory.h>
#include <core/assert.h>
#include <core/math.h>

#include <stdlib.h>
#include <string.h>

bool walrus_str_equal(void const *s1, void const *s2)
{
    return strcmp(s1, s2) == 0;
}

#define STR_MAGIC_NUMBER 0xfeabcd

typedef struct {
    u64 len;           // length of the string, excluding NULL
    u64 capacity;      // the max character capacity of the string
    u32 magic_number;  // validation magic number
} StringHeader;

static StringHeader const *get_const_header(char const *str)
{
    return (StringHeader const *)(str - sizeof(StringHeader));
}

static StringHeader *get_header(char const *str)
{
    return (StringHeader *)(str - sizeof(StringHeader));
}

static bool check_str_valid(char const *str)
{
    return str && get_const_header(str)->magic_number == STR_MAGIC_NUMBER;
}

char *walrus_str_dup(char const *str)
{
    return walrus_str_substr(str, 0, walrus_str_len(str));
}

char *walrus_str_substr(char const *str, u32 start, u64 len)
{
    char *alloc_str = walrus_str_alloc(len);
    u64   max_len   = walrus_str_len(str);
    walrus_str_nappend(&alloc_str, str + start, walrus_min(len, max_len - start));

    return alloc_str;
}

static char *str_storage_realloc(void *ptr, u64 len)
{
    u64 capacity = len + 1;
    if (ptr) {
        len = walrus_min(len, ((StringHeader *)(ptr))->len);
    }
    else {
        len = 0;
    }

    char         *mem    = walrus_realloc(ptr, capacity * sizeof(char) + sizeof(StringHeader));
    StringHeader *header = (StringHeader *)mem;
    char         *str    = mem + sizeof(StringHeader);

    str[len]             = 0;
    header->len          = len;
    header->capacity     = capacity;
    header->magic_number = STR_MAGIC_NUMBER;

    return str;
}

char *walrus_str_alloc(u64 len)
{
    return str_storage_realloc(NULL, len);
}

void walrus_str_free(char *str)
{
    walrus_assert(check_str_valid(str));
    walrus_free(str - sizeof(StringHeader));
}

u64 walrus_str_len(char const *str)
{
    if (check_str_valid(str)) {
        StringHeader const *header = get_const_header(str);
        return header->len;
    }
    else {
        return strlen(str);
    }
}

bool walrus_str_resize(char **pstr, u64 len)
{
    char *str = *pstr;
    if (!check_str_valid(str)) {
        return false;
    }

    StringHeader const *header = get_const_header(str);
    char               *ptr    = (char *)header;

    if (header->capacity != len + 1) {
        str = str_storage_realloc(ptr, len);
    }

    *pstr = str;

    return true;
}

bool walrus_str_skip(char *str, u64 len)
{
    if (!check_str_valid(str)) {
        return false;
    }

    StringHeader *header = get_header(str);
    header->len          = walrus_min(header->len + len, header->capacity - 1);
    str[header->len]     = 0;
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

    StringHeader const *dst_header = get_const_header(dst);
    u64 const           dst_len    = dst_header->len;
    u64 const           new_len    = dst_len + src_len;

    bool can_append = new_len + 1 <= dst_header->capacity;
    if (!can_append) {
        can_append = walrus_str_resize(&dst, new_len);
    }

    if (can_append) {
        memcpy(dst + dst_len, src, src_len);
        get_header(dst)->len = new_len;
        dst[new_len]         = 0;
    }

    *pdst = dst;
}

Walrus_StringView walrus_str_substrview(char const *str, u32 start, u64 len)
{
    Walrus_StringView string_view;

    u32 str_len   = strlen(str);
    start         = walrus_min(start, str_len);
    u64 avail_len = str_len - start;

    string_view.str = str + start;
    string_view.len = walrus_min(avail_len, len);

    return string_view;
}

u64 walrus_str_first_of(char const *str, char c)
{
    char *find = strchr(str, c);
    if (find) {
        return find - str;
    }
    return WR_STRING_INVALID_POS;
}

u64 walrus_str_last_of(char const *str, char c)
{
    char *find = strrchr(str, c);
    if (find) {
        return find - str;
    }
    return WR_STRING_INVALID_POS;
}
