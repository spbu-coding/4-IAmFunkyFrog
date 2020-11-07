//
// Created by trefi on 26.10.2020.
//
#include "bmpv3converter.h"

enum BmpV3DecodeError BMPV3FILEDECODEERROR = NOERROR;

//Приватная функция
void reverse_pixel_array(int* array, int bits_per_pxl, int width, int height) {
    int bytes_per_pxl = bits_per_pxl / 8;
    int aligning = (4 - (bytes_per_pxl * width) % 4) % 4;
    int byte_length_to_swap = (int)sizeof(int) * width + aligning;
    int* tmp = (int*)malloc(byte_length_to_swap);
    for(int i = 0; i < height / 2; i++) {
        memcpy(tmp, &array[width * i], byte_length_to_swap);
        memcpy(&array[width * i], &array[width * (height - i - 1)], byte_length_to_swap);
        memcpy(&array[width * (height - i - 1)], tmp, byte_length_to_swap);
    }
}
//Приватная функция
void malloc_error() {
    fprintf(stderr, "Malloc allocation error\n");
    exit(EXIT_FAILURE);
}
//Приватная функция
int validate_bmpv3_header_meta_type(BMPV3HEADERMETA bmpv3_header_meta) {
    char possible_values[6][2] = {
            "BM", "BA", "CI", "CP", "IC", "PT"
    };
    for(unsigned int i = 0; i < 6; i++) {
        if (memcmp(&bmpv3_header_meta.type, possible_values[i], 2) == 0)
            return 1;
    }
    return 0;
}
//Подразумевается, что bmpv3file указывает на начало bmp файла, приватная функция
BMPV3HEADERMETA* decode_bmpv3_meta(FILE* bmpv3file) {
    unsigned int calculated_size;
    BMPV3HEADERMETA* bmpv3_header_meta = (BMPV3HEADERMETA*)malloc(sizeof(BMPV3HEADERMETA));
    if(bmpv3_header_meta == NULL) {
        malloc_error();
        return NULL;
    }
    if(bmpv3file == NULL) {
        free(bmpv3_header_meta);
        BMPV3FILEDECODEERROR = NULLFILEPTR;
        return NULL;
    }
    else {
        if(fseek(bmpv3file, 0, SEEK_END) != 0) {
            free(bmpv3_header_meta);
            BMPV3FILEDECODEERROR = READERROR;
            return NULL;
        }
        calculated_size = ftell(bmpv3file);
        if(fseek(bmpv3file, 0, SEEK_SET) != 0) {
            free(bmpv3_header_meta);
            BMPV3FILEDECODEERROR = READERROR;
            return NULL;
        }
    }
    //проверка типа bmp (BM, BA, CI, CP, IC, PT)
    if(fread(&bmpv3_header_meta->type, 2, 1, bmpv3file) != 1) {
        free(bmpv3_header_meta);
        BMPV3FILEDECODEERROR = READERROR;
        return NULL;
    }
    if(validate_bmpv3_header_meta_type(*bmpv3_header_meta) == 0) {
        free(bmpv3_header_meta);
        BMPV3FILEDECODEERROR = TYPEERROR;
        return NULL;
    }
    //считывание размера bmp файла из заголовка
    if(fread(&bmpv3_header_meta->size, sizeof(bmpv3_header_meta->size), 1, bmpv3file) != 1) {
        free(bmpv3_header_meta);
        BMPV3FILEDECODEERROR = READERROR;
        return NULL;
    }
    if(bmpv3_header_meta->size != calculated_size) {
        free(bmpv3_header_meta);
        BMPV3FILEDECODEERROR = SIZEERROR;
        return NULL;
    }
    //считыание 4 зарезервированных байт (должен быть 0)
    unsigned int reserved;
    if(fread(&reserved, sizeof(reserved), 1, bmpv3file) != 1) {
        free(bmpv3_header_meta);
        BMPV3FILEDECODEERROR = READERROR;
        return NULL;
    }
    if(reserved != 0) {
        free(bmpv3_header_meta);
        BMPV3FILEDECODEERROR = BROKENFILEERROR;
        return NULL;
    }
    //считывание pxl_array_offset
    if(fread(&bmpv3_header_meta->pxl_array_offset, sizeof(bmpv3_header_meta->pxl_array_offset), 1, bmpv3file) != 1) {
        free(bmpv3_header_meta);
        BMPV3FILEDECODEERROR = READERROR;
        return NULL;
    }
    //считывание версии bitmap header
    unsigned int bmp_version;
    if(fread(&bmp_version, sizeof(bmp_version), 1, bmpv3file) != 1) {
        free(bmpv3_header_meta);
        BMPV3FILEDECODEERROR = READERROR;
        return NULL;
    }
    if(bmp_version != 40) { //40 байт - флаг 3 версии BMP
        free(bmpv3_header_meta);
        BMPV3FILEDECODEERROR = VERSIONERROR;
        return NULL;
    }
    //считывание ширины и высоты bmp в пикселях
    if(fread(&bmpv3_header_meta->width, sizeof(bmpv3_header_meta->width), 1, bmpv3file) != 1) {
        free(bmpv3_header_meta);
        BMPV3FILEDECODEERROR = READERROR;
        return NULL;
    }
    if(bmpv3_header_meta->width < 0) { //если высота отрицательная, выдает ошибку
        free(bmpv3_header_meta);
        BMPV3FILEDECODEERROR = READERROR;
        return NULL;
    }
    if(fread(&bmpv3_header_meta->height, sizeof(bmpv3_header_meta->height), 1, bmpv3file) != 1) {
        free(bmpv3_header_meta);
        BMPV3FILEDECODEERROR = READERROR;
        return NULL;
    }
    if(bmpv3_header_meta->height < 0) {
        bmpv3_header_meta->signed_height = -1;
        bmpv3_header_meta->height = -bmpv3_header_meta->height;
    }
    else
        bmpv3_header_meta->signed_height = 1;
    //считывание number of color planes (должна быть единица)
    unsigned short number_of_color_planes;
    if(fread(&number_of_color_planes, sizeof(number_of_color_planes), 1, bmpv3file) != 1) {
        free(bmpv3_header_meta);
        BMPV3FILEDECODEERROR = READERROR;
        return NULL;
    }
    if(number_of_color_planes != 1) {
        free(bmpv3_header_meta);
        BMPV3FILEDECODEERROR = BROKENFILEERROR;
        return NULL;
    }
    //считывание количества бит на пиксель для хранения цвета
    if(fread(&bmpv3_header_meta->bits_per_pxl, sizeof(bmpv3_header_meta->bits_per_pxl), 1, bmpv3file) != 1) {
        free(bmpv3_header_meta);
        BMPV3FILEDECODEERROR = READERROR;
        return NULL;
    }
    if(bmpv3_header_meta->bits_per_pxl != 8 && bmpv3_header_meta->bits_per_pxl != 24) {
        free(bmpv3_header_meta);
        //программа предназначена лишь для бмп 8 и 24 бит на один пиксель
        BMPV3FILEDECODEERROR = BITSPERPXLERROR;
        return NULL;
    }
    //считывание метода компрессии
    if(fread(&bmpv3_header_meta->compression_method, sizeof(bmpv3_header_meta->compression_method), 1, bmpv3file) != 1) {
        free(bmpv3_header_meta);
        BMPV3FILEDECODEERROR = READERROR;
        return NULL;
    }
    if(bmpv3_header_meta->compression_method != 0) {
        free(bmpv3_header_meta);
        //предполагается работа только с не сжатыми бмп
        BMPV3FILEDECODEERROR = COMPRESSIONMETHODERROR;
        return NULL;
    }
    //считывание bitmap_size (может быть 0 когда данные не сжаты)
    if(fread(&bmpv3_header_meta->bitmap_size, sizeof(bmpv3_header_meta->bitmap_size), 1, bmpv3file) != 1) {
        free(bmpv3_header_meta);
        BMPV3FILEDECODEERROR = READERROR;
        return NULL;
    }
    //проверка, что размер bmp без метаданных совпадает с ожидаемым
    unsigned int calculated_bitmap_size = bmpv3_header_meta->size - bmpv3_header_meta->pxl_array_offset;
    if(calculated_bitmap_size != bmpv3_header_meta->bitmap_size) {
        free(bmpv3_header_meta);
        BMPV3FILEDECODEERROR = BITMAPSIZEERROR;
        return NULL;
    }
    //считывание разрешения бмп по горизонтали и вертикали
    if(fread(&bmpv3_header_meta->hor_resolution, sizeof(bmpv3_header_meta->hor_resolution), 1, bmpv3file) != 1) {
        free(bmpv3_header_meta);
        BMPV3FILEDECODEERROR = READERROR;
        return NULL;
    }
    if(fread(&bmpv3_header_meta->ver_resolution, sizeof(bmpv3_header_meta->ver_resolution), 1, bmpv3file) != 1) {
        free(bmpv3_header_meta);
        BMPV3FILEDECODEERROR = READERROR;
        return NULL;
    }
    //считывание количества цветов в палитре и количества важных цветов
    if(fread(&bmpv3_header_meta->number_of_colors, sizeof(bmpv3_header_meta->number_of_colors), 1, bmpv3file) != 1) {
        free(bmpv3_header_meta);
        BMPV3FILEDECODEERROR = READERROR;
        return NULL;
    }
    if(fread(&bmpv3_header_meta->number_of_important_colors, sizeof(bmpv3_header_meta->number_of_important_colors), 1, bmpv3file) != 1) {
        free(bmpv3_header_meta);
        BMPV3FILEDECODEERROR = READERROR;
        return NULL;
    }

    return bmpv3_header_meta;
}
//Подразумевается, что bmpv3file уже указывает на 1ый байт таблицы цветов, приватная функция
int* decode_bmpv3_color_table(unsigned int number_of_colors, FILE* bmpv3file) {
    if(number_of_colors == 0)
        return NULL;
    int* color_table = (int*)malloc(sizeof(int) * number_of_colors);
    if(color_table == NULL) {
        malloc_error();
        return NULL;
    }
    if(fread(color_table, 4, number_of_colors, bmpv3file) != number_of_colors) {
        free(color_table);
        BMPV3FILEDECODEERROR = READERROR;
        return NULL;
    }
    return color_table;
}
//Подразумевается, что bmpv3file уже указывает на 1 байт пиксельной матрицы в bmp файлеm, приватная функция
int* decode_bmpv3_pixel_array(BMPV3HEADERMETA* header_meta, FILE* bmpv3file) {
    int width = header_meta->width;
    int height = header_meta->height;
    int bits_per_pxl = (int)header_meta->bits_per_pxl;
    int pixels_count = height * width;
    int* pixel_array = (int*)malloc(sizeof(int) * pixels_count);
    if(pixel_array == NULL) {
        malloc_error();
        return NULL;
    }
    int bytes_per_pxl = bits_per_pxl / 8;
    int aligning = (4 - (bytes_per_pxl * width) % 4) % 4;
    for(int i = 0; i < pixels_count; i++) {
        pixel_array[i] = 0;
        if(fread(&pixel_array[i], bytes_per_pxl, 1, bmpv3file) != 1) {
            free(pixel_array);
            BMPV3FILEDECODEERROR = READERROR;
            return NULL;
        }
        if((i + 1) % width == 0) {
            if(fseek(bmpv3file, (int) aligning, SEEK_CUR) != 0) {
                free(pixel_array);
                BMPV3FILEDECODEERROR = READERROR;
                return NULL;
            }
        }
    }
    if(header_meta->signed_height == -1)
        reverse_pixel_array(pixel_array, header_meta->bits_per_pxl, width, height);
    return pixel_array;
}
//Приватная функция, возвращает 4 байта, содержащие информацию о пикселе (могут использоваться не все байты)
int get_pixel(int x, int y, BMPV3IMAGE* bmpv3image) {
    return bmpv3image->pixel_array[y * bmpv3image->header_meta->width + x];
}
//Приватная функция, устанавливает первые bmpv3image->header_meta->bits_per_pixel из value как новое значение пикселся
void set_pixel(int x, int y, int value, BMPV3IMAGE* bmpv3image) {
    bmpv3image->pixel_array[y * bmpv3image->header_meta->width + x] = value;
}
//Публичная функция
BMPV3IMAGE* decode_bmpv3file(char* filename) {
    FILE* bmpv3file = fopen(filename, "rb");
    if(bmpv3file == NULL) {
        BMPV3FILEDECODEERROR = NULLFILEPTR;
        return NULL;
    }
    BMPV3IMAGE* bmpv3image = (BMPV3IMAGE*)malloc(sizeof(BMPV3IMAGE));
    if(bmpv3image == NULL) {
        malloc_error();
        return NULL;
    }
    BMPV3HEADERMETA* bmpv3_header_meta;
    bmpv3image->header_meta = decode_bmpv3_meta(bmpv3file);
    if(BMPV3FILEDECODEERROR != NOERROR) {
        free_bmpv3image(bmpv3image);
        fclose(bmpv3file);
        return NULL;
    }
    else
        bmpv3_header_meta = bmpv3image->header_meta;
    bmpv3image->color_table = decode_bmpv3_color_table(bmpv3_header_meta->number_of_colors, bmpv3file);
    if(BMPV3FILEDECODEERROR != NOERROR) {
        free_bmpv3image(bmpv3image);
        fclose(bmpv3file);
        return NULL;
    }
    bmpv3image->pixel_array = decode_bmpv3_pixel_array(bmpv3image->header_meta, bmpv3file);
    if(BMPV3FILEDECODEERROR != NOERROR) {
        free_bmpv3image(bmpv3image);
        fclose(bmpv3file);
        return NULL;
    }

    fclose(bmpv3file);
    return bmpv3image;
}
//Публичная функция
void make_bmpv3image_negative(BMPV3IMAGE* bmpv3image) {
    if(bmpv3image->color_table == NULL) {
        for(int x = 0; x < bmpv3image->header_meta->width; x++) {
            for(int y = 0; y < bmpv3image->header_meta->height; y++)
                set_pixel(x, y, ~bmpv3image->pixel_array[y * bmpv3image->header_meta->width + x], bmpv3image);
        }
    }
    else {
        for(unsigned int i = 0; i < bmpv3image->header_meta->number_of_colors; i++)
            bmpv3image->color_table[i] = (~bmpv3image->color_table[i]) & 0x00FFFFFF;
    }
}
//Публичная функция
void save_bmpv3image(char* name, BMPV3IMAGE* bmpv3image) {
    if(bmpv3image == NULL)
        return;

    FILE* new_file = fopen(name, "wb");
    BMPV3HEADERMETA* bmpv3_header_meta = bmpv3image->header_meta;
    //Запись типа bmp файла
    if(fwrite(&bmpv3_header_meta->type, sizeof(bmpv3_header_meta->type), 1, new_file) != 1) {
        fclose(new_file);
        BMPV3FILEDECODEERROR = WRITEERROR;
        return;
    }
    //Запись размера bmp файла
    if(fwrite(&bmpv3_header_meta->size, sizeof(bmpv3_header_meta->size), 1, new_file) != 1) {
        fclose(new_file);
        BMPV3FILEDECODEERROR = WRITEERROR;
        return;
    }
    //Запись резервных нулей
    int reserved = 0;
    if(fwrite(&reserved, 4, 1, new_file) != 1) {
        fclose(new_file);
        BMPV3FILEDECODEERROR = WRITEERROR;
        return;
    }
    //Запись pxl_array_offset
    if(fwrite(&bmpv3_header_meta->pxl_array_offset, sizeof(bmpv3_header_meta->pxl_array_offset), 1, new_file) != 1) {
        fclose(new_file);
        BMPV3FILEDECODEERROR = WRITEERROR;
        return;
    }
    //Запись версии заголовка (40 байт для Bmp 3)
    unsigned int bmpv3_bite_key = 40;
    if(fwrite(&bmpv3_bite_key, 4, 1, new_file) != 1) {
        fclose(new_file);
        BMPV3FILEDECODEERROR = WRITEERROR;
        return;
    }
    //Запись ширины и высоты
    if(fwrite(&bmpv3_header_meta->width, sizeof(bmpv3_header_meta->width), 1, new_file) != 1) {
        fclose(new_file);
        BMPV3FILEDECODEERROR = WRITEERROR;
        return;
    }
    int real_height = bmpv3_header_meta->height * bmpv3_header_meta->signed_height;
    if(fwrite(&real_height, sizeof(bmpv3_header_meta->height), 1, new_file) != 1) {
        fclose(new_file);
        BMPV3FILEDECODEERROR = WRITEERROR;
        return;
    }
    //Запись number of color planes (должна быть 1 типа unsigned short)
    unsigned short number_of_color_planes = 1;
    if(fwrite(&number_of_color_planes, 2, 1, new_file) != 1) {
        fclose(new_file);
        BMPV3FILEDECODEERROR = WRITEERROR;
        return;
    }
    //Запись bits_per_pxl
    if(fwrite(&bmpv3_header_meta->bits_per_pxl, sizeof(bmpv3_header_meta->bits_per_pxl), 1, new_file) != 1) {
        fclose(new_file);
        BMPV3FILEDECODEERROR = WRITEERROR;
        return;
    }
    //Запись метода компрессии (0 в нашем случае)
    if(fwrite(&bmpv3_header_meta->compression_method, sizeof(bmpv3_header_meta->compression_method), 1, new_file) != 1) {
        fclose(new_file);
        BMPV3FILEDECODEERROR = WRITEERROR;
        return;
    }
    //Запись bitmap_size
    if(fwrite(&bmpv3_header_meta->bitmap_size, sizeof(bmpv3_header_meta->bitmap_size), 1, new_file) != 1) {
        fclose(new_file);
        BMPV3FILEDECODEERROR = WRITEERROR;
        return;
    }
    //Запись горизонтального и вртикального разрешения
    if(fwrite(&bmpv3_header_meta->hor_resolution, sizeof(bmpv3_header_meta->hor_resolution), 1, new_file) != 1) {
        fclose(new_file);
        BMPV3FILEDECODEERROR = WRITEERROR;
        return;
    }
    if(fwrite(&bmpv3_header_meta->ver_resolution, sizeof(bmpv3_header_meta->ver_resolution), 1, new_file) != 1) {
        fclose(new_file);
        BMPV3FILEDECODEERROR = WRITEERROR;
        return;
    }
    //Запись количества цветов в цветовой палитре
    if(fwrite(&bmpv3_header_meta->number_of_colors, sizeof(bmpv3_header_meta->number_of_colors), 1, new_file) != 1) {
        fclose(new_file);
        BMPV3FILEDECODEERROR = WRITEERROR;
        return;
    }
    if(fwrite(&bmpv3_header_meta->number_of_important_colors, sizeof(bmpv3_header_meta->number_of_important_colors), 1, new_file) != 1) {
        fclose(new_file);
        BMPV3FILEDECODEERROR = WRITEERROR;
        return;
    }
    //Запись таблицы цветов
    if(fwrite(bmpv3image->color_table, 4, bmpv3_header_meta->number_of_colors, new_file) != bmpv3_header_meta->number_of_colors) {
        fclose(new_file);
        BMPV3FILEDECODEERROR = WRITEERROR;
        return;
    }
    //Запись пиксельного массива
    int width = bmpv3_header_meta->width;
    int height = bmpv3_header_meta->height;
    int bytes_per_pxl = (int)bmpv3_header_meta->bits_per_pxl / 8;
    int aligning = (4 - (bytes_per_pxl * width) % 4) % 4;
    int aligning_zero = 0;
    if(bmpv3_header_meta->signed_height == -1)
        reverse_pixel_array(bmpv3image->pixel_array, bmpv3_header_meta->bits_per_pxl, width, height);
    for(int i = 0; i < height * width; i++) {
        if(fwrite(&bmpv3image->pixel_array[i], bytes_per_pxl, 1, new_file) != 1) {
            fclose(new_file);
            BMPV3FILEDECODEERROR = WRITEERROR;
            return;
        }
        if((i + 1) % bmpv3_header_meta->width == 0 && aligning > 0) {
            if(fwrite(&aligning_zero, aligning, 1, new_file) != 1) {
                fclose(new_file);
                BMPV3FILEDECODEERROR = WRITEERROR;
                return;
            }
        }
    }
    fclose(new_file);
}
//Публичная функция
void free_bmpv3image(BMPV3IMAGE* bmpv3image) {
    free(bmpv3image->header_meta);
    free(bmpv3image->pixel_array);
    free(bmpv3image->color_table);
    free(bmpv3image);
    bmpv3image = NULL;
}
/*Публичная функция, 0 - код удачного сравнения, <0 - код неудачного,
 * >0 - код неудачного сравнения, указывающий на количество несовпадающих точек в out_dismatch_array
 * */
