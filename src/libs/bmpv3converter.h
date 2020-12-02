//
// Created by trefi on 24.10.2020.
//
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifndef HOME_2310_BMPV3CONVERTER_H
#define HOME_2310_BMPV3CONVERTER_H

enum BmpV3DecodeError {
    NOERROR,
    MALLOCERROR,
    NULLFILEPTR,
    TYPEERROR,
    BROKENFILEERROR,
    SIZEERROR,
    VERSIONERROR,
    BITSPERPXLERROR,
    COMPRESSIONMETHODERROR,
    BITMAPSIZEERROR,
    READERROR,
    WRITEERROR
};
enum BmpV3DecodeError BMPV3FILEDECODEERROR;

/* Структура, хрянящая метаданные BmpV3 файла,
 * несмотря на то, что содержит поле с описанием метода компрессии,
 * работа с пережатыми бмп не подразумевается.
 */
typedef struct BmpV3HeaderMeta {
    unsigned short type;
    unsigned int size;
    unsigned int pxl_array_offset;
    int width;
    int signed_height; //если высота отрицательная = -1, иначе 1
    int height;
    unsigned short bits_per_pxl;
    unsigned int compression_method;
    unsigned int bitmap_size;
    int hor_resolution;
    int ver_resolution;
    unsigned int number_of_colors;
    unsigned int number_of_important_colors;
} BMPV3HEADERMETA;

typedef struct BmpV3Image {
    BMPV3HEADERMETA* header_meta;
    int* color_table;
    int* pixel_array;
} BMPV3IMAGE;

BMPV3IMAGE* decode_bmpv3file(char* filename);
void make_bmpv3image_negative(BMPV3IMAGE* bmpv3image);
void save_bmpv3image(char* name, BMPV3IMAGE* bmpv3image);
void free_bmpv3image(BMPV3IMAGE* bmpv3image);
int compare_bmpv3image(BMPV3IMAGE bmpv3Image1, BMPV3IMAGE bmpv3Image2, int* out_mismatch_array, int mismatch_array_size);

#endif //HOME_2310_BMPV3CONVERTER_H