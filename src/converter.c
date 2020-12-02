#include <stdio.h>
#include <string.h>
#include "libs/bmpv3converter.h"
#include "libs/qdbmp/qdbmp.h"

#define error(...) (fprintf(stderr, __VA_ARGS__))
#define MINE_PARAMETER_STRING_PATTERN "--mine"
#define THEIRS_PARAMETER_STRING_PATTERN "--theirs"

int convert_with_mine_lib(char* input_filename, char* output_filename) {
    BMPV3IMAGE* bmpv3image = decode_bmpv3file(input_filename);
    if(bmpv3image == NULL) {
        switch(BMPV3FILEDECODEERROR) {
            case NULLFILEPTR:
                error("Input file must exist");
                return -1;
            case TYPEERROR:
                error("First 2 bytes not valid");
                return -2;
            case BROKENFILEERROR:
                error("Broken file (one of reserved fields is not valid)");
                return -2;
            case SIZEERROR:
                error("Size in meta header != real size, or size < 54 bytes");
                return -2;
            case VERSIONERROR:
                error("File must be BMP of 3rd version");
                return -2;
            case BITSPERPXLERROR:
                error("Color depth must be 8 or 24 pixels");
                return -2;
            case COMPRESSIONMETHODERROR:
                error("BMP must be not compressed");
                return -2;
            case BITMAPSIZEERROR:
                error("Bitmap size in meta header != expected size from pixel offset");
                return -2;
            case READERROR:
                error("Error with reading bmp file");
                return -2;
            default:
                error("Unknown error");
                return -1;
        }
    }
    make_bmpv3image_negative(bmpv3image);
    save_bmpv3image(output_filename, bmpv3image);
    if(BMPV3FILEDECODEERROR == WRITEERROR) {
        error("Error with writing new file");
        return -1;
    }
    free_bmpv3image(bmpv3image);
    return 0;
}

int convert_with_other_lib(char* input_filename, char* output_filename) {
    BMP* bmp_image = BMP_ReadFile(input_filename);
    if(BMP_LAST_ERROR_CODE != BMP_OK)
        return -3;
    if(bmp_image->Header.BitsPerPixel == 8) {
        unsigned char r, g, b;
        for(unsigned int i = 0; i < bmp_image->Header.ColorsUsed; i++) {
            BMP_GetPaletteColor(bmp_image, i, &r, &g, &b);
            if(BMP_LAST_ERROR_CODE != BMP_OK)
                return -3;
            BMP_SetPaletteColor(bmp_image, i, 255 - r, 255 - g, 255 - b);
        }
    }
    else if(bmp_image->Header.BitsPerPixel == 24) {
        unsigned char r, g, b;
        for(unsigned int x = 0; x < bmp_image->Header.Width; x++) {
            for(unsigned int y = 0; y < bmp_image->Header.Height; y++) {
                BMP_GetPixelRGB(bmp_image, x, y, &r, &g, &b);
                if(BMP_LAST_ERROR_CODE != BMP_OK)
                    return -3;
                BMP_SetPixelRGB(bmp_image, x, y, 255 - r, 255 - g, 255 - b);
            }
        }
    }
    else
        return -3;
    BMP_WriteFile(bmp_image, output_filename);
    if(BMP_LAST_ERROR_CODE != BMP_OK)
        return -3;
    BMP_Free(bmp_image);
    return 0;
}

int validate_filename(char* filename) {
    char* p = strstr(filename, ".bmp");
    if(p == NULL || *(p + 4) != '\0')
        return 0;
    else
        return 1;
}

int main(int argc, char* argv[]) {
    if(argc < 4) {
        error("3 arguments expected");
        return -1;
    }
    if(validate_filename(argv[2]) == 0) {
        error("Input file must be <name>.bmp");
        return -1;
    }
    if(validate_filename(argv[3]) == 0) {
        error("Output filename must be <name>.bmp");
        return -1;
    }
    int return_code;
    if(strcmp(argv[1], MINE_PARAMETER_STRING_PATTERN) == 0) {
        return_code = convert_with_mine_lib(argv[2], argv[3]);
    }
    else if(strcmp(argv[1], THEIRS_PARAMETER_STRING_PATTERN) == 0) {
        return_code = convert_with_other_lib(argv[2], argv[3]);
        if(return_code == -3)
            error("Unknown error in other lib");
    }
    else {
        error("Not valid conversation method (must be --theirs or --mine)");
        return_code = -1;
    }

    return return_code;
}