int compare_bmpv3image(BMPV3IMAGE bmpv3Image1, BMPV3IMAGE bmpv3Image2, int* out_mismatch_array, int mismatch_array_size) {
    int pixel_bitmask;
    if(bmpv3Image1.header_meta->bits_per_pxl != bmpv3Image2.header_meta->bits_per_pxl)
        return -1;
    else {
        switch (bmpv3Image1.header_meta->bits_per_pxl) {
            case 8:
                pixel_bitmask = 0x000000FF;
                break;
            case 24:
                pixel_bitmask = 0x00FFFFFF;
                break;
            default:
                return 0;
        }
    }
    if(bmpv3Image1.header_meta->width != bmpv3Image2.header_meta->width)
        return -2;
    if(bmpv3Image1.header_meta->height != bmpv3Image2.header_meta->height)
        return -3;
    int width = bmpv3Image1.header_meta->width;
    int height = bmpv3Image1.header_meta->height;
    int mismatch_iterator = 0;

    for(int x = 0; x < width; x++) {
        for(int y = 0; y < height; y++) {
            if(mismatch_iterator == mismatch_array_size)
                return mismatch_iterator;
            if((get_pixel(x, y, &bmpv3Image1) & pixel_bitmask) != (get_pixel(x, y, &bmpv3Image2) & pixel_bitmask)) {
                if(bmpv3Image1.header_meta->signed_height == 1)
                    out_mismatch_array[mismatch_iterator++] = y * width + x;
                else
                    out_mismatch_array[mismatch_iterator++] = (height - y - 1) * width + x;
            }
        }
    }
    if(mismatch_iterator == 0)
        return 0;
    else
        return mismatch_iterator;
}