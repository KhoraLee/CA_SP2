//
//  ku_histo.c
//  SP-Asigmt02
//
//  Created by 이승윤 on 11/20/23.
//

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>
#include <pthread.h>
#include <time.h>

#include "ku_histo.h"

#define STDERR 2
#define ERROR_EXIT(MSG) \
    char *err_msg = MSG; \
    write(STDERR, err_msg, strlen(err_msg)); \
    exit(EXIT_FAILURE);

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, const char * argv[]) {
    int threads, interval, input_fd, output_fd;
    if (argc == 4){
        threads = atoi(argv[1]);
        interval = atoi(argv[2]);
        input_fd = open(argv[3], O_RDONLY);
        output_fd = open("results.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (threads < 1) {
            ERROR_EXIT("Argument error: Thread number must be bigger than 1\n");
        }
        if (256 % interval != 0) {
            ERROR_EXIT("Argument error: Interval must be divisor of 256\n");
        }
        if (input_fd == -1) {
            ERROR_EXIT("File error: Cannot open bmp file\n");
        }
        if (output_fd == -1) {
            ERROR_EXIT("File error: Cannot create result.txt file\n");
        }
        main_logic(threads, interval, input_fd, output_fd);
        return 0; // Normal return
    }
    ERROR_EXIT("Argument error.\nusage: ku_histo <thread number> <interval> <bmp file path>\n")
    
}

void main_logic(int threads, int interval, int input_fd, int output_fd) {
    // initalize result array
    int arr_len = 256 / interval;
    int arr_size = sizeof(uint32_t) * arr_len;
    uint32_t *result = malloc(arr_size);
    memset(result, 0, arr_size);

    // read bitmap header and info heade
    BITMAP_FILE_HEADER header;
    BITMAP_INFO_HEADER info;
    read(input_fd, &header, sizeof(BITMAP_FILE_HEADER));
    read(input_fd, &info, sizeof(BITMAP_INFO_HEADER));
    
    // validate input bmp file
    validate_bmp(header, info);
    
    // map file to memory
    PTR addr = mmap(NULL, header.bfSize, PROT_READ, MAP_PRIVATE, input_fd, 0);
    PTR color_table = addr + header.bfOffBits;
    
    // calc job counts for threads
    uint32_t per_thread = info.biSizeImage / threads;
    uint32_t remnant = info.biSizeImage % threads;
    uint32_t *job_count = malloc(sizeof(uint32_t) * threads);
    for (int i = 0; i < threads; i++) {
        job_count[i] = per_thread;
    }
    while (remnant > 0) {
        job_count[remnant - 1] += 1;
        remnant -= 1;
    }

#ifdef DEBUG
    // Check prior created thread do more work than next and total sum is same with image size. (Only on DEBUG build)
    int job_sum = 0;
    for (int i = 0; i < threads; i++) {
        job_sum += job_count[i];
        if (i < threads - 1) {
            assert(job_count[i] >= job_count[i + 1]);
        }
    }
    assert(job_sum == info.biSizeImage);
#endif

    // prepare thread args and create threads
    pthread_t *pids = malloc(sizeof(pthread_t) * threads);
    pARG args = malloc(sizeof(ARG) * threads);
    for (int i = 0; i < threads; i++) {
        args[i].color_table = color_table;
        args[i].pixel_count = job_count[i];
        args[i].result_array = result;
        args[i].interval = interval;
        args[i].thread_idx = i;
        args[i].thread_num = threads;
        pthread_create(pids + i, NULL, thread_job, args + i);
    }

    // wait threads to finish job and join threads
    void *thread_result;
    for (int i = 0; i < threads; i++) {
        pthread_join(pids[i], &thread_result);
        assert(thread_result == NULL);
    }
    pthread_mutex_destroy(&mutex);
    
    // write results to file
    write_result_file(output_fd, result, arr_len);

    // free memory and unmap file
    free(job_count);
    free(pids);
    free(args);
    free(result);
    munmap(addr, header.bfSize);
    return;
}

void validate_bmp(BITMAP_FILE_HEADER file, BITMAP_INFO_HEADER info) {
    // check whether it is bmp file or not
    char *bfType = (char *)&file.bfType;
    if (bfType[0] != 'B' || bfType[1] != 'M') {
        ERROR_EXIT("File is not bmp or bmp file is damaged.\n");
    }
    // check whether it is gray-scaled image or not
    if (info.biBitCount != 8) {
        ERROR_EXIT("BMP file is not gray-scaled image.\n");
    }
}

void* thread_job(void *args) {
    pARG arg = args;
    int arr_size = 256 / arg->interval;
    uint32_t *result = malloc(sizeof(uint32_t) * arr_size);
    memset(result, 0, sizeof(uint32_t) * arr_size);

    for (int i = 0; i < arg->pixel_count; i++) {
        PTR pixel = arg->color_table + arg->thread_idx + i * arg->thread_num;
        result[*pixel / arg->interval] += 1;
    }
    
    // save thread result to total result
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < arr_size; i++) {
        arg->result_array[i] += result[i];
    }
    pthread_mutex_unlock(&mutex);
    return NULL;
}

void write_result_file(int output_fd, uint32_t *array, int size) {
    int interval = 256 / size;
    for (int i = 0; i < size; i++) {
        char *buff = malloc(sizeof(char) * 10);
        sprintf(buff, "%d %d\n", i * interval, array[i]);
        write(output_fd, buff, strlen(buff));
    }
    return;
}
