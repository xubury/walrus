#include <core/image.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

Walrus_ImageResult walrus_image_load_from_file_full(Walrus_Image *img, char const *filename, u32 desired_channels)
{
    i32 width, height, channels;

    img->data = stbi_load(filename, &width, &height, &channels, desired_channels);

    if (img->data) {
        img->width   = width;
        img->height  = height;
        img->channel = channels;
    }
    else {
        img->width   = 0;
        img->height  = 0;
        img->channel = 0;

        return WR_IMAGE_LOAD_ERROR;
    }

    return WR_IMAGE_SUCCESS;
}

Walrus_ImageResult walrus_image_load_from_file(Walrus_Image *img, char const *filename)
{
    return walrus_image_load_from_file_full(img, filename, 0);
}

void walrus_image_shutdown(Walrus_Image *img)
{
    if (img->data) {
        stbi_image_free(img->data);
    }
}
