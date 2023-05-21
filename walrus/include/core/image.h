#pragma once

#include "type.h"

typedef struct {
    u32   width;
    u32   height;
    u32   channel;
    void *data;
} Walrus_Image;

typedef enum {
    WR_IMAGE_SUCCESS = 0,
    WR_IMAGE_LOAD_ERROR,

    WR_IMAGE_UNKNOWN_ERORR = -1
} Walrus_ImageResult;

Walrus_ImageResult walrus_image_load_from_file_full(Walrus_Image *img, char const *filename, u32 desired_channels);
Walrus_ImageResult walrus_image_load_from_file(Walrus_Image *img, char const *filename);
Walrus_ImageResult walrus_image_load_from_memory_full(Walrus_Image *img, void *data, u64 size, u32 desired_channels);
Walrus_ImageResult walrus_image_load_from_memory(Walrus_Image *img, void *data, u64 size);

void walrus_image_shutdown(Walrus_Image *img);
