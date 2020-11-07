#include <stdio.h>
#include "libs/bmpv3converter.h"

#define error(...) (fprintf(stderr, __VA_ARGS__))
#define MISMATCH_ARRAY_MAX_SIZE 100

int validate_filename(char* filename) {
    char* p = strstr(filename, ".bmp");
    if(p == NULL || *(p + 4) != '\0')
        return 0;
    else
        return 1;
}

int check_decode_error() {
    switch(BMPV3FILEDECODEERROR) {
        case NULLFILEPTR:
            error("Input file must exist");
            return -1;
        case TYPEERROR:
            error("First 2 bytes not valid");
            return -1;
        case BROKENFILEERROR:
            error("Broken file (one of reserved fields is not valid)");
            return -1;
        case SIZEERROR:
            error("Size in meta header != real size, or size < 54 bytes");
            return -1;
        case VERSIONERROR:
            error("File must be BMP of 3rd version");
            return -1;
        case BITSPERPXLERROR:
            error("Color depth must be 8 or 24 pixels");
            return -1;
        case COMPRESSIONMETHODERROR:
            error("BMP must be not compressed");
            return -1;
        case BITMAPSIZEERROR:
            error("Bitmap size in meta header != expected size from pixel offset");
            return -1;
        case READERROR:
            error("Error with reading bmp file");
            return -1;
        case WRITEERROR:
            error("Error with writing new file");
            return -1;
        default:
            return 0;
    }
}

int main(int argc, char* argv[]) {

    if(argc < 3) {
        error("Expected 3 arguments");
        return -1;
    }
    if(validate_filename(argv[1]) == 0) {
        error("File must be <name>.bmp");
        return -1;
    }
    if(validate_filename(argv[2]) == 0) {
        error("File must be <name>.bmp");
        return -1;
    }
    BMPV3IMAGE* bmpv3image1 = decode_bmpv3file(argv[1]);
    if(check_decode_error() != 0)
        return -1;
    BMPV3IMAGE* bmpv3image2 = decode_bmpv3file(argv[2]);
    if(check_decode_error() != 0)
        return -1;
    int mismatch_array[MISMATCH_ARRAY_MAX_SIZE], size;
    size = compare_bmpv3image(*bmpv3image1, *bmpv3image2, mismatch_array, MISMATCH_ARRAY_MAX_SIZE);
    if(size < 0) {
        switch(size) {
            case -1:
                error("Bits per pixel mismatch");
                break;
            case -2:
                error("Width mismatch");
                break;
            case -3:
                error("Height mismatch");
                break;
            default:
                error("Unknown error");
                break;
        }
        return -1;
    }
    else if(size > 0) {
        for(int i = 0; i < size; i++) {
            int x = mismatch_array[i] % bmpv3image1->header_meta->width;
            int y = mismatch_array[i] / bmpv3image1->header_meta->width;
            error("%d %d\n", x + 1, y + 1);
        }
        return size;
    }

    return 0;
}
