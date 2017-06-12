#include <hellfire.h>
#include <noc.h>
#include "image.h"

#define POISON_PILL 3
#define MASTER 0
#define AGGREGATOR 1

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

void do_gausian(int32_t width, int32_t height){
    int32_t i, j, k, l, pos, worker = 2, buf_pos;
    uint8_t image_buf[27]; // 1+1+(5x5) 

    for(i = 0; i < height; i++){
        if (i > 1 || i < height-2){
            for(j = 0; j < width; j++){
                pos = ((i * width) + j); // posição da img
                image_buf[1] = pos;
                if (j > 1 || j < width-2){
                    // Create a flat buffer
                    buf_pos = 2;
                    for (k = 0; k < 5;k++)
                        for(l = 0; l < 5; l++){
                            image_buf[buf_pos] = image[(((i + l-1) * width) + (j + k-1))];
                            buf_pos++;
                        }

                    /* Delegate the buffer to the worker */
                    if (worker > 5){
                        worker = 2; //round robin
                    } 
                    val = hf_sendack(worker, 5000, image_buf, sizeof(image_buf), 1, 500);
                    worker++; //next worker
                    if (val)
                        printf("[MASTER] hf_sendack() to worker %d error: %d\n", worker, val);
                }else{    if (i > 0 || i < height-1){
            for(j = 0; j < width-1; j++){
                pos = ((i * width) + j); // posição da img
                image_buf[1] = pos;
                if (j > 0 || j < width-1){
                    // Create a flat buffer
                    buf_pos = 2;
                    for (k = 0; k < 3;k++){
                        for(l = 0; l < 3; l++){
                            image_buf[buf_pos] = image[(((i + l-1) * width) + (j + k-1))];
                            buf_pos++;
                        }
                    }
                    image_buf[0] = 0;//apply gaussian filter
                }else{
                    image_buf[0] = 2;//do nothing, just bypass the pixel
                    image_buf[2] = image[((i * width) + j)]; // bypass pixel
                }

                /* Delegate the buffer to the worker */
                if (worker > 5){
                    worker = 2; //round robin
                } 
                val = hf_sendack(worker, 5000, image_buf, sizeof(image_buf), 1, 500);
                worker++; //next worker
                if (val)
                    printf("[MASTER] hf_sendack() to worker %d error: %d\n", worker, val);
            }
        }
                    image_buf[0] = 2;//do nothing, just bypass the pixel
                    image_buf[2] = image[((i * width) + j)]; // bypass pixel
                }
            }
        }
    }
}

void do_sobel(int32_t width, int32_t height){
    int32_t i, j, k, l, pos, worker = 2, buf_pos;
    int16_t val;
    uint8_t image_buf[11]; // 1+1+(3x3) 

    for(i = 0; i < height; i++){
        if (i > 0 || i < height-1){
            for(j = 0; j < width-1; j++){
                pos = ((i * width) + j); // posição da img
                image_buf[1] = pos;
                if (j > 0 || j < width-1){
                    // Create a flat buffer
                    buf_pos = 2;
                    for (k = 0; k < 3;k++){
                        for(l = 0; l < 3; l++){
                            image_buf[buf_pos] = image[(((i + l-1) * width) + (j + k-1))];
                            buf_pos++;
                        }
                    }
                    image_buf[0] = 0;//apply gaussian filter
                }else{
                    image_buf[0] = 2;//do nothing, just bypass the pixel
                    image_buf[2] = image[((i * width) + j)]; // bypass pixel
                }

                /* Delegate the buffer to the worker */
                if (worker > 5){
                    worker = 2; //round robin
                } 
                val = hf_sendack(worker, 5000, image_buf, sizeof(image_buf), 1, 500);
                worker++; //next worker
                if (val)
                    printf("[MASTER] hf_sendack() to worker %d error: %d\n", worker, val);
            }
        }
    }

}

