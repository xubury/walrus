#include <core/image.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

Walrus_ImageResult walrus_image_load_from_file(Walrus_Image *img, char const *filename)
{
    i32 width, height;

    img->data = stbi_load(filename, &width, &height, NULL, 4);

    if (img->data) {
        img->width  = width;
        img->height = height;
    }
    else {
        img->width  = 0;
        img->height = 0;

        return WR_IMAGE_LOAD_ERROR;
    }

    return WR_IMAGE_SUCCESS;
}

void walrus_image_shutdown(Walrus_Image *img)
{
    if (img->data) {
        stbi_image_free(img->data);
    }
}
