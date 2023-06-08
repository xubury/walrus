#include <core/sort.h>
#include <core/memory.h>

#include <string.h>
void swap_byte(u8* a, u8* b)
{
    u8 tmp = *a;
    *a     = *b;
    *b     = tmp;
}

void swap(void* a, void* b, u64 num_bytes)
{
    u8*       lhs = (u8*)a;
    u8*       rhs = (u8*)b;
    u8 const* end = rhs + num_bytes;
    while (rhs != end) {
        swap_byte(lhs++, rhs++);
    }
}

static void quick_sort(void* pivot, void* ptr, u32 num, u32 stride, ComparisonFn fn)
{
    if (2 > num) {
        return;
    }

    memcpy(pivot, ptr, stride);

    u8* data = (u8*)ptr;

    u32 ll = 0;
    u32 gg = 1;

    for (u32 ii = 1; ii < num;) {
        i32 result = fn(&data[ii * stride], pivot);
        if (result < 0) {
            swap(&data[ll * stride], &data[ii * stride], stride);
            ++ll;
        }
        else if (result == 0) {
            swap(&data[gg * stride], &data[ii * stride], stride);
            ++gg;
            ++ii;
        }
        else {
            ++ii;
        }
    }

    quick_sort(pivot, &data[0], ll, stride, fn);
    quick_sort(pivot, &data[gg * stride], num - gg, stride, fn);
}

void walrus_quick_sort(void* data, u32 num, u32 stride, ComparisonFn fn)
{
    u8* pivot = (u8*)walrus_alloca(stride);
    quick_sort(pivot, data, num, stride, fn);
}

#define RADIX_SORT_BITS           (11)
#define RADIX_SORT_HISTOGRAM_SIZE (1 << RADIX_SORT_BITS)
#define RADIX_SORT_BIT_MASK       (RADIX_SORT_HISTOGRAM_SIZE - 1)

void walrus_radix_sort(u32* keys, u32* temp_keys, void* values, void* temp_values, u32 size, u32 element_size)
{
    u32*  _keys        = keys;
    u32*  _temp_keys   = temp_keys;
    void* _values      = values;
    void* _temp_values = temp_values;

    u32 histogram[RADIX_SORT_HISTOGRAM_SIZE];
    u16 shift = 0;
    u32 pass  = 0;
    for (; pass < 3; ++pass) {
        memset(histogram, 0, sizeof(u32) * RADIX_SORT_HISTOGRAM_SIZE);

        bool sorted = true;
        {
            u32 key      = _keys[0];
            u32 prev_key = key;
            for (u32 ii = 0; ii < size; ++ii, prev_key = key) {
                key       = _keys[ii];
                u16 index = (key >> shift) & RADIX_SORT_BIT_MASK;
                ++histogram[index];
                sorted &= prev_key <= key;
            }
        }

        if (sorted) {
            break;
        }

        u32 offset = 0;
        for (u32 ii = 0; ii < RADIX_SORT_HISTOGRAM_SIZE; ++ii) {
            u32 count     = histogram[ii];
            histogram[ii] = offset;
            offset += count;
        }

        for (u32 ii = 0; ii < size; ++ii) {
            u32 key          = _keys[ii];
            u16 index        = (key >> shift) & RADIX_SORT_BIT_MASK;
            u32 dest         = histogram[index]++;
            _temp_keys[dest] = key;
            memcpy((u8*)_temp_values + dest * element_size, (u8*)_values + ii * element_size, element_size);
        }

        u32* swap_keys = _temp_keys;
        _temp_keys     = _keys;
        _keys          = swap_keys;

        void* swap_values = _temp_values;
        _temp_values      = _values;
        _values           = swap_values;

        shift += RADIX_SORT_BITS;
    }

    if (0 != (pass & 1)) {
        // Odd number of passes needs to do copy to the destination.
        memcpy(keys, temp_keys, size * sizeof(u32));
        for (u32 ii = 0; ii < size; ++ii) {
            memcpy((u8*)values + ii * element_size, (u8*)temp_values + ii * element_size, element_size);
        }
    }
}

void walrus_radix_sort64(u64* keys, u64* temp_keys, void* values, void* temp_values, u32 size, u32 element_size)
{
    u64*  _keys        = keys;
    u64*  _temp_keys   = temp_keys;
    void* _values      = values;
    void* _temp_values = temp_values;

    u32 histogram[RADIX_SORT_HISTOGRAM_SIZE];
    u16 shift = 0;
    u32 pass  = 0;
    for (; pass < 6; ++pass) {
        memset(histogram, 0, sizeof(u32) * RADIX_SORT_HISTOGRAM_SIZE);

        bool sorted = true;
        {
            u64 key     = _keys[0];
            u64 prevKey = key;
            for (u32 ii = 0; ii < size; ++ii, prevKey = key) {
                key       = _keys[ii];
                u16 index = (key >> shift) & RADIX_SORT_BIT_MASK;
                ++histogram[index];
                sorted &= prevKey <= key;
            }
        }

        if (sorted) {
            break;
        }

        u32 offset = 0;
        for (u32 ii = 0; ii < RADIX_SORT_HISTOGRAM_SIZE; ++ii) {
            u32 count     = histogram[ii];
            histogram[ii] = offset;
            offset += count;
        }

        for (u32 ii = 0; ii < size; ++ii) {
            u64 key          = _keys[ii];
            u16 index        = (key >> shift) & RADIX_SORT_BIT_MASK;
            u32 dest         = histogram[index]++;
            _temp_keys[dest] = key;
            memcpy((u8*)_temp_values + dest * element_size, (u8*)_values + ii * element_size, element_size);
        }

        u64* swap_keys = _temp_keys;
        _temp_keys     = _keys;
        _keys          = swap_keys;

        void* swap_values = _temp_values;
        _temp_values      = _values;
        _values           = swap_values;

        shift += RADIX_SORT_BITS;
    }

    if (0 != (pass & 1)) {
        // Odd number of passes needs to do copy to the destination.
        memcpy(keys, temp_keys, size * sizeof(u64));
        for (u32 ii = 0; ii < size; ++ii) {
            memcpy((u8*)values + ii * element_size, (u8*)temp_values + ii * element_size, element_size);
        }
    }
}
