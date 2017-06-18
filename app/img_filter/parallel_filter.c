#include <hellfire.h>
#include <noc.h>
#include "image.h"
#include <math.h>

// CPU IDs
#define MASTER 0
#define AGGREGATOR 1

// Message flags
#define GAUSIAN 0
#define SOBEL 1

#define KEEP_ALIVE 0
#define POISON_PILL 3

// Block size
#define BLOCK_SIZE 36

#define MSG_SIZE 1500


uint8_t gausian(uint8_t buffer[5][5]){
    int32_t sum = 0, mpixel;
    uint8_t i, j;
    int16_t kernel[5][5] =    {    {2, 4, 5, 4, 2},
                    {4, 9, 12, 9, 4},
                    {5, 12, 15, 12, 5},
                    {4, 9, 12, 9, 4},
                    {2, 4, 5, 4, 2}
                };
    for (i = 0; i < 5; i++)
        for (j = 0; j < 5; j++)
            sum += ((int32_t)buffer[i][j] * (int32_t)kernel[i][j]);
    mpixel = (int32_t)(sum / 159);
    return (uint8_t)mpixel;
}

uint32_t isqrt(uint32_t a){
    uint32_t i, rem = 0, root = 0, divisor = 0;
    for (i = 0; i < 16; i++){
        root <<= 1;
        rem = ((rem << 2) + (a >> 30));
        a <<= 2;
        divisor = (root << 1) + 1;
        if (divisor <= rem){
            rem -= divisor;
            root++;
        }
    }
    return root;
}

uint8_t sobel(uint8_t buffer[3][3]){
    int32_t sum = 0, gx = 0, gy = 0;
    uint8_t i, j;
    int16_t kernelx[3][3] =    {    {-1, 0, 1},
                    {-2, 0, 2},
                    {-1, 0, 1},
                };
    int16_t kernely[3][3] =    {    {-1, -2, -1},
                    {0, 0, 0},
                    {1, 2, 1},
                };
    for (i = 0; i < 3; i++){
        for (j = 0; j < 3; j++){
            gx += ((int32_t)buffer[i][j] * (int32_t)kernelx[i][j]);
            gy += ((int32_t)buffer[i][j] * (int32_t)kernely[i][j]);
        }
    }
    sum = isqrt(gy * gy + gx * gx);
    if (sum > 255) sum = 255;
    if (sum < 0) sum = 0;
    return (uint8_t)sum;
}

struct Data{
    // flag, see above
    uint8_t flag;  
    // block order
    uint16_t block_line;
    uint16_t block_column; 
    // pixel block
    uint8_t pixels[BLOCK_SIZE][BLOCK_SIZE]; 
};

union Package{
    struct Data data;
    int8_t raw_data[MSG_SIZE];
};

/* Aplica o filtro. filter 0 = gaussian, filter 1 == sobel*/
void do_filter(uint8_t block[BLOCK_SIZE][BLOCK_SIZE], uint8_t filter, uint8_t result[BLOCK_SIZE][BLOCK_SIZE]){
    int32_t i, j, k, l;
    uint8_t border, block_size;

    if (filter == GAUSIAN){
        border = 2;
        block_size = 5;
    } else if (filter == SOBEL) {
        border = 1;
        block_size = 3;
    }

    uint8_t image_buf[block_size][block_size]; // internal buffer for using filters
    
    for(i = 0; i < BLOCK_SIZE; i++){
        if (i > 1 || i < height - border){
            for(j = 0; j < BLOCK_SIZE; j++){
                if (j > 1 || j < width - border){
                    for (k = 0; k < filter_block_size;k++)
                        for(l = 0; l < filter_block_size; l++)
                            image_buf[k][l] = block[k][l];
                    if (filter == GAUSIAN){
                        result[i][j] = gausian(image_buf)
                    }else{
                        result[i][j] = sobel(image_buf)
                    }
                }else{
                    result[i][j] = fragment[i][j];
                }
            }
        }
    }
}

