//ds18b20.c
//authored by Tony Brown
//

#ifndef NDEBUG 
#include <video.h>
#endif

#include <gpio.h>
#include "portmacro.h"
#include "onewire.h"
#include "ds18b20.h"


#define SEARCH_TRIES 7


static int w1_response_len = 0;
static int PARASITE_POWER = 0; 
static int PIN=4;



/* PUBLIC FUNCTIONS */

int 
ds18b20_setup(int _pin) {
    int pin = _pin;
    SetGpioDirection(pin, GPIO_IN);
    return w1_reset(pin);
}

void 
ds18b20_readrom_single(int pin, w1_block_obj_t *block_obj)
{
portENTER_CRITICAL()
    w1_reset(pin);
    w1_write(pin, READ_ROM, 1);    
    w1_read_block(pin, 8, block_obj);
portEXIT_CRITICAL()
    return;
}


#define WATCH_COUNT 4000000
int 
ds18b20_readtemp_single(int pin, w1_block_obj_t *block_obj)
{
portENTER_CRITICAL()
    w1_reset(pin);
    w1_write(pin, SKIP_ROM, 1);    
    w1_write(pin, CONVERT_T, 1);
    int start = GET_TIMER_COUNT();
    int end = GET_TIMER_COUNT();
    while (!w1_read_bit(pin)) {
        end = GET_TIMER_COUNT();
        if (end-start > WATCH_COUNT)
            return -1;
    }
    //the temp conv is done
    w1_reset(pin);
    w1_write(pin, SKIP_ROM, 1);    
    w1_write(pin, READ_SCRATCH, 1);    
    w1_read_block(pin, 9, block_obj);
    
portEXIT_CRITICAL()
    return 0;


}



