#pragma once

#include "type.h"

typedef struct {
    u32   width;
    u32   height;
    void *data;
} Walrus_Image;

typedef enum {
    WR_IMAGE_SUCCESS = 0,
    WR_IMAGE_LOAD_ERROR,

    WR_IMAGE_UNKNOWN_ERORR = -1
} Walrus_ImageResult;

Walrus_ImageResult walrus_image_load_from_file(Walrus_Image *img, char const *filename);

void walrus_image_shutdown(Walrus_Image *img);
