//onewire.h
//authored by Tony Brown (nemesis9@github)

//  This driver implements "bit-banging" GPIO 
//      cycles to drive the bus
//
//
//
#ifndef _ONEWIRE_H
#define _ONEWIRE_H

#ifndef NDEBUG
#include <video.h>
#endif

#define MAX_BYTES_TO_READ 9

typedef struct w1_search_obj {
    int junction;
    int exhausted;
    unsigned char addr[MAX_BYTES_TO_READ];
} w1_search_obj_t;


typedef struct w1_block_obj {
    unsigned char data[MAX_BYTES_TO_READ];
} w1_block_obj_t;



#define COUNTS_PER_US 4
void waitUs(int u);

void w1_data(int pin, int state);

void w1_write(int pin, unsigned char b, int power);

int w1_read_bit(int pin);

void w1_read_block(int pin, int bytes, w1_block_obj_t *block_obj);

int w1_reset(int pin);


#endif //ONEWIRE