void master(void){
    int32_t msg = 0, worker;
    int8_t buf[2];
    int16_t val, channel;

    if (hf_comm_create(hf_selfid(), 1000, 0))
        panic(0xff);

    do_gausian(width, height);
    do_sobel(width, height);
    buf[0] = POISON_PILL;
    for (worker = 2; worker <=5; worker++){
        hf_sendack(worker, 5000, buf, sizeof(buf), 1, 500);
    }
    hf_kill(hf_selfid());
}


/**
    Receive pixel vector and applies the requested filter
    Buffer format: [task, position, [pixel_vector]]
    if task is poison_pill, then this process must die
*/
void worker(void){
    int8_t buf[1500], filtered_pixel;
    uint16_t cpu, src_port, size, task;
    int16_t val;
    int32_t i,j;

    if (hf_comm_create(hf_selfid(), 5000, 0))
        panic(0xff);

    while (task != POISON_PILL){
        i = hf_recvprobe();
        if (i >= 0) {
            val = hf_recvack(&cpu, &src_port, buf, &size, i);
            if (val){
                printf("[WORKER %d] hf_recvack(): error %d\n", cpu, val);
            } else {
                // this does not work for images with resolutions greater than 256x256
                int task = buf[0]; // task type
                int tgt_position = buf[1]; // pixel position of target image
                int pos_flat_buffer = 2; // initial position of image buffer
                if (size == 11){ //3x3 sobel
                    int8_t block_buffer[3][3];
                    for (i = 0; i < 3; i++){
                        for (j = 0; j < 3; j++){
                            block_buffer[i][j] = buf[pos_flat_buffer];
                            pos_flat_buffer++;
                        }    
                    }
                    filtered_pixel = sobel(block_buffer);
                } else if (size == 27) { //5x5
                    //guaussian
                    int8_t block_buffer[5][5];
                    for (i = 0; i < 5; i++){
                        for (j = 0; j < 5; j++){
                            block_buffer[i][j] = buf[pos_flat_buffer];
                            pos_flat_buffer++;
                        }    
                    }
                    filtered_pixel = gausian(block_buffer);
                } else {
                    //bypass pixel
                    filtered_pixel = buf[pos_flat_buffer];
                }

                //send filtered buffer, one channel for worker
                val = hf_sendack(AGGREGATOR, 6000, image_buf, sizeof(image_buf), hf_selfid(), 500);
                if (val)
                    printf("[WORKER %d] Error sending the message to aggregator. Val = %d \n", hf_selfid(), val);
            }
        }
    }
    buf[0] = POISON_PILL;
    hf_sendack(AGGREGATOR, 5000, buf, sizeof(buf), hf_selfid(), 500);
    hf_kill(hf_selfid());
}

/**
    Aggregate the filtered pixels.
    Buffer format: [task, position, pixel value]
    if this process receive poison pill of all workers, then this process must die
*/
void aggregator(void){
    int8_t buf[1500], filtered_pixel, task, poison_pills=0, remaining_workers = 4; // cpus 2, 3, 4 and 5
    uint16_t cpu, src_port, size;
    int16_t val;
    int32_t i,j, channel;

    uint8_t *img;
    img = (uint8_t *) malloc(height * width);

    if (hf_comm_create(hf_selfid(), 6000, 0))
        panic(0xff);

    /*  */
    while (poison_pills < remaining_workers){
        channel = hf_recvprobe(); // retorna o channel, cuidado ao enviar, 
        if (channel >= 0) {
            val = hf_recvack(&cpu, &src_port, buf, &size, channel);
            if (val){
                printf("[WORKER %d] hf_recvack(): error %d\n", cpu, val);
            } else {
                // this does not work for images with resolutions greater than 256x256
                task = buf[0]; // task type
                int8_t tgt_position = buf[1]; // pixel position of target image
                int8_t filtered_pixel = buf[2];
                img[tgt_position] = filtered_pixel;
                if (task == POISON_PILL) {
                    poison_pills++;
                }
            }
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
        hf_spawn(master, 0, 0, 0, "filter", 2048);
    } else if (hf_cpuid() == AGGREGATOR){
        hf_spawn(aggregator, 0, 0, 0, "aggregator", 2048);
    } else {
        hf_spawn(worker, 0, 0, 0, "worker", 2048);
    }
}