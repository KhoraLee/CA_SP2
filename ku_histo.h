//
//  ku_histo.h
//  SP-Asigmt02
//
//  Created by 이승윤 on 11/20/23.
//

#ifndef ku_histo_h
#define ku_histo_h

typedef uint32_t DWORD;
typedef uint16_t WORD;
typedef int32_t LONG;
typedef uint8_t BYTE, *PTR;

typedef struct __attribute__ ((packed)) _bitmap_file_header {
    WORD bfType; // The file type; must be BM.
    DWORD bfSize; // The size, in bytes, of the bitmap file.
    WORD bfReserved1; // Reserved; must be zero.
    WORD bfReserved2; // Reserved; must be zero.
    DWORD bfOffBits; // The offset, in bytes, from the beginning of the BITMAPFILEHEADER structure to the bitmap bits.
} BITMAP_FILE_HEADER, *pBITMAP_FILE_HEADER;

typedef struct _bitmap_info_header {
    DWORD biSize;
    LONG biWidth;
    LONG biHeight;
    WORD biPlanes;
    WORD biBitCount;
    DWORD biCompression;
    DWORD biSizeImage;
    LONG biXPelsPerMeter;
    LONG biYPelsPerMeter;
    DWORD biClrUsed;
    DWORD biClrImportant;
} BITMAP_INFO_HEADER, *pBITMAP_INFO_HEADER;

typedef struct _thread_arg {
    PTR color_table;
    uint32_t pixel_count;
    uint32_t *result_array;
    int interval;
    int thread_idx;
    int thread_num;
} ARG, *pARG;

void main_logic(int threads, int interval, int input_fd, int output_fd);
void validate_bmp(BITMAP_FILE_HEADER file, BITMAP_INFO_HEADER info);
void* thread_job(void *args);
void write_result_file(int output_fd, uint32_t *array, int size);

#endif /* ku_histo_h */
