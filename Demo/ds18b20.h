
#ifndef _DS18B20_H
#define _DS18B20_H

#include "onewire.h"


#define READ_ROM      0x33
#define SKIP_ROM      0xCC
#define CONVERT_T     0x44
#define READ_SCRATCH  0xBE




int ds18b20_setup(int _pin);

// _single routines assume only 1 device on bus
void ds18b20_readrom_single(int _pin, w1_block_obj_t *block_obj);

int ds18b20_readtemp_single(int _pin, w1_block_obj_t *block_obj);


#endif