void sanitize_block(uint8_t block[BLOCK_SIZE][BLOCK_SIZE]){
    uint16_t i, j;
    for (i = 0; i < BLOCK_SIZE; i++){
        for (j = 0; j < BLOCK_SIZE; j++){
            block[i][j] = 0;
        }
    }
}

/**
 Populate a block, leaving border of size 2
*/
void create_block(uint8_t matrix[height][width], uint16_t block_line, uint16_t block_column, uint8_t[BLOCK_SIZE][BLOCK_SIZE] block){
    uint16_t start_line = (block_line * BLOCK_SIZE);
    uint16_t start_column =  (block_column * BLOCK_SIZE);
    uint8_t i = 2, j;
    /* Leaving 2 spaces for border */
    while (i < BLOCK_SIZE-2){
        j = 2;
        while (j < BLOCK_SIZE-2){
            if (i+start_line < height && j+start_column < width){
                block[i][j] = matrix[i+start_line][j+start_column];
            }
            j++;
        }
        i++;
    }
}

void master(void){
    printf("[MASTER] Starting process...\n");
    int32_t worker, i, j, block_lines, block_columns, i, j, k, l, matrix_pos = 0;
    union Package poison_package;

    if (hf_comm_create(hf_selfid(), 1000, 0))
        panic(0xff);

    // Original image in 2D format
    uint8_t matrix[height][width];
    for (i = 0; i < height; i++)
        for (j = 0; j < width; j++)
            matrix[i][j] = image[matrix_pos++];

    // Block to be processed in the worker cpu
    uint8_t block[BLOCK_SIZE][BLOCK_SIZE];

    block_lines = ceil(height / block_size);
    block_col = ceil(width / block_size);
    for (i = 0; i < block_lines; i++){
        for (j = 0; j < block_columns; j++){
            sanitize_block(block);
            create_block(matrix, i, j, block);// create block with border 2
            /* Prepare worker message */
            union Package package;
            package.data.flag = GAUSIAN;
            package.data.block_line = i;
            package.data.block_column = j;
            for (k = 0; k <BLOCK_SIZE; k++){
                for (l = 0; l < BLOCK_SIZE; l++)
                    package.data.pixels[i][j] = block[i][j];

            hf_sendack(worker, 5000, package.raw_data, MSG_SIZE, 1, 500);
        }
    }
    // Kill workers
    poison_package.data.flag = POISON_PILL;
    for (worker = 2; worker <= 5; worker++){
        hf_sendack(worker, 5000, poison_package.raw_data, ..., 1, 500);
    }
    hf_kill(hf_selfid());
}


/**
    Receive pixel vector and applies the requested filter
    Buffer format: [task, position, [pixel_vector]]
    if task is poison_pill, then this process must die
*/
void worker(void){
    uint8_t filtered_pixel, flag, keep_alive = 1, result_block[BLOCK_SIZE][BLOCK_SIZE];
    uint16_t cpu, src_port, size,i,j, ch;
    int16_t val;
    int8_t buf[MSG_SIZE];

    union Package package;

    if (hf_comm_create(hf_selfid(), 5000, 0))
        panic(0xff);

    while (keep_alive == 1){
        ch = hf_recvprobe();
        if (ch >= 0) {
            val = hf_recvack(&cpu, &src_port, buf, &size, ch);
            if (val){
                printf("[WORKER %d] hf_recvack(): error %d\n", cpu, val);
            } else {
                // decode buffer into a union to retrieve struct
                for (i = 0; i < MSG_SIZE; i++){
                    package.raw_data[i] = buf[i];
                }
                if (package.data.flag == POISON_PILL){
                    keep_alive = 0;
                } else {
                    //do_filter gausian
                    sanitize_block(result_block);
                    do_filter(package.data.pixels, GAUSIAN, result_block);
                    do_filter(package.data.pixels, SOBEL, result_block);

                    /* Sending message to aggregator, using the same reference */
                    package.data.flag = KEEP_ALIVE; // keep aggregating
                    for (i = 0; i < BLOCK_SIZE; i++){
                        for ( j = 0; j < BLOCK_SIZE; j++){
                            package.data.pixels[i][j] = result_block[i][j];

                    //send filtered buffer, one channel for worker
                    printf("[WORKER %d] Sending pixel %d in position %d to Aggregator\n", hf_cpuid(), filtered_pixel, tgt_position);
                    val = hf_sendack(AGGREGATOR, 6000, package.raw_data, MSG_SIZE, hf_cpuid(), 500);
                    if (val)
                        printf("[WORKER %d] Error sending the message to aggregator. Val = %d \n", hf_cpuid(), val);
                }
            }
        }
    }
    package.data.flag = POISON_PILL;
    hf_sendack(AGGREGATOR, 6000, package.raw_data, MSG_SIZE, hf_cpuid(), 500);
    hf_kill(hf_selfid());
}

/**
    Aggregate the filtered pixels.
    Buffer format: [task, position, pixel value]
    if this process receive poison pill of all workers, then this process must die
*/
void aggregator(void){
    uint8_t task, poison_pills=0, remaining_workers = 4, filtered_pixel, keep_alive = 1; // cpus 2, 3, 4 and 5
    uint16_t cpu, src_port, size, channel, i, j, k, l;
    int16_t val, start_line, start_column, line, column;
    int8_t buf[MSG_SIZE];

    union Package package;
    uint8_t matrix[height][width];

    uint8_t *img;
    img = (uint8_t *) malloc(height * width);

    if (hf_comm_create(hf_selfid(), 6000, 0))
        panic(0xff);

    /* Only stops if all workers sent the poison pills */
    while (keep_alive == 1){
        channel = hf_recvprobe(); // retorna o channel,
        if (channel >= 0) {
            val = hf_recvack(&cpu, &src_port, buf, &size, channel);
            if (val){
                printf("[AGGREGATOR] hf_recvack(): error %d\n", cpu, val);
            } else {
                // decode buffer into a union to retrieve struct
                for (i = 0; i < MSG_SIZE; i++){
                    package.raw_data[i] = buf[i];
                }
                task = package.data.flag; // task type
                if (task == POISON_PILL) {
                    poison_pills++;
                }
                if (poison_pills < remaining_workers){
                    start_line = package.data.block_line * (BLOCK_SIZE-2);
                    start_column = package.data.block_column * (BLOCK_SIZE-2);
                    // receive pixels, ignoring border
                    for (i = 2; i < BLOCK_SIZE-2; i++){
                        for (j = 2; j < BLOCK_SIZE-2; j++){
                            line = start_line + (i-2);
                            column = start_line + (j-2);
                            matrix[line][column] = package.data.pixels[i][j];
                        }
                    }
                }else{
                    keep_alive = 0;
                }
            }
        }
    }

    l = 0;
    for (i = 0; i < height; i++){
        for (j = 0; j < width; j++){
            img[l] = matrix[i][j];
            l++;
        }
    }

    //print the output, just how filter.c does
    printf("\n\nint32_t width = %d, height = %d;\n", width, height);
    printf("uint8_t image[] = {\n");
    for (i = 0; i < height; i++){
        for (j = 0; j < width; j++){
            printf("0x%x", img[i * width + j]);
            if ((i < height-1) || (j < width-1)) printf(", ");
            if ((++k % 16) == 0) printf("\n");
        }
    }
    printf("};\n");

    free(img);

    printf("\n\nend of processing!\n");
    panic(0);
    hf_kill(hf_selfid());
}

/**
    Notation:
        0 - master
        2, 3,4,5 - Worker
        1 - Aggregator
*/
void app_main(void) {
    if (hf_cpuid() == MASTER){
        printf("Spawn Master...\n");
        hf_spawn(master, 0, 0, 0, "filter", 4096);
    } else if (hf_cpuid() == AGGREGATOR){
        printf("Spawn Aggregator...\n");
        hf_spawn(aggregator, 0, 0, 0, "aggregator", 4096);
    } else {
        printf("Spawn worker %d...\n", hf_cpuid());
        hf_spawn(worker, 0, 0, 0, "worker", 4096);
    }
}